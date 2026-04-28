project(Openwakeword)
cmake_minimum_required(VERSION 3.14.0)

set (PACKAGE_NAME Openwakeword)
set (CMAKE_VERBOSE_MAKEFILE ON)
set (CMAKE_CXX_STANDARD 20)

# Enable Raw Props parsing in react-native (for Nitro Views)
add_compile_options(-DRN_SERIALIZABLE_STATE=1)

# Fetch TensorFlow Lite AAR directly to get headers and prebuilt binaries
set(TFLITE_VERSION "2.14.0")
set(TFLITE_AAR_URL "https://maven.google.com/org/tensorflow/tensorflow-lite/${TFLITE_VERSION}/tensorflow-lite-${TFLITE_VERSION}.aar")

include(FetchContent)
FetchContent_Declare(
  tflite_aar
  URL ${TFLITE_AAR_URL}
)
FetchContent_MakeAvailable(tflite_aar)

# The AAR contains the C/C++ headers in the "headers" directory
include_directories(${tflite_aar_SOURCE_DIR}/headers)

# The AAR contains the prebuilt .so binaries in the "jni" directory per architecture
add_library(libtensorflowlite SHARED IMPORTED)
set_target_properties(libtensorflowlite PROPERTIES IMPORTED_LOCATION
    ${tflite_aar_SOURCE_DIR}/jni/${ANDROID_ABI}/libtensorflowlite_jni.so)

# Define C++ library and add all sources
add_library(${PACKAGE_NAME} SHARED 
	src/main/cpp/cpp-adapter.cpp
	../cpp/HybridOpenwakeword.cpp
)

# Add Nitrogen specs :)
include(${CMAKE_SOURCE_DIR}/../nitrogen/generated/android/Openwakeword+autolinking.cmake)

# Set up local includes
include_directories(
        "src/main/cpp"
        "../cpp"
)

find_library(LOG_LIB log)

# Link all libraries together
target_link_libraries(
        ${PACKAGE_NAME}
        ${LOG_LIB}
        libtensorflowlite                         # <-- TensorFlow Lite NDK
        android                                   # <-- Android core
)
