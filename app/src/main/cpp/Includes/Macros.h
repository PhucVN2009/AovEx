#ifndef ANDROID_MOD_MENU_MACROS_H
#define ANDROID_MOD_MENU_MACROS_H

#include <vector>
#include <string>
#include <algorithm>
#include "STARCOOLX/Tools/Dobby/dobby.h"
#include "Logger.h"

// Menggunakan Dobby untuk hooking karena lebih stabil di Arm64 dan Armv7
inline void hook(void *offset, void* ptr, void **orig) {
    if (offset == nullptr) return;
    DobbyHook(offset, ptr, orig);
}

#define HOOK(offset, ptr, orig) hook((void *)getAbsoluteAddress(targetLibName, string2Offset(OBFUSCATE(offset))), (void *)ptr, (void **)&orig)
#define HOOK_LIB(lib, offset, ptr, orig) hook((void *)getAbsoluteAddress(OBFUSCATE(lib), string2Offset(OBFUSCATE(offset))), (void *)ptr, (void **)&orig)

// ... (sisanya tetap sama)
#endif //ANDROID_MOD_MENU_MACROS_H
