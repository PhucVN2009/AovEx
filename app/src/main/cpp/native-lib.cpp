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

// ========== TOGGLE FLAGS ==========
bool hackMap = false;     // Map Hack (visibility)
bool hackCamXa = false;   // Cam Xa toggle
float camXaValue = 0.0f;  // 0 - 10

// ========== MAP HACK (SetVisible) ==========
enum COM_PLAYERCAMP {
    ComPlayercampMid = 0,
    ComPlayercamp1 = 1,
    ComPlayercamp2 = 2
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
    float original = old_GetCameraHeightRateValue(instance, type);
    if (instance != NULL && hackCamXa) {
        return original + camXaValue;
    }
    return original;
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

    // Hook Map Hack (SetVisible)
    DobbyHook((void *)(il2cppMap + 0x6BCE384), (void *) SetVisible, (void **) &_SetVisible);

    // Hook Cam Xa (GetCameraHeightRateValue)
    DobbyHook((void *)(il2cppMap + 0x8D546F4), (void *) GetCameraHeightRateValue, (void **) &old_GetCameraHeightRateValue);

    // Giữ thread chạy (không có coins loop)
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
