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

# set(CMAKE_CXX_STANDARD 20) // cannot use 20 for socket.io-client-cpp, error: no template named 'result_of' in namespace 'std'
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the missing find_package(Threads REQUIRED) command
find_package(Threads REQUIRED)
# find_package(spdlog REQUIRED)
# find_package(fmt REQUIRED)


include_directories(
  ${webrtc_DIR}
  # ${webrtc_DIR}/third_party/abseil-cpp
  # ${webrtc_DIR}/buildtools/third_party/libc++
  # ${webrtc_DIR}/third_party/libyuv/include
)

add_library(webrtc STATIC IMPORTED)
set_target_properties(webrtc PROPERTIES IMPORTED_LOCATION ${webrtc_BUILD_DIR}/obj/libwebrtc.a)

# file(GLOB LIBCPP_OBJS ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/*.o)
# file(GLOB LIBCPPABI_OBJS ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++abi/libc++abi/*.o)
file(GLOB_RECURSE LIBABSL_OBJS ${webrtc_BUILD_DIR}/obj/third_party/abseil-cpp/absl/*.o)
set(PRECOMPILED_OBJS
    # ${LIBCPP_OBJS}
    # ${LIBCPPABI_OBJS}
    ${LIBABSL_OBJS}
)

# set_source_files_properties(${PRECOMPILED_OBJS} PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)

# #building target executable
add_executable(peer_datachannel_client src/peer_datachannel_client.cpp ${PRECOMPILED_OBJS})
# add_executable(peer_datachannel_client src/peer_datachannel_client.cpp)


# Disable using the install rpath
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
# set(CMAKE_SKIP_BUILD_PATH FALSE)

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
    ${webrtc_DIR}
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
