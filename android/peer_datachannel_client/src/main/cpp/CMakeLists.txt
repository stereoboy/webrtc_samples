
#
# Copyright (C) The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required(VERSION 3.4.1)

set(webrtc_include_DIR ${WEBRTC_DIR}/include)
set(webrtc_out_DIR ${WEBRTC_DIR}/src/out/${ANDROID_ABI})
add_library(lib_webrtc STATIC IMPORTED)
set_target_properties(lib_webrtc PROPERTIES IMPORTED_LOCATION
        ${webrtc_out_DIR}/obj/libwebrtc.a)

# configure import libs
set(distribution_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../distribution)
set(common_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../common)

# shared lib will also be tucked into APK and sent to target
# refer to app/build.gradle, jniLibs section for that purpose.
# ${ANDROID_ABI} is handy for our purpose here. Probably this ${ANDROID_ABI} is
# the most valuable thing of this sample, the rest are pretty much normal cmake
add_library(lib_sio_client STATIC IMPORTED)
set_target_properties(lib_sio_client PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/lib/${ANDROID_ABI}/libsioclient.a)

# build application's shared lib
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(hello-libs SHARED
            hello-libs.cpp
            peer_datachannel_client.cpp)

target_include_directories(hello-libs PRIVATE
        ${distribution_DIR}/include
        ${common_DIR}/include
        ${webrtc_include_DIR}
        ${webrtc_include_DIR}/third_party/abseil-cpp
        )

target_compile_definitions(
        hello-libs PUBLIC
        WEBRTC_POSIX
        WEBRTC_LINUX
        __STDC_CONSTANT_MACROS
        __STDC_FORMAT_MACROS
        _FORTIFY_SOURCE=2
        _GNU_SOURCE
        ANDROID
        HAVE_SYS_UIO_H
        ANDROID_NDK_VERSION_ROLL=r21d_1
        CR_CLANG_REVISION=\"llvmorg-18-init-4631-gd50b56d1-1\"
        NDEBUG
        NVALGRIND
        DYNAMIC_ANNOTATIONS_ENABLED=0
        WEBRTC_ENABLE_PROTOBUF=1
        WEBRTC_STRICT_FIELD_TRIALS=0
        WEBRTC_INCLUDE_INTERNAL_AUDIO_DEVICE
        RTC_ENABLE_VP9
        RTC_DAV1D_IN_INTERNAL_DECODER_FACTORY
        WEBRTC_HAVE_SCTP
        WEBRTC_ENABLE_LIBEVENT
        WEBRTC_LIBRARY_IMPL
        WEBRTC_ENABLE_AVX2
        WEBRTC_NON_STATIC_TRACE_EVENT_HANDLERS=1
        WEBRTC_POSIX
        WEBRTC_LINUX
        WEBRTC_ANDROID
        ABSL_ALLOCATOR_NOTHROW=1
        LIBYUV_DISABLE_NEON
        GOOGLE_PROTOBUF_NO_RTTI
        GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
        GOOGLE_PROTOBUF_INTERNAL_DONATE_STEAL_INLINE=0
        HAVE_PTHREAD
)

#if(${ANDROID_ABI} STREQUAL "armeabi-v7a")
#    message(STATUS "Configuring for armeabi-v7a")
#elseif(${ANDROID_ABI} STREQUAL "arm64-v8a")
#    message(STATUS "Configuring for arm64-v8a")
#    target_compile_definitions(hello-libs PUBLIC
#            __STDC_CONSTANT_MACROS
#            __STDC_FORMAT_MACROS
#            _FORTIFY_SOURCE=2
#            _GNU_SOURCE
#            ANDROID
#            HAVE_SYS_UIO_H
#            ANDROID_NDK_VERSION_ROLL=r21d_1
#            CR_CLANG_REVISION=\"llvmorg-18-init-4631-gd50b56d1-1\"
#            NDEBUG
#            NVALGRIND
#            DYNAMIC_ANNOTATIONS_ENABLED=0
#            WEBRTC_ENABLE_PROTOBUF=1
#            WEBRTC_STRICT_FIELD_TRIALS=0
#            WEBRTC_INCLUDE_INTERNAL_AUDIO_DEVICE
#            RTC_ENABLE_VP9
#            RTC_DAV1D_IN_INTERNAL_DECODER_FACTORY
#            WEBRTC_HAVE_SCTP
#            WEBRTC_ENABLE_LIBEVENT
#            WEBRTC_ARCH_ARM64
#            WEBRTC_HAS_NEON
#            WEBRTC_LIBRARY_IMPL
#            WEBRTC_ENABLE_AVX2
#            WEBRTC_NON_STATIC_TRACE_EVENT_HANDLERS=1
#            WEBRTC_POSIX
#            WEBRTC_LINUX
#            WEBRTC_ANDROID
#            ABSL_ALLOCATOR_NOTHROW=1
#            GOOGLE_PROTOBUF_NO_RTTI
#            GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
#            GOOGLE_PROTOBUF_INTERNAL_DONATE_STEAL_INLINE=0
#            HAVE_PTHREAD
#
#            -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D_FORTIFY_SOURCE=2 -D_GNU_SOURCE -DANDROID -DHAVE_SYS_UIO_H -DANDROID_NDK_VERSION_ROLL=r21d_1 -DCR_CLANG_REVISION=\"llvmorg-18-init-4631-gd50b56d1-1\" -DNDEBUG -DNVALGRIND -DDYNAMIC_ANNOTATIONS_ENABLED=0 -DWEBRTC_ENABLE_PROTOBUF=1 -DWEBRTC_STRICT_FIELD_TRIALS=0 -DWEBRTC_INCLUDE_INTERNAL_AUDIO_DEVICE -DRTC_ENABLE_VP9 -DRTC_DAV1D_IN_INTERNAL_DECODER_FACTORY -DWEBRTC_HAVE_SCTP -DWEBRTC_ENABLE_LIBEVENT -DWEBRTC_ARCH_ARM64 -DWEBRTC_HAS_NEON -DWEBRTC_ENABLE_AVX2 -DWEBRTC_NON_STATIC_TRACE_EVENT_HANDLERS=1 -DWEBRTC_POSIX -DWEBRTC_LINUX -DWEBRTC_ANDROID -DABSL_ALLOCATOR_NOTHROW=1 -DABSL_FLAGS_STRIP_NAMES=0
#            )
#elseif(${ANDROID_ABI} STREQUAL "x86")
#    message(STATUS "Configuring for x86")
#elseif(${ANDROID_ABI} STREQUAL "x86_64")
#    message(STATUS "Configuring for x86_64")
#else()
#    message(WARNING "Unknown Android ABI: ${ANDROID_ABI}")
#endif()

#target_compile_options(hello-libs PUBLIC
#        -fvisibility-inlines-hidden
#        -fno-rtti
#        -fdata-sections -ffunction-sections -fno-unique-section-names -fno-math-errno -std=c++20 -Wno-trigraphs
#        -mno-outline
#
#        -std=c++20 -Wno-trigraphs -fno-rtti --sysroot=/home/wom/android-sdk/ndk/21.4.7075529/toolchains/llvm/prebuilt/linux-x86_64/sysroot -fvisibility-inlines-hidden -Wnon-virtual-dtor -Woverloaded-virtual
#)

target_link_libraries(hello-libs
                      android
                      lib_sio_client
                      lib_webrtc
                      log)
