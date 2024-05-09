cmake_minimum_required(VERSION 3.13)

project(webrtc_lib_link_test) #Project name

# set(CMAKE_C_COMPILER ${CMAKE_SOURCE_DIR}/../src/third_party/llvm-build/Release+Asserts/bin/clang) # Set C compiler to "clang"
# set(CMAKE_CXX_COMPILER ${CMAKE_SOURCE_DIR}/../src/third_party/llvm-build/Release+Asserts/bin/clang++) # Set C++ compiler to "clang++"
# set(CMAKE_LINKER ${CMAKE_SOURCE_DIR}/../src/third_party/llvm-build/Release+Asserts/bin/ld.lld) # Set linker to "clang++"
# set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_LINKER>  <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
# set(CMAKE_CXX_LINK_FLAGS " -L${CMAKE_SOURCE_DIR}/../src/out/Default/obj/ -lwebrtc -Werror -fuse-ld=lld -Wl,--fatal-warnings -Wl,--build-id -fPIC -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now -Wl,--color-diagnostics -Wl,--undefined-version -Wl,--no-call-graph-profile-sort -m64 -no-canonical-prefixes -Wl,--gdb-index -Wl,-z,defs -Wl,--as-needed -nostdlib++ --sysroot=${CMAKE_SOURCE_DIR}/../src/build/linux/debian_bullseye_amd64-sysroot -rdynamic -pie -Wl,--disable-new-dtags")
set(CMAKE_CXX_STANDARD 17) #setting C++ 20 standard
set(CMAKE_CXX_STANDARD_REQUIRED 17) #setting C++
# set (CMAKE_CXX_FLAGS "-lstdc++ -lstdc++fs -Wall -Wextra -Wpedantic ")
# set (CMAKE_CXX_FLAGS " -stdlib=libc++  -isystem ../src/third_party/libc++/src/include -isystem../src/third_party/libc++abi/src/include")
# set (CMAKE_CXX_FLAGS " -stdlib=libc++ ")
# set (CMAKE_CXX_FLAGS " -Wenum-compare-conditional -Wno-c++11-narrowing-const-reference -Wno-undefined-bool-conversion -Wno-tautological-undefined-compare -std=c++17 -Wno-trigraphs -gsimple-template-names -fno-exceptions \
#  -fno-rtti -nostdinc++ -isystem${CMAKE_SOURCE_DIR}/../src/third_party/libc++/src/include -isystem${CMAKE_SOURCE_DIR}/../src/third_party/libc++abi/src/include --sysroot=${CMAKE_SOURCE_DIR}/../src/build/linux/debian_bullseye_amd64-sysroot \
#  -DUSE_UDEV -DUSE_AURA=1 -DUSE_GLIB=1 -DUSE_OZONE=1 -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_GNU_SOURCE -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE -DCR_CLANG_REVISION=\"llvmorg-19-init-8091-gab037c4f-1\" -D_LIBCPP_DISABLE_VISIBILITY_ANNOTATIONS -D_LIBCXXABI_DISABLE_VISIBILITY_ANNOTATIONS -DCR_LIBCXX_REVISION=e3b94d0e5b86883fd77696bf10dc33ba250ba99b -DCR_SYSROOT_KEY=20230611T210420Z-2 -D_DEBUG -DDYNAMIC_ANNOTATIONS_ENABLED=1 -DWEBRTC_ENABLE_PROTOBUF=1 -DWEBRTC_STRICT_FIELD_TRIALS=0 -DWEBRTC_INCLUDE_INTERNAL_AUDIO_DEVICE -DRTC_ENABLE_VP9 -DRTC_DAV1D_IN_INTERNAL_DECODER_FACTORY -DWEBRTC_HAVE_SCTP -DWEBRTC_ENABLE_LIBEVENT -DWEBRTC_ENABLE_AVX2 -DWEBRTC_NON_STATIC_TRACE_EVENT_HANDLERS=1 -DWEBRTC_POSIX -DWEBRTC_LINUX -DABSL_ALLOCATOR_NOTHROW=1 -DABSL_FLAGS_STRIP_NAMES=0 -DLIBYUV_DISABLE_NEON -DLIBYUV_DISABLE_LSX -DLIBYUV_DISABLE_LASX")
# set (CMAKE_CXX_FLAGS " -nostdlib++ ")
# set (CMAKE_CXX_FLAGS " -nostdinc++ -nostdlib++           \
# -isystem ../src/third_party/libc++/src/include \
# -isystem ../src/third_party/libc++abi/src/include \
# -L <install>/lib                  \
# -Wl,-rpath,<install>/lib          \
# -lc++                             \
# test.cpp ")
#including header files directory
include_directories(
  ${CMAKE_SOURCE_DIR}/../src
  ${CMAKE_SOURCE_DIR}/../src/third_party/abseil-cpp
  ${CMAKE_SOURCE_DIR}/../src/buildtools/third_party/libc++
  ${CMAKE_SOURCE_DIR}/../src/third_party/libyuv/include
)

# Add the missing find_package(Threads REQUIRED) command
find_package(Threads REQUIRED)
find_package(X11 REQUIRED)

add_library(webrtc STATIC IMPORTED)
set_target_properties(webrtc PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../src/out/Default/obj/libwebrtc.a)

#building target executable
add_executable(webrtc_lib_link_test webrtc_lib_link_test.cc)

# Disable using the install rpath
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
# set(CMAKE_SKIP_BUILD_PATH FALSE)
# set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

# #linking library directory
target_link_directories( webrtc_lib_link_test PUBLIC
    ${CMAKE_SOURCE_DIR}/../src/out/Default/obj/
)

target_link_libraries(
    webrtc_lib_link_test PUBLIC
    webrtc
    ${CMAKE_THREAD_LIBS_INIT} # Add the Threads library
    ${X11_LIBRARIES} # Add the X11 library
    dl
)
target_compile_definitions(
  webrtc_lib_link_test PUBLIC
  WEBRTC_POSIX
  WEBRTC_LINUX
)
