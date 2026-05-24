#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include "Includes/obfuscate.h"
#include "Includes/Logger.h"
#include "Includes/Macros.h"
#include "Includes/JNIStuff.h"
#include "Includes/Utils.h"
#include "STARCOOLX/Call_Me.h"
#include "STARCOOL.h"
#include "SocketControl.h"
#include "ESP.h"

// ========== TOGGLE FLAGS ==========
bool hackMap    = false;   // Map Hack (visibility)
bool hackCamXa  = false;   // Cam Xa toggle
float camXaValue = 0.0f;   // 0 - 10
// hackESP is declared in ESP.h

// ========== MAP HACK (SetVisible) ==========
enum COM_PLAYERCAMP {
    ComPlayercampMid = 0,
    ComPlayercamp1   = 1,
    ComPlayercamp2   = 2
};

void (*_SetVisible)(void *instance, COM_PLAYERCAMP camp, bool bVisible, bool forceSync);
void SetVisible(void *instance, COM_PLAYERCAMP camp, bool bVisible, bool forceSync) {
    if (instance != NULL && hackMap) {
        if (camp == ComPlayercamp1 || camp == ComPlayercamp2) {
            bVisible = true;
        }
    }
    _SetVisible(instance, camp, bVisible, forceSync);
}

// ========== CAM XA HOOK ==========
// RVA: 0x8D546F4 - float GetCameraHeightRateValue(int type)
float (*old_GetCameraHeightRateValue)(void *instance, int type);
float GetCameraHeightRateValue(void *instance, int type) {
    // Capture CameraSystem instance for ESP use
    if (instance && !g_camSysInst) {
        g_camSysInst = instance;
    }
    float original = old_GetCameraHeightRateValue(instance, type);
    if (instance != NULL && hackCamXa) {
        return original + camXaValue;
    }
    return original;
}

// ========== ESP: CameraSystem.LateUpdate HOOK ==========
// RVA: 0x8D53F98
void (*old_CameraLateUpdate)(void *instance, void *mi);
void CameraLateUpdate(void *instance, void *mi) {
    // Always capture the camera system instance
    g_camSysInst = instance;
    // Update ESP actor data on Unity's main thread
    ESP_Update();
    old_CameraLateUpdate(instance, mi);
}

// ========== ESP: glViewport HOOK (capture screen size) ==========
typedef void (*fn_glViewport)(GLint x, GLint y, GLsizei w, GLsizei h);
fn_glViewport old_glViewport = nullptr;
void new_glViewport(GLint x, GLint y, GLsizei w, GLsizei h) {
    // Accept only the primary full-screen viewport
    if (w > 200 && h > 200) {
        g_sw = (int)w;
        g_sh = (int)h;
    }
    old_glViewport(x, y, w, h);
}

// ========== ESP: eglSwapBuffers HOOK (render ESP overlay) ==========
typedef EGLBoolean (*fn_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
fn_eglSwapBuffers old_eglSwapBuffers = nullptr;
EGLBoolean new_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    ESP_Render();
    return old_eglSwapBuffers(dpy, surface);
}

// ========== THREAD CHÍNH ==========
void *Init_Thread(void *) {
    uintptr_t il2cppMap = 0;
    for (int i = 0; i < 15; i++) {
        il2cppMap = Tools::GetBaseAddress("libil2cpp.so");
        if (il2cppMap != 0) break;
        sleep(2);
    }
    if (il2cppMap == 0) return nullptr;

    // ── Hook Map Hack (SetVisible) ──────────────────────────────────────────
    DobbyHook((void *)(il2cppMap + 0x6BCE384),
              (void *) SetVisible,
              (void **) &_SetVisible);

    // ── Hook Cam Xa (GetCameraHeightRateValue) ──────────────────────────────
    DobbyHook((void *)(il2cppMap + 0x8D546F4),
              (void *) GetCameraHeightRateValue,
              (void **) &old_GetCameraHeightRateValue);

    // ── ESP: Hook CameraSystem.LateUpdate (RVA: 0x8D53F98) ─────────────────
    DobbyHook((void *)(il2cppMap + 0x8D53F98),
              (void *) CameraLateUpdate,
              (void **) &old_CameraLateUpdate);

    // ── ESP: Set up IL2CPP function pointers ────────────────────────────────
    // KyriosFramework.get_actorManager()  static, RVA: 0x89179A8
    fpGetActorMgr = (fn_GetActorMgr)(il2cppMap + 0x89179A8);
    // CameraSystem.get_MainCamera()       instance, RVA: 0x8D53F48
    fpGetMainCam  = (fn_GetMainCam)(il2cppMap + 0x8D53F48);
    // Camera.WorldToScreenPoint(Vector3)  instance, RVA: 0x94ADBF4
    fpW2S         = (fn_W2S)(il2cppMap + 0x94ADBF4);

    // ── ESP: Hook glViewport to capture screen size ─────────────────────────
    void *libGLES = dlopen("libGLESv2.so", RTLD_LAZY);
    if (libGLES) {
        void *pViewport = dlsym(libGLES, "glViewport");
        if (pViewport) {
            DobbyHook(pViewport,
                      (void *) new_glViewport,
                      (void **) &old_glViewport);
        }
    }

    // ── ESP: Hook eglSwapBuffers to draw overlay ────────────────────────────
    void *libEGL = dlopen("libEGL.so", RTLD_LAZY);
    if (libEGL) {
        void *pSwap = dlsym(libEGL, "eglSwapBuffers");
        if (pSwap) {
            DobbyHook(pSwap,
                      (void *) new_eglSwapBuffers,
                      (void **) &old_eglSwapBuffers);
        }
    }

    // Keep thread alive
    while (true) {
        sleep(1);
    }
    return nullptr;
}

// ========== LIBRARY ENTRY ==========
__attribute__((constructor))
void lib_main() {
    pthread_t ptid;
    pthread_create(&ptid, NULL, socket_server_thread, NULL);
    pthread_t myThread;
    pthread_create(&myThread, NULL, Init_Thread, NULL);
}

// ========== JNI ==========
extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    return JNI_VERSION_1_6;
}
