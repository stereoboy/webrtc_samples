
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

set(webrtc_out_DIR ${WEBRTC_DIR}/src/out/${ANDROID_ABI})
set(webrtc_src_DIR ${WEBRTC_DIR}/src)
add_library(lib_webrtc STATIC IMPORTED)
set_target_properties(lib_webrtc PROPERTIES IMPORTED_LOCATION
        ${webrtc_out_DIR}/obj/libwebrtc.a)

# configure import libs
set(distribution_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../distribution)
set(common_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../common)

add_library(lib_gmath STATIC IMPORTED)
set_target_properties(lib_gmath PROPERTIES IMPORTED_LOCATION
    ${distribution_DIR}/gmath/lib/${ANDROID_ABI}/libgmath.a)

# shared lib will also be tucked into APK and sent to target
# refer to app/build.gradle, jniLibs section for that purpose.
# ${ANDROID_ABI} is handy for our purpose here. Probably this ${ANDROID_ABI} is
# the most valuable thing of this sample, the rest are pretty much normal cmake
add_library(lib_gperf SHARED IMPORTED)
set_target_properties(lib_gperf PROPERTIES IMPORTED_LOCATION
    ${distribution_DIR}/gperf/lib/${ANDROID_ABI}/libgperf.so)

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
        ${webrtc_src_DIR}
        ${webrtc_src_DIR}/third_party/abseil-cpp
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

target_link_libraries(hello-libs
                      android
                      lib_sio_client
                      lib_webrtc
                      log)
