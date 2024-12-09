# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)

# Declares the project name. The project name can be accessed via ${ PROJECT_NAME},
# Since this is the top level CMakeLists.txt, the project name is also accessible
# with ${CMAKE_PROJECT_NAME} (both CMake variables are in-sync within the top level
# build script scope).
project("ffmpegdemo")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
#
# In this top level CMakeLists.txt, ${CMAKE_PROJECT_NAME} is used to define
# the target library name; in the sub-module's CMakeLists.txt, ${PROJECT_NAME}
# is preferred for the same purpose.
#
# In order to load a library into your app from Java/Kotlin, you must call
# System.loadLibrary() and pass the name of the library defined here;
# for GameActivity/NativeActivity derived applications, the same library name must be
# used in the AndroidManifest.xml file.

set(SRC_FFMPEG
        ffmpeg/cmdutils.c
        ffmpeg/ffmpeg.c
        ffmpeg/ffmpeg_demux.c
        ffmpeg/ffmpeg_filter.c
        ffmpeg/ffmpeg_hw.c
        ffmpeg/ffmpeg_mux.c
        ffmpeg/ffmpeg_mux_init.c
        ffmpeg/ffmpeg_opt.c
        ffmpeg/ffprobe.c
        ffmpeg/objpool.c
        ffmpeg/opt_common.c
        ffmpeg/sync_queue.c
        ffmpeg/thread_queue.c)

set(ffmpeg_libs_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../libs/${CMAKE_ANDROID_ARCH_ABI})
link_directories(${ffmpeg_libs_dir})
file(GLOB ffmpeg_libs ${ffmpeg_libs_dir}/*.so)
message("test:${ffmpeg_libs_dir}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

include_directories(.)
include_directories(include)

if(${CMAKE_ANDROID_ARCH_ABI} MATCHES "armeabi-v7a")
    include_directories(include/armeabi-v7a/libavcodec)
    include_directories(include/armeabi-v7a/libavutil)
    message("This is armeabi-v7a")
elseif(${CMAKE_ANDROID_ARCH_ABI} MATCHES "arm64-v8a")
    include_directories(include/arm64-v8a/libavcodec)
    include_directories(include/arm64-v8a/libavutil)
    message("This is arm64-v8a")
endif()

# 获取当前目录下所有的 .cpp 文件
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
file(GLOB SRC_CPP_FILES ${SRC_DIR}/*.cpp)
message("SRC_CPP_FILES:${SRC_CPP_FILES}")
add_library(${CMAKE_PROJECT_NAME} SHARED
        # List C/C++ source files with relative paths to this CMakeLists.txt.
        ${SRC_CPP_FILES})

# Specifies libraries CMake should link to your target library. You
# can link libraries from various origins, such as libraries defined in this
# build script, prebuilt third-party libraries, or Android system libraries.
target_link_libraries(${CMAKE_PROJECT_NAME}
        # List libraries link to the target library
        ${ffmpeg_libs}
        android
        log)

