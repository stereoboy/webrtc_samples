cmake_minimum_required(VERSION 3.16)

project(sample_build) #Project name

# Add the missing find_package(Threads REQUIRED) command
find_package(Threads REQUIRED)
find_package(X11 REQUIRED)

set(webrtc_DIR ${CMAKE_SOURCE_DIR}/../webrtc-checkout/src)
set(webrtc_BUILD_DIR ${CMAKE_SOURCE_DIR}/../webrtc-checkout/src/out/Default)

#
# https://libcxx.llvm.org/UsingLibcxx.html#using-a-custom-built-libc
#
# set(CMAKE_C_COMPILER ${webrtc_DIR}/third_party/llvm-build/Release+Asserts/bin/clang) # Set C compiler to "clang"
# set(CMAKE_CXX_COMPILER ${webrtc_DIR}/third_party/llvm-build/Release+Asserts/bin/clang++) # Set C++ compiler to "clang++"
set(CMAKE_C_COMPILER clang-17) # Set C compiler to "clang"
set(CMAKE_CXX_COMPILER clang++-17) # Set C++ compiler to "clang++"

# set(CMAKE_LINKER ${CMAKE_SOURCE_DIR}/../src/third_party/llvm-build/Release+Asserts/bin/ld.lld) # Set linker to "clang++"
# set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_LINKER>  <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
# set(CMAKE_CXX_LINK_FLAGS " -L${CMAKE_SOURCE_DIR}/../src/out/Default/obj/ -lwebrtc -Werror -fuse-ld=lld -Wl,--fatal-warnings -Wl,--build-id -fPIC -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now -Wl,--color-diagnostics -Wl,--undefined-version -Wl,--no-call-graph-profile-sort -m64 -no-canonical-prefixes -Wl,--gdb-index -Wl,-z,defs -Wl,--as-needed -nostdlib++ --sysroot=${CMAKE_SOURCE_DIR}/../src/build/linux/debian_bullseye_amd64-sysroot -rdynamic -pie -Wl,--disable-new-dtags")
set(CMAKE_CXX_STANDARD 17) #setting C++ 20 standard
set(CMAKE_CXX_STANDARD_REQUIRED ON) #setting C++
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

# set (CMAKE_CXX_FLAGS " -nostdinc++  -Wl,--as-needed -nostdlib++  -isystem${webrtc_DIR}/third_party/libc++/src/include -isystem${webrtc_DIR}/third_party/libc++abi/src/include")
# set(CMAKE_C_FLAGS)
# set(CMAKE_CXX_FLAGS "-Wno-undefined-bool-conversion -Wno-tautological-undefined-compare -std=c++17 -Wno-trigraphs -gsimple-template-names -fno-exceptions -fno-rtti -nostdinc++ -isystem${webrtc_DIR}/third_party/libc++/src/include -isystem${webrtc_DIR}/third_party/libc++abi/src/include --sysroot=${webrtc_DIR}/build/linux/debian_bullseye_amd64-sysroot -fvisibility-inlines-hidden -Wnon-virtual-dtor -Woverloaded-virtual")
# set(CMAKE_EXE_LINKER_FLAGS "-Werror -fuse-ld=lld -Wl,--fatal-warnings -Wl,--build-id -fPIC -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now -Wl,--color-diagnostics -Wl,--undefined-version -Wl,--no-call-graph-profile-sort -m64 -no-canonical-prefixes -Wl,--gdb-index -rdynamic -Wl,-z,defs -Wl,--as-needed -nostdlib++ --sysroot=${webrtc_DIR}/build/linux/debian_bullseye_amd64-sysroot")
# set(CMAKE_CXX_FLAGS "-Wno-undefined-bool-conversion -Wno-tautological-undefined-compare -std=c++17 -Wno-trigraphs -gsimple-template-names -fno-exceptions -fno-rtti -nostdinc++ -isystem${webrtc_DIR}/third_party/libc++/src/include -isystem${webrtc_DIR}/third_party/libc++abi/src/include --sysroot=${webrtc_DIR}/build/linux/debian_bullseye_amd64-sysroot -fvisibility-inlines-hidden -Wnon-virtual-dtor -Woverloaded-virtual")
# set(CMAKE_EXE_LINKER_FLAGS "-Werror -fuse-ld=lld -Wl,--fatal-warnings -Wl,--build-id -fPIC -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now -Wl,--color-diagnostics -Wl,--undefined-version -Wl,--no-call-graph-profile-sort -m64 -no-canonical-prefixes -Wl,--gdb-index -rdynamic -Wl,-z,defs -Wl,--as-needed -nostdlib++ --sysroot=${webrtc_DIR}/build/linux/debian_bullseye_amd64-sysroot")
#including header files directory
include_directories(
  ${CMAKE_SOURCE_DIR}/../precompiled/webrtc_headers
  ${webrtc_DIR}/third_party/abseil-cpp
  ${webrtc_DIR}/buildtools/third_party/libc++
  ${webrtc_DIR}/third_party/libyuv/include
)

add_library(webrtc STATIC IMPORTED)
set_target_properties(webrtc PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../precompiled/libwebrtc.a)

file(GLOB LIBCPP_OBJS ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/*.o)
file(GLOB LIBCPPABI_OBJS ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++abi/libc++abi/*.o)
set(PREBUILT_OBJS ${LIBCPP_OBJS} ${LIBCPPABI_OBJS})

# set(PREBUILT_OBJS ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/algorithm.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/any.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/atomic.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/barrier.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/bind.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/charconv.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/chrono.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/condition_variable.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/condition_variable_destructor.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/error_category.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/exception.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/directory_iterator.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/filesystem_error.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/operations.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/path.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/functional.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/future.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/hash.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/ios.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/ios.instantiations.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/iostream.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/legacy_pointer_safety.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/locale.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/memory.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/mutex.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/mutex_destructor.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/new_handler.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/new_helpers.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/optional.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/random.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/random_shuffle.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/regex.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/d2fixed.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/d2s.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/f2s.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/shared_mutex.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/stdexcept.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/string.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/strstream.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/system_error.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/thread.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/typeinfo.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/valarray.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/variant.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/vector.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/verbose_abort.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/new.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/directory_entry.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++/filesystem_clock.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++abi/libc++abi/abort_message.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++abi/libc++abi/cxa_aux_runtime.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++abi/libc++abi/cxa_default_handlers.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++abi/libc++abi/cxa_exception.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_pwget https://apt.llvm.org/llvm.sharty/libc++abi/libc++abi/cxa_exception_storage.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++abi/libc++abi/cxa_handlers.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++abi/libc++abi/cxa_personality.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++abi/libc++abi/cxa_vector.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++abi/libc++abi/cxa_virtual.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++abi/libc++abi/fallback_malloc.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++abi/libc++abi/private_typeinfo.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++abi/libc++abi/stdlib_exception.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++abi/libc++abi/stdlib_stdexcept.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++abi/libc++abi/stdlib_typeinfo.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++abi/libc++abi/cxa_guard.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++abi/libc++abi/cxa_demangle.o
#                   ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++abi/libc++abi/cxa_thread_atexit.o
#                   # ${webrtc_BUILD_DIR}/obj/libwebrtc.a || ${webrtc_BUILD_DIR}/obj/build/config/executable_deps.stamp ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++/libc++.stamp ${webrtc_BUILD_DIR}/obj/buildtools/third_party/libc++abi/libc++abi.stamp
# )

set_source_files_properties(${PREBUILT_OBJS} PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)

#building target executablewget https://apt.llvm.org/llvm.sh
add_executable(webrtc_lib_link_test webrtc_lib_link_test.cc ${PREBUILT_OBJS})

# target_compile_features(webrtc_lib_link_test PUBLIC cxx_std_17)

# Disable using the install rpath
# set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
# set(CMAKE_SKIP_BUILD_PATH FALSE)
# set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

# #linking library directory
target_link_directories( webrtc_lib_link_test PUBLIC
    # ${CMAKE_SOURCE_DIR}/../webrtc-checkout/src/out/Custom/obj/
)

target_compile_definitions(
  webrtc_lib_link_test PUBLIC
  _LIBCPP_ENABLE_SAFE_MODE=1
  _LIBCPP_DISABLE_VISIBILITY_ANNOTATIONS
  _LIBCXXABI_DISABLE_VISIBILITY_ANNOTATIONS
  CR_LIBCXX_REVISION=7cf98622abaf832e2d4784889ebc69d5b6fde4d8
  WEBRTC_POSIX
  WEBRTC_LINUX
)

target_compile_options(
  webrtc_lib_link_test PUBLIC
  -Wno-undefined-bool-conversion -Wno-tautological-undefined-compare -std=c++17 -Wno-trigraphs
  -gsimple-template-names
  -fno-exceptions -fno-rtti -nostdinc++
  -isystem${webrtc_DIR}/buildtools/third_party/libc++/trunk/include
  -isystem${webrtc_DIR}/buildtools/third_party/libc++abi/trunk/include
  # --sysroot=${webrtc_DIR}/build/linux/debian_bullseye_amd64-sysroot
  -fvisibility-inlines-hidden -Wnon-virtual-dtor -Woverloaded-virtual
)

target_link_options(
  webrtc_lib_link_test PUBLIC
  -fuse-ld=lld-17
  -Werror
  -Wl,--fatal-warnings -Wl,--build-id -fPIC -Wl,-z,noexecstack
  -Wl,-z,relro -Wl,-z,now -Wl,--color-diagnostics -Wl,--undefined-version
  -Wl,--no-call-graph-profile-sort -m64 -no-canonical-prefixes -Wl,--gdb-index -rdynamic -Wl,-z,defs -Wl,--as-needed -nostdlib++
  # --sysroot=${webrtc_DIR}/build/linux/debian_bullseye_amd64-sysroot
)

target_link_libraries(
    webrtc_lib_link_test PUBLIC
    webrtc
    ${X11_LIBRARIES} # Add the X11 library
    dl
)
if (CMAKE_THREAD_LIBS_INIT)
    target_link_libraries(webrtc_lib_link_test PUBLIC ${CMAKE_THREAD_LIBS_INIT})
else()
    target_link_libraries(webrtc_lib_link_test PUBLIC pthread)
endif()
