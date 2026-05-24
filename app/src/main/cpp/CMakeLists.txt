cmake_minimum_required(VERSION 3.22.1)

project("starandroidinject")

# Menambahkan semua direktori yang berisi header (.h)
include_directories(
        .
        Includes
        KittyMemory
        STARCOOLX
        STARCOOLX/Dump/Il2Cpp
        STARCOOLX/IL2CppSDKGenerator
        STARCOOLX/Tools
        Substrate
)

# Mencari semua file sumber secara otomatis (.cpp dan .c)
file(GLOB_RECURSE SOURCE_FILES
        "*.cpp"
        "*.c"
)

# Konfigurasi library Dobby
add_library(dobby STATIC IMPORTED)
set_target_properties(dobby PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/STARCOOLX/Tools/Dobby/libraries/${ANDROID_ABI}/libdobby.a)

# Membuat library utama (Starcool) dengan semua file sumber yang ditemukan
add_library(Starcool SHARED
        ${SOURCE_FILES}
)

# Mencari library log bawaan Android
find_library(log-lib log)

# Menghubungkan library
target_link_libraries(Starcool
        ${log-lib}
        dobby
        GLESv2
        EGL
        atomic
        m
)
