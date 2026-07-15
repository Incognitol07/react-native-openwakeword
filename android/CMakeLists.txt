project(Openwakeword)
cmake_minimum_required(VERSION 3.14.0)

set(PACKAGE_NAME Openwakeword)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 20)

add_compile_options(-DRN_SERIALIZABLE_STATE=1)

# TensorFlow Lite AAR.
# Keep this on 2.10.0 for now: the 2.14.0 AAR headers reference
# tensorflow/lite/core/async/c/types.h, but that file is not included in the
# extracted AAR header tree, which breaks native Android builds.
set(TFLITE_VERSION "2.10.0")
set(TFLITE_AAR_URL
    "https://repo1.maven.org/maven2/org/tensorflow/tensorflow-lite/${TFLITE_VERSION}/tensorflow-lite-${TFLITE_VERSION}.aar")

include(FetchContent)
FetchContent_Declare(tflite_aar URL ${TFLITE_AAR_URL})
FetchContent_MakeAvailable(tflite_aar)

# Headers extracted from the AAR.
include_directories(${tflite_aar_SOURCE_DIR}/headers)

add_library(libtensorflowlite SHARED IMPORTED)
set_target_properties(libtensorflowlite PROPERTIES IMPORTED_LOCATION
    ${tflite_aar_SOURCE_DIR}/jni/${ANDROID_ABI}/libtensorflowlite_jni.so)

# ── Library ───────────────────────────────────────────────────────────────
add_library(${PACKAGE_NAME} SHARED
    src/main/cpp/cpp-adapter.cpp
    ../cpp/HybridOpenwakeword.cpp
)

include(${CMAKE_SOURCE_DIR}/../nitrogen/generated/android/Openwakeword+autolinking.cmake)

include_directories("src/main/cpp" "../cpp")

find_library(LOG_LIB log)

target_link_libraries(${PACKAGE_NAME}
    ${LOG_LIB}
    libtensorflowlite
    android
)

# ── Per-ABI optimisation flags ────────────────────────────────────────────
# These replace the -O2 in build.gradle. CMakeLists owns all optimisation.
# -ffast-math:        allows auto-vectorisation of FP loops (IEEE compliance
#                     blocks it at -O2). Safe for audio/mel processing.
# -march=armv8-a+simd unlocks NEON auto-vectorisation on arm64.
# -march=armv7-a -mfpu=neon  equivalent for 32-bit ARM.
# debug builds get -O0 -g for debuggability.
# ─────────────────────────────────────────────────────────────────────────
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PACKAGE_NAME} PRIVATE -O0 -g)
else()
    if(ANDROID_ABI STREQUAL "arm64-v8a")
        target_compile_options(${PACKAGE_NAME} PRIVATE
            -O3
            -ffast-math
            -march=armv8-a+simd
            -fno-omit-frame-pointer   # retain stack frames for profiling
        )
    elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
        target_compile_options(${PACKAGE_NAME} PRIVATE
            -O3
            -ffast-math
            -march=armv7-a
            -mfpu=neon
            -mfloat-abi=softfp
            -fno-omit-frame-pointer
        )
    else()
        # x86 / x86_64 (emulator)
        target_compile_options(${PACKAGE_NAME} PRIVATE
            -O3
            -ffast-math
        )
    endif()
endif()
