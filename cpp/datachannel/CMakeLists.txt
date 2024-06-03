cmake_minimum_required(VERSION 3.13)

project(peer_datachannel) #Project name

# Add the missing find_package(Threads REQUIRED) command
find_package(Threads REQUIRED)

set(webrtc_DIR ${CMAKE_SOURCE_DIR}/../webrtc-checkout/src)
set(webrtc_BUILD_DIR ${CMAKE_SOURCE_DIR}/../webrtc-checkout/src/out/Default)

# set(CMAKE_C_COMPILER ${webrtc_DIR}/third_party/llvm-build/Release+Asserts/bin/clang) # Set C compiler to "clang"
# set(CMAKE_CXX_COMPILER ${webrtc_DIR}/third_party/llvm-build/Release+Asserts/bin/clang++) # Set C++ compiler to "clang++"
set(CMAKE_C_COMPILER clang-18) # Set C compiler to "clang"
set(CMAKE_CXX_COMPILER clang++-18) # Set C++ compiler to "clang++"

set(CMAKE_CXX_STANDARD 17) #setting C++ 20 standard
set(CMAKE_CXX_STANDARD_REQUIRED ON) #setting C++

include_directories(
  ${webrtc_DIR}
  ${webrtc_DIR}/third_party/abseil-cpp
  ${webrtc_DIR}/buildtools/third_party/libc++
  ${webrtc_DIR}/third_party/libyuv/include
)

add_library(webrtc STATIC IMPORTED)
set_target_properties(webrtc PROPERTIES IMPORTED_LOCATION ${webrtc_BUILD_DIR}/obj/libwebrtc.a)

file(GLOB LIBCPP_OBJS ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/*.o)
file(GLOB LIBCPPABI_OBJS ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++abi/libc++abi/*.o)
file(GLOB_RECURSE LIBABSL_OBJS ${webrtc_BUILD_DIR}/obj/third_party/abseil-cpp/absl/*.o)
set(PREBUILT_OBJS
    ${LIBCPP_OBJS}
    ${LIBCPPABI_OBJS}
    # ${LIBABSL_OBJS}
)

set_source_files_properties(${PREBUILT_OBJS} PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)

# file(GLOB_RECURSE LIBCPP_SRCS     ${webrtc_DIR}/third_party/libc++/src/src/*.cpp)
# file(GLOB_RECURSE LIBCPPABI_SRCS  ${webrtc_DIR}/third_party/libc++abi/src/src/*.cpp)
# # file(GLOB_RECURSE LIBABSL_SRCS    ${webrtc_BUILD_DIR}/obj/third_party/abseil-cpp/absl/*.o)
# set(PRECOMPILED_SRCS
#     ${LIBCPP_SRCS}
#     ${LIBCPPABI_SRCS}
#     # ${LIBABSL_OBJS}
# )
# message (PRECOMPILED_SRCS: ${PRECOMPILED_SRCS})
# add_library(c++_custom_obj OBJECT IMPORTED)
# set_property(TARGET c++_custom_obj PROPERTY IMPORTED_OBJECTS ${PREBUILT_OBJS})

# add_library(c++_custom SHARED ${PRECOMPILED_SRCS})
# # target_link_libraries(c++_custom PUBLIC $<TARGET_OBJECTS:c++_custom_obj>)

# target_compile_definitions(
#   c++_custom PUBLIC
#   _LIBCPP_ENABLE_SAFE_MODE=1
#   _LIBCPP_DISABLE_VISIBILITY_ANNOTATIONS
#   _LIBCXXABI_DISABLE_VISIBILITY_ANNOTATIONS
#   CR_LIBCXX_REVISION=7cf98622abaf832e2d4784889ebc69d5b6fde4d8
#   # _LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE
#   WEBRTC_POSIX
#   WEBRTC_LINUX
# )
# target_include_directories(c++_custom PUBLIC
#     ${webrtc_DIR}/third_party/libc++/src/include
#     ${webrtc_DIR}/third_party/libc++abi/src/include
#     # ${webrtc_DIR}/third_party/abseil-cpp
# )

# add_library(c++_custom INTERFACE IMPORTED)
# set_target_properties(c++_custom PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../llvm-project/build/lib/libc++.so)

# add_library(c++abi_custom INTERFACE IMPORTED)
# set_target_properties(c++abi_custom PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../llvm-project/build/lib/libc++abi.so)



#building target executable
# add_executable(peer_datachannel peer_datachannel.cpp ${PREBUILT_OBJS})
add_executable(peer_datachannel peer_datachannel.cpp)

target_link_directories(peer_datachannel PUBLIC
${CMAKE_SOURCE_DIR}/../llvm-project/build/lib/
)
target_include_directories(peer_datachannel PUBLIC
# ${CMAKE_SOURCE_DIR}/../llvm-project/build/include/c++/v1
)

# Disable using the install rpath
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
# set(CMAKE_SKIP_BUILD_PATH FALSE)

# #linking library directory
target_link_directories( peer_datachannel PUBLIC
    # ${CMAKE_SOURCE_DIR}/../webrtc-checkout/src/out/Default/obj/
    # ${CMAKE_SOURCE_DIR}/../webrtc-checkout/src/build/linux/debian_bullseye_amd64-sysroot/usr/lib/gcc/x86_64-linux-gnu/10/
)
target_include_directories(
    peer_datachannel PUBLIC
    # /usr/include
)

target_compile_definitions(
  peer_datachannel PUBLIC
  _LIBCPP_ENABLE_SAFE_MODE=1
  _LIBCPP_DISABLE_VISIBILITY_ANNOTATIONS
  _LIBCXXABI_DISABLE_VISIBILITY_ANNOTATIONS
  CR_LIBCXX_REVISION=7cf98622abaf832e2d4784889ebc69d5b6fde4d8
  _LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_NONE
  WEBRTC_POSIX
  WEBRTC_LINUX
)

target_compile_options(
  peer_datachannel PUBLIC
  -Wno-undefined-bool-conversion -Wno-tautological-undefined-compare -std=c++17 -Wno-trigraphs -gsimple-template-names -fno-exceptions -fno-rtti
  -nostdinc++
  # -stdlib=libc++
  -isystem${webrtc_DIR}/third_party/libc++/src/include
  -isystem${webrtc_DIR}/third_party/libc++abi/src/include
  # --sysroot=${webrtc_DIR}/build/linux/debian_bullseye_amd64-sysroot
  -fvisibility-inlines-hidden -Wnon-virtual-dtor -Woverloaded-virtual
)

target_link_options(
  peer_datachannel PUBLIC
  -Werror -fuse-ld=lld-18 -Wl,--fatal-warnings -Wl,--build-id -fPIC -Wl,-z,noexecstack
  -Wl,-z,relro -Wl,-z,now -Wl,--color-diagnostics -Wl,--undefined-version
  -Wl,--no-call-graph-profile-sort -m64 -no-canonical-prefixes -Wl,--gdb-index -rdynamic -Wl,-z,defs
  -Wl,--as-needed -nostdlib++
  # -Wl,--as-needed -stdlib=libc++
  # --sysroot=${webrtc_DIR}/build/linux/debian_bullseye_amd64-sysroot
)

target_link_libraries(
    peer_datachannel PUBLIC
    webrtc
    dl
    c++
    c++abi
)
if (CMAKE_THREAD_LIBS_INIT)
    target_link_libraries(peer_datachannel PUBLIC ${CMAKE_THREAD_LIBS_INIT})
else()
    target_link_libraries(peer_datachannel PUBLIC pthread)
endif()
