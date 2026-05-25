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
#include "AntiDialog.h"

// ========== TOGGLE FLAGS ==========
bool hackMap        = false;
bool hackCamXa      = false;
float camXaValue    = 0.0f;
bool hackAntiDialog = false;
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
    // Dùng RTLD_DEFAULT để tìm symbol đã được game load, không mở lại libGLESv2.so
    {
        void *pViewport = dlsym(RTLD_DEFAULT, "glViewport");
        if (pViewport) {
            DobbyHook(pViewport,
                      (void *) new_glViewport,
                      (void **) &old_glViewport);
        }
    }

    // ── ESP: Hook eglSwapBuffers to draw overlay ────────────────────────────
    // Dùng RTLD_DEFAULT để intercept đúng hàm Unity đang dùng trong process
    {
        void *pSwap = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
        if (pSwap) {
            DobbyHook(pSwap,
                      (void *) new_eglSwapBuffers,
                      (void **) &old_eglSwapBuffers);
        }
    }

    // ── Anti-Dialog: OpenMessageBoxBase (CATCH-ALL mọi dialog OLDSYS) ──────
    // RVA: 0x72D8DEC — tất cả OpenMessageBox/* đều gọi hàm này
    DobbyHook((void *)(il2cppMap + 0x72D8DEC),
              (void *) Hook_OpenMessageBoxBase,
              (void **) &old_OpenMessageBoxBase);
    // Handle_NetworkOpenMessageBoxProto  RVA: 0x742EB28
    DobbyHook((void *)(il2cppMap + 0x742EB28),
              (void *) Hook_Handle_NetworkOpenMessageBoxProto,
              (void **) &old_Handle_NetworkOpenMessageBoxProto);
    // IOpenMessageBox (NetworkModule)    RVA: 0x742E698
    DobbyHook((void *)(il2cppMap + 0x742E698),
              (void *) Hook_IOpenMessageBox_Net,
              (void **) &old_IOpenMessageBox_Net);
    // IOpenMessageBoxCallback (NetworkModule) RVA: 0x742E854
    DobbyHook((void *)(il2cppMap + 0x742E854),
              (void *) Hook_IOpenMessageBoxCallback_Net,
              (void **) &old_IOpenMessageBoxCallback_Net);
    // IOpenMessageBox (LNetwork)         RVA: 0x6F2D02C
    DobbyHook((void *)(il2cppMap + 0x6F2D02C),
              (void *) Hook_IOpenMessageBox_LNet,
              (void **) &old_IOpenMessageBox_LNet);
    // IOpenMessageBoxCallback (LNetwork) RVA: 0x6F29208
    DobbyHook((void *)(il2cppMap + 0x6F29208),
              (void *) Hook_IOpenMessageBoxCallback_LNet,
              (void **) &old_IOpenMessageBoxCallback_LNet);
    // checkAndShowBanGameAlert           RVA: 0x77ED020
    DobbyHook((void *)(il2cppMap + 0x77ED020),
              (void *) Hook_checkAndShowBanGameAlert,
              (void **) &old_checkAndShowBanGameAlert);
    // OpenPunishBanGameAlert             RVA: 0x72E0AEC
    DobbyHook((void *)(il2cppMap + 0x72E0AEC),
              (void *) Hook_OpenPunishBanGameAlert,
              (void **) &old_OpenPunishBanGameAlert);

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
