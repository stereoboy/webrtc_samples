cmake_minimum_required(VERSION 3.13)

option(USE_PRECOMPILED_NVIDIA_WEBRTC "use precompiled NVIDIA provided library" OFF)
option(USE_PRECOMPILED_WEBRTC "use precompiled library" ON)
option(USE_CUSTOM_LIBCXX "Use in-tree libc++ (buildtools/third_party/libc++, otherwise (OFF) use the host_cpu toolchain" ON)

message(STATUS "USE_CUSTOM_LIBCXX=<${USE_CUSTOM_LIBCXX}>")

set(webrtc_DIR ${CMAKE_SOURCE_DIR}/../webrtc-checkout/src)

if (USE_CUSTOM_LIBCXX)
    set(webrtc_suffix "_use_custom_libcxx")
endif()
#
# Build type settings
#
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(DEFAULT_BUILD_TYPE "Release")
    message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Choose the type of build." FORCE)

    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
message(STATUS "BUILD_TYPE=<${BUILD_TYPE}>")
if(BUILD_TYPE STREQUAL "DEBUG")
    message(STATUS "Debug build")
    set(webrtc_BUILD_DIR ${CMAKE_SOURCE_DIR}/../webrtc-checkout/src/out/Debug${webrtc_suffix})
elseif(BUILD_TYPE STREQUAL "RELEASE")
    message(STATUS "Release build")
    set(webrtc_BUILD_DIR ${CMAKE_SOURCE_DIR}/../webrtc-checkout/src/out/Release${webrtc_suffix})
elseif(BUILD_TYPE STREQUAL "RELWITHDEBINFO")
    message(STATUS "RelWithDebInfo build")
    # Add specific settings for RelWithDebInfo build here
elseif(BUILD_TYPE STREQUAL "MINSIZEREL")
    message(STATUS "MinSizeRel build")
    # Add specific settings for MinSizeRel build here
else()
    message(WARNING "Unknown build type: ${CMAKE_BUILD_TYPE}")
endif()

message(STATUS "webrtc_BUILD_DIR=<${webrtc_BUILD_DIR}>")


if (USE_CUSTOM_LIBCXX)
    #
    # Compiler settings
    #
    # set(CMAKE_C_COMPILER clang) # Set C compiler to "clang"
    # set(CMAKE_CXX_COMPILER clang++) # Set C++ compiler to "clang++"
    set(CMAKE_C_COMPILER clang-18) # Set C compiler to "clang"
    set(CMAKE_CXX_COMPILER clang++-18) # Set C++ compiler to "clang++"

    set(llvm_BUILD_DIR ${CMAKE_SOURCE_DIR}/../llvm-project/build)

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
endif()

project(peer_clients) #Project name

set(BUILD_SHARED_LIBS ON)
add_subdirectory(./lib/socket.io-client-cpp)
set(SPDLOG_BUILD_SHARED ON)
add_subdirectory(./lib/spdlog)
# add_subdirectory(./lib/fmt)
add_subdirectory(./lib/abseil-cpp/)

# set(CMAKE_CXX_STANDARD 20) // cannot use 20 for socket.io-client-cpp, error: no template named 'result_of' in namespace 'std'
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the missing find_package(Threads REQUIRED) command
find_package(Threads REQUIRED)
find_package(X11 REQUIRED)
# find_package(spdlog REQUIRED)
# find_package(fmt REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(LIBPULSE REQUIRED libpulse)
pkg_search_module(GLIB REQUIRED glib-2.0)
pkg_search_module(GIO REQUIRED gio-2.0)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(GMODULE REQUIRED gmodule-2.0)
pkg_check_modules(GTHREAD REQUIRED gthread-2.0)
message(STATUS "GLIB_LIBRARIES: ${GLIB_LIBRARIES}")
message(STATUS "GIO_LIBRARIES: ${GIO_LIBRARIES}")
message(STATUS "GTK3_LIBRARIES: ${GTK3_LIBRARIES}")
message(STATUS "GMODULE_LIBRARIES: ${GMODULE_LIBRARIES}")
message(STATUS "GTHREAD_LIBRARIES: ${GTHREAD_LIBRARIES}")

#include_directories(
#  ${webrtc_DIR}
#  # ${webrtc_DIR}/third_party/abseil-cpp
#  # ${webrtc_DIR}/buildtools/third_party/libc++
#  # ${webrtc_DIR}/third_party/libyuv/include
#)

message(STATUS "USE_PRECOMPILED_NVIDIA_WEBRTC=<${USE_PRECOMPILED_NVIDIA_WEBRTC}>")
message(STATUS "USE_PRECOMPILED_WEBRTC=<${USE_PRECOMPILED_WEBRTC}>")
if(USE_PRECOMPILED_NVIDIA_WEBRTC)
  include_directories(
    ${CMAKE_SOURCE_DIR}/../precompiled/webrtc_headers
    )
  add_library(webrtc STATIC IMPORTED)
  set_target_properties(webrtc PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../precompiled/libwebrtc.a)
elseif (USE_PRECOMPILED_WEBRTC)
  include_directories(
    ${CMAKE_SOURCE_DIR}/../precompiled/include
    )
  add_library(webrtc STATIC IMPORTED)
  if(BUILD_TYPE STREQUAL "DEBUG")
    set_target_properties(webrtc PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../precompiled/src/out/Debug${webrtc_suffix}/obj/libwebrtc.a)
  elseif(BUILD_TYPE STREQUAL "RELEASE")
    set_target_properties(webrtc PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../precompiled/src/out/Release${webrtc_suffix}/obj/libwebrtc.a)
  else()
    message(WARNING "Unknown build type: ${CMAKE_BUILD_TYPE}")
  endif()
else()
  include_directories(
    ${webrtc_DIR}
    )
  add_library(webrtc STATIC IMPORTED)
  set_target_properties(webrtc PROPERTIES IMPORTED_LOCATION ${webrtc_BUILD_DIR}/obj/libwebrtc.a)
endif()

if (USE_CUSTOM_LIBCXX)
    # file(GLOB LIBCPP_OBJS ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/*.o)
    # file(GLOB LIBCPPABI_OBJS ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++abi/libc++abi/*.o)
    file(GLOB_RECURSE LIBABSL_OBJS ${webrtc_BUILD_DIR}/obj/third_party/abseil-cpp/absl/*.o)
    set(PRECOMPILED_OBJS
        # ${LIBCPP_OBJS}
        # ${LIBCPPABI_OBJS}
        # ${LIBABSL_OBJS}
    )
endif()

# set_source_files_properties(${PRECOMPILED_OBJS} PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)

# Disable using the install rpath
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
# set(CMAKE_SKIP_BUILD_PATH FALSE)

################################################################################################################
# WebRTC Sample for DataChannel
#
add_executable(peer_datachannel_client
    src/peer_datachannel_client.cpp
    ${PRECOMPILED_OBJS}
)

# #linking library directory
target_link_directories(peer_datachannel_client PUBLIC
)

target_include_directories(
    peer_datachannel_client PUBLIC
    lib/socket.io-client-cpp/src
    lib/socket.io-client-cpp/lib/asio/asio/include
    lib/abseil-cpp
    # ${webrtc_DIR}/third_party/abseil-cpp
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

target_link_libraries(
    peer_datachannel_client PUBLIC
    dl
    webrtc
    absl::flags_parse
    absl::flags_usage
    # sioclient_tls
    sioclient
    spdlog::spdlog
    # fmt::fmt
)
if (CMAKE_THREAD_LIBS_INIT)
    target_link_libraries(peer_datachannel_client PUBLIC ${CMAKE_THREAD_LIBS_INIT})
else()
    target_link_libraries(peer_datachannel_client PUBLIC pthread)
endif()

if (USE_CUSTOM_LIBCXX)
    message(STATUS "use_custom_libcxx")
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
    )
endif()

################################################################################################################
# WebRTC Sample for Audio
#
add_executable(peer_audio_client
    src/peer_audio_client.cpp
    ${PRECOMPILED_OBJS}
)

# #linking library directory
target_link_directories(peer_audio_client PUBLIC
)

target_include_directories(
    peer_audio_client PUBLIC
    lib/socket.io-client-cpp/src
    lib/socket.io-client-cpp/lib/asio/asio/include
    lib/abseil-cpp
    # ${webrtc_DIR}/third_party/abseil-cpp
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

target_link_libraries(
    peer_audio_client PUBLIC
    dl
    webrtc
    absl::flags_parse
    absl::flags_usage
    # sioclient_tls
    sioclient
    spdlog::spdlog
    ${X11_LIBRARIES} # Add the X11 library
    ${LIBPULSE_LIBRARIES} # Add the PulseAudio library
    rt
)

if (CMAKE_THREAD_LIBS_INIT)
    target_link_libraries(peer_audio_client PUBLIC ${CMAKE_THREAD_LIBS_INIT})
else()
    target_link_libraries(peer_audio_client PUBLIC pthread)
endif()

if (USE_CUSTOM_LIBCXX)
    target_compile_options(
        peer_audio_client PUBLIC
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
    )
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
    ${webrtc_DIR}/test/vcm_capturer.cc
    ${webrtc_DIR}/test/platform_video_capturer.cc
    ${webrtc_DIR}/test/test_video_capturer.cc
    ${PRECOMPILED_OBJS}
    # ${TEST_OBJS}
)

# #linking library directory
target_link_directories(peer_video_client PUBLIC
)

target_include_directories(
    peer_video_client PUBLIC
    lib/socket.io-client-cpp/src
    lib/socket.io-client-cpp/lib/asio/asio/include
    lib/abseil-cpp
    # ${webrtc_DIR}/third_party/abseil-cpp
    include
    ${GTK3_INCLUDE_DIRS} # Add the Gtk3 include directory
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

target_link_libraries(
    peer_video_client PUBLIC
    dl
    webrtc
    absl::flags_parse
    absl::flags_usage
    # sioclient_tls
    sioclient
    spdlog::spdlog
    ${X11_LIBRARIES} # Add the X11 library
    yuv # Add the libyuv library
    ${GLIB_LIBRARIES}
    ${GIO_LIBRARIES}
    ${GTK3_LIBRARIES} # Add the Gtk3 library library
    rt
)

if (CMAKE_THREAD_LIBS_INIT)
    target_link_libraries(peer_video_client PUBLIC ${CMAKE_THREAD_LIBS_INIT})
else()
    target_link_libraries(peer_video_client PUBLIC pthread)
endif()

if (USE_CUSTOM_LIBCXX)
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
    )
else()
    target_compile_options(
        peer_video_client PUBLIC
        -fno-rtti
    )
endif()

################################################################################################################
# WebRTC Sample for Combined Audio and Video and DataChannel
#
file(GLOB_RECURSE TEST_OBJS
    ${webrtc_BUILD_DIR}/obj/test/platform_video_capturer/*.o
    ${webrtc_BUILD_DIR}/obj/test/test_video_capturer/*.o
)

# add_library(video_track_source STATIC IMPORTED)
# set_target_properties(video_track_source PROPERTIES IMPORTED_LOCATION ${webrtc_BUILD_DIR}/obj/pc/libvideo_track_source.a)

add_executable(peer_combined_client
    src/peer_combined_client.cpp
    ${webrtc_DIR}/test/vcm_capturer.cc
    ${webrtc_DIR}/test/platform_video_capturer.cc
    ${webrtc_DIR}/test/test_video_capturer.cc
    ${PRECOMPILED_OBJS}
    # ${TEST_OBJS}
)

# #linking library directory
target_link_directories(peer_combined_client PUBLIC
)

target_include_directories(
    peer_combined_client PUBLIC
    lib/socket.io-client-cpp/src
    lib/socket.io-client-cpp/lib/asio/asio/include
    lib/abseil-cpp
    # ${webrtc_DIR}/third_party/abseil-cpp
    include
)

target_compile_definitions(
    peer_combined_client PUBLIC
    _LIBCPP_ENABLE_SAFE_MODE=1
    _LIBCPP_DISABLE_VISIBILITY_ANNOTATIONS
    _LIBCXXABI_DISABLE_VISIBILITY_ANNOTATIONS
    # _LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE
    # CR_LIBCXX_REVISION=7cf98622abaf832e2d4784889ebc69d5b6fde4d8
    WEBRTC_POSIX
    WEBRTC_LINUX
    # ASIO_STANDALONE
)


target_link_libraries(
    peer_combined_client PUBLIC
    dl
    webrtc
    absl::flags_parse
    absl::flags_usage
    # sioclient_tls
    sioclient
    spdlog::spdlog
    ${X11_LIBRARIES} # Add the X11 library
    ${GLIB_LIBRARIES}
    ${GIO_LIBRARIES}
    rt
)

if (CMAKE_THREAD_LIBS_INIT)
    target_link_libraries(peer_combined_client PUBLIC ${CMAKE_THREAD_LIBS_INIT})
else()
    target_link_libraries(peer_combined_client PUBLIC pthread)
endif()

if (USE_CUSTOM_LIBCXX)
    target_compile_options(
        peer_combined_client PUBLIC
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
        peer_combined_client PUBLIC
        -Werror -fuse-ld=lld-18
        -Wl,--fatal-warnings -Wl,--build-id -fPIC -Wl,-z,noexecstack
        -Wl,-z,relro -Wl,-z,now -Wl,--color-diagnostics -Wl,--undefined-version
        -Wl,--no-call-graph-profile-sort -m64 -no-canonical-prefixes -Wl,--gdb-index -rdynamic -Wl,-z,defs
        -Wl,--as-needed -nostdlib++
        # -Wl,--as-needed -stdlib=libc++
        # --sysroot=${webrtc_DIR}/build/linux/debian_bullseye_amd64-sysroot
    )
    target_link_libraries(
        peer_combined_client PUBLIC
        c++
        c++abi
    )
else()
    target_compile_options(
        peer_combined_client PUBLIC
        -fno-rtti
    )
endif()