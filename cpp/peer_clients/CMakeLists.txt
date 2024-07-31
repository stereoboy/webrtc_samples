cmake_minimum_required(VERSION 3.13)

# set(CMAKE_C_COMPILER clang) # Set C compiler to "clang"
# set(CMAKE_CXX_COMPILER clang++) # Set C++ compiler to "clang++"
set(CMAKE_C_COMPILER clang-18) # Set C compiler to "clang"
set(CMAKE_CXX_COMPILER clang++-18) # Set C++ compiler to "clang++"


set(llvm_BUILD_DIR ${CMAKE_SOURCE_DIR}/../llvm-project/build)
set(webrtc_DIR ${CMAKE_SOURCE_DIR}/../webrtc-checkout/src)
set(webrtc_BUILD_DIR ${CMAKE_SOURCE_DIR}/../webrtc-checkout/src/out/Release)

include_directories(
    ${llvm_BUILD_DIR}/include/c++/v1
)

add_compile_options(
  -Wno-undefined-bool-conversion -Wno-tautological-undefined-compare -std=c++17 -Wno-trigraphs -gsimple-template-names
  -fvisibility-inlines-hidden -Wnon-virtual-dtor -Woverloaded-virtual
  -nostdinc++
)

add_compile_definitions(
  _LIBCPP_ENABLE_SAFE_MODE=1
  _LIBCPP_DISABLE_VISIBILITY_ANNOTATIONS
  _LIBCXXABI_DISABLE_VISIBILITY_ANNOTATIONS
)

link_directories(
    ${llvm_BUILD_DIR}/lib/
)
add_link_options(

)

project(peer_clients) #Project name

set(BUILD_SHARED_LIBS ON)
add_subdirectory(./lib/socket.io-client-cpp)
set(SPDLOG_BUILD_SHARED ON)
add_subdirectory(./lib/spdlog)
# add_subdirectory(./lib/fmt)
# add_subdirectory(./lib/abseil-cpp/)

option(USE_PRECOMPILED_WEBRTC "use precompiled NVIDIA provided library" ON)

# set(CMAKE_CXX_STANDARD 20) // cannot use 20 for socket.io-client-cpp, error: no template named 'result_of' in namespace 'std'
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the missing find_package(Threads REQUIRED) command
find_package(Threads REQUIRED)
find_package(X11 REQUIRED)
# find_package(spdlog REQUIRED)
# find_package(fmt REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)
pkg_search_module(GIO REQUIRED gio-2.0)
message(STATUS "GLIB_LDFLAGS: ${GLIB_LDFLAGS}")
message(STATUS "GIO_LDFLAGS: ${GIO_LDFLAGS}")


#include_directories(
#  ${webrtc_DIR}
#  # ${webrtc_DIR}/third_party/abseil-cpp
#  # ${webrtc_DIR}/buildtools/third_party/libc++
#  # ${webrtc_DIR}/third_party/libyuv/include
#)
if (USE_PRECOMPILED_WEBRTC)
  include_directories(
    ${CMAKE_SOURCE_DIR}/../precompiled/webrtc_headers
    )
  add_library(webrtc STATIC IMPORTED)
  set_target_properties(webrtc PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../precompiled/libwebrtc.a)
else()
  include_directories(
    ${webrtc_DIR}
    )
  add_library(webrtc STATIC IMPORTED)
  set_target_properties(webrtc PROPERTIES IMPORTED_LOCATION ${webrtc_BUILD_DIR}/obj/libwebrtc.a)
endif()

# file(GLOB LIBCPP_OBJS ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/*.o)
# file(GLOB LIBCPPABI_OBJS ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++abi/libc++abi/*.o)
file(GLOB_RECURSE LIBABSL_OBJS ${webrtc_BUILD_DIR}/obj/third_party/abseil-cpp/absl/*.o)
set(PRECOMPILED_OBJS
    # ${LIBCPP_OBJS}
    # ${LIBCPPABI_OBJS}
    ${LIBABSL_OBJS}
)

# set_source_files_properties(${PRECOMPILED_OBJS} PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)

# Disable using the install rpath
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
# set(CMAKE_SKIP_BUILD_PATH FALSE)

################################################################################################################
# WebRTC Sample for DataChannel
#
add_executable(peer_datachannel_client src/peer_datachannel_client.cpp ${PRECOMPILED_OBJS})
# add_executable(peer_datachannel_client src/peer_datachannel_client.cpp)

# #linking library directory
target_link_directories(peer_datachannel_client PUBLIC
    ${llvm_BUILD_DIR}/lib/
)

target_include_directories(
    peer_datachannel_client PUBLIC
    ${llvm_BUILD_DIR}/include/c++/v1
    lib/socket.io-client-cpp/src
    lib/socket.io-client-cpp/lib/asio/asio/include
    # lib/abseil-cpp
    ${webrtc_DIR}/third_party/abseil-cpp
    include
)

target_compile_definitions(
    peer_datachannel_client PUBLIC
    _LIBCPP_ENABLE_SAFE_MODE=1
    _LIBCPP_DISABLE_VISIBILITY_ANNOTATIONS
    _LIBCXXABI_DISABLE_VISIBILITY_ANNOTATIONS
    # _LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE
    # CR_LIBCXX_REVISION=7cf98622abaf832e2d4784889ebc69d5b6fde4d8
    WEBRTC_POSIX
    WEBRTC_LINUX
    # ASIO_STANDALONE
)

target_compile_options(
  peer_datachannel_client PUBLIC
  -Wno-undefined-bool-conversion -Wno-tautological-undefined-compare -Wno-trigraphs
  -gsimple-template-names
  # -stdlib=libc++
  #---------------------------------------------------------
  # for socket.io-client-cpp
  # -fno-exceptions   // error: cannot use 'throw' with exceptions disabled
  # -fno-rtti         // error: use of typeid requires -frtti
  #---------------------------------------------------------
  -nostdinc++
  # -isystem${webrtc_DIR}/third_party/libc++/src/include
  # -isystem${webrtc_DIR}/third_party/libc++abi/src/include
  # --sysroot=${webrtc_DIR}/build/linux/debian_bullseye_amd64-sysroot
  -fvisibility-inlines-hidden -Wnon-virtual-dtor -Woverloaded-virtual
)

target_link_options(
    peer_datachannel_client PUBLIC
    -Werror -fuse-ld=lld-18
    -Wl,--fatal-warnings -Wl,--build-id -fPIC -Wl,-z,noexecstack
    -Wl,-z,relro -Wl,-z,now -Wl,--color-diagnostics -Wl,--undefined-version
    -Wl,--no-call-graph-profile-sort -m64 -no-canonical-prefixes -Wl,--gdb-index -rdynamic -Wl,-z,defs
    -Wl,--as-needed -nostdlib++
    # -Wl,--as-needed -stdlib=libc++
    # --sysroot=${webrtc_DIR}/build/linux/debian_bullseye_amd64-sysroot
)

target_link_libraries(
    peer_datachannel_client PUBLIC
    c++
    c++abi
    dl
    webrtc
    # sioclient_tls
    sioclient
    spdlog::spdlog
    # fmt::fmt
    # absl::flags_parse
    # absl::flags_usage
)
if (CMAKE_THREAD_LIBS_INIT)
    target_link_libraries(peer_datachannel_client PUBLIC ${CMAKE_THREAD_LIBS_INIT})
else()
    target_link_libraries(peer_datachannel_client PUBLIC pthread)
endif()

################################################################################################################
# WebRTC Sample for Audio
#
add_executable(peer_audio_client
    src/peer_audio_client.cpp
    # ${webrtc_DIR}/test/vcm_capturer.cc
    # ${webrtc_DIR}/test/platform_video_capturer.cc
    ${PRECOMPILED_OBJS}
)

# #linking library directory
target_link_directories(peer_audio_client PUBLIC
    ${llvm_BUILD_DIR}/lib/
)

target_include_directories(
    peer_audio_client PUBLIC
    ${llvm_BUILD_DIR}/include/c++/v1
    lib/socket.io-client-cpp/src
    lib/socket.io-client-cpp/lib/asio/asio/include
    ${webrtc_DIR}/third_party/abseil-cpp
    include
)

target_compile_definitions(
    peer_audio_client PUBLIC
    _LIBCPP_ENABLE_SAFE_MODE=1
    _LIBCPP_DISABLE_VISIBILITY_ANNOTATIONS
    _LIBCXXABI_DISABLE_VISIBILITY_ANNOTATIONS
    # _LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE
    # CR_LIBCXX_REVISION=7cf98622abaf832e2d4784889ebc69d5b6fde4d8
    WEBRTC_POSIX
    WEBRTC_LINUX
    # ASIO_STANDALONE
)

target_compile_options(
    peer_audio_client PUBLIC
    -Wno-undefined-bool-conversion -Wno-tautological-undefined-compare -Wno-trigraphs
    -gsimple-template-names
    # -stdlib=libc++
    #---------------------------------------------------------
    # for socket.io-client-cpp
    # -fno-exceptions   // error: cannot use 'throw' with exceptions disabled
    # -fno-rtti         // error: use of typeid requires -frtti
    -fno-rtti
    #---------------------------------------------------------
    -nostdinc++
    # -isystem${webrtc_DIR}/third_party/libc++/src/include
    # -isystem${webrtc_DIR}/third_party/libc++abi/src/include
    # --sysroot=${webrtc_DIR}/build/linux/debian_bullseye_amd64-sysroot
    -fvisibility-inlines-hidden -Wnon-virtual-dtor -Woverloaded-virtual
)

target_link_options(
    peer_audio_client PUBLIC
    -Werror -fuse-ld=lld-18
    -Wl,--fatal-warnings -Wl,--build-id -fPIC -Wl,-z,noexecstack
    -Wl,-z,relro -Wl,-z,now -Wl,--color-diagnostics -Wl,--undefined-version
    -Wl,--no-call-graph-profile-sort -m64 -no-canonical-prefixes -Wl,--gdb-index -rdynamic -Wl,-z,defs
    -Wl,--as-needed -nostdlib++
    # -Wl,--as-needed -stdlib=libc++
    # --sysroot=${webrtc_DIR}/build/linux/debian_bullseye_amd64-sysroot
)

target_link_libraries(
    peer_audio_client PUBLIC
    c++
    c++abi
    dl
    webrtc
    # sioclient_tls
    sioclient
    spdlog::spdlog
    ${X11_LIBRARIES} # Add the X11 library
    ${GLIB_LIBRARIES}
    ${GIO_LIBRARIES}
    rt
)

if (CMAKE_THREAD_LIBS_INIT)
    target_link_libraries(peer_audio_client PUBLIC ${CMAKE_THREAD_LIBS_INIT})
else()
    target_link_libraries(peer_audio_client PUBLIC pthread)
endif()

################################################################################################################
# WebRTC Sample for Video
#
file(GLOB_RECURSE TEST_OBJS
    ${webrtc_BUILD_DIR}/obj/test/platform_video_capturer/*.o
    ${webrtc_BUILD_DIR}/obj/test/test_video_capturer/*.o
)

# add_library(video_track_source STATIC IMPORTED)
# set_target_properties(video_track_source PROPERTIES IMPORTED_LOCATION ${webrtc_BUILD_DIR}/obj/pc/libvideo_track_source.a)

add_executable(peer_video_client
    src/peer_video_client.cpp
    # ${webrtc_DIR}/test/vcm_capturer.cc
    # ${webrtc_DIR}/test/platform_video_capturer.cc
    ${PRECOMPILED_OBJS}
    ${TEST_OBJS}
)

# #linking library directory
target_link_directories(peer_video_client PUBLIC
    ${llvm_BUILD_DIR}/lib/
)

target_include_directories(
    peer_video_client PUBLIC
    ${llvm_BUILD_DIR}/include/c++/v1
    lib/socket.io-client-cpp/src
    lib/socket.io-client-cpp/lib/asio/asio/include
    ${webrtc_DIR}/third_party/abseil-cpp
    include
)

target_compile_definitions(
    peer_video_client PUBLIC
    _LIBCPP_ENABLE_SAFE_MODE=1
    _LIBCPP_DISABLE_VISIBILITY_ANNOTATIONS
    _LIBCXXABI_DISABLE_VISIBILITY_ANNOTATIONS
    # _LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE
    # CR_LIBCXX_REVISION=7cf98622abaf832e2d4784889ebc69d5b6fde4d8
    WEBRTC_POSIX
    WEBRTC_LINUX
    # ASIO_STANDALONE
)

target_compile_options(
    peer_video_client PUBLIC
    -Wno-undefined-bool-conversion -Wno-tautological-undefined-compare -Wno-trigraphs
    -gsimple-template-names
    # -stdlib=libc++
    #---------------------------------------------------------
    # for socket.io-client-cpp
    # -fno-exceptions   // error: cannot use 'throw' with exceptions disabled
    # -fno-rtti         // error: use of typeid requires -frtti
    -fno-rtti
    #---------------------------------------------------------
    -nostdinc++
    # -isystem${webrtc_DIR}/third_party/libc++/src/include
    # -isystem${webrtc_DIR}/third_party/libc++abi/src/include
    # --sysroot=${webrtc_DIR}/build/linux/debian_bullseye_amd64-sysroot
    -fvisibility-inlines-hidden -Wnon-virtual-dtor -Woverloaded-virtual
)

target_link_options(
    peer_video_client PUBLIC
    -Werror -fuse-ld=lld-18
    -Wl,--fatal-warnings -Wl,--build-id -fPIC -Wl,-z,noexecstack
    -Wl,-z,relro -Wl,-z,now -Wl,--color-diagnostics -Wl,--undefined-version
    -Wl,--no-call-graph-profile-sort -m64 -no-canonical-prefixes -Wl,--gdb-index -rdynamic -Wl,-z,defs
    -Wl,--as-needed -nostdlib++
    # -Wl,--as-needed -stdlib=libc++
    # --sysroot=${webrtc_DIR}/build/linux/debian_bullseye_amd64-sysroot
)

target_link_libraries(
    peer_video_client PUBLIC
    c++
    c++abi
    dl
    webrtc
    # sioclient_tls
    sioclient
    spdlog::spdlog
    ${X11_LIBRARIES} # Add the X11 library
    ${GLIB_LIBRARIES}
    ${GIO_LIBRARIES}
    rt
)

if (CMAKE_THREAD_LIBS_INIT)
    target_link_libraries(peer_video_client PUBLIC ${CMAKE_THREAD_LIBS_INIT})
else()
    target_link_libraries(peer_video_client PUBLIC pthread)
endif()