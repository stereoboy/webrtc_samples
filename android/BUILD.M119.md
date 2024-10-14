## Chromium WebRTC Implementation
### References
* Official Development Document
  * https://webrtc.github.io/webrtc-org/native-code/development/
  * https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up
* Official Branch Details
  * https://chromiumdash.appspot.com/branches

## Base Code
* NDK Official Samples
  * https://github.com/android/ndk-samples/tree/3a94e115ced511c9f95f505b273332d53c6b0aca/hello-libs
  * commit-id `3a94e115ced511c9f95f505b273332d53c6b0aca`

## Build `libwebrtc.a`
### Download `M119`(`6045`)
* https://webrtc.github.io/webrtc-org/native-code/android/
  ```
  mkdir webrtc_android
  cd webrtc_android
  webrtc_android$ time fetch --nohooks webrtc_android
  Running: gclient root
  Running: gclient config --spec 'solutions = [
  {
      "name": "src",
      "url": "https://webrtc.googlesource.com/src.git",
      "deps_file": "DEPS",
      "managed": False,
      "custom_deps": {},
  },
  ]
  target_os = ["android", "unix"]
  '
  Running: gclient sync --nohooks --with_branch_heads
  ...

  [0:11:24]   src/third_party
  Syncing projects: 100% (201/201), done.
  Running: git config --add remote.origin.fetch '+refs/tags/*:refs/tags/*'
  Running: git config diff.ignoreSubmodules dirty

  real    16m16.835s
  user    32m22.748s
  sys     3m13.018s

  cd src
  git checkout branch-heads/6045
  src ((HEAD detached at branch-heads/6045))$ time gclient sync -D
  Running hooks: 100% (31/31), done.

  real    9m32.739s
  user    16m56.937s
  sys     3m1.530s
  ```
### Code Modifications
* References
  * https://bugs.chromium.org/p/webrtc/issues/detail?id=13535&no_tracker_redirect=1

  ```diff
  src/build/config ((HEAD detached at bc10f9ffb))$ diff --color --unified ./BUILD.gn.backup ./BUILD.gn
  --- ./BUILD.gn.backup   2024-10-02 18:20:33.977610929 +0900
  +++ ./BUILD.gn  2024-10-02 18:20:39.649664798 +0900
  @@ -240,6 +240,8 @@

    if (use_custom_libcxx) {
      public_deps += [ "//buildtools/third_party/libc++" ]
  +  } else {
  +    public_deps += [ "//buildtools/third_party/libunwind" ]
    }

    if (use_afl) {
  ```

  ```diff
  src/buildtools/third_party/libunwind ((HEAD detached at 50c3489))$ diff --color --unified ./BUILD.gn.backup ./BUILD.gn
  --- ./BUILD.gn.backup   2024-10-02 18:26:56.236298353 +0900
  +++ ./BUILD.gn  2024-10-02 18:27:02.676603007 +0900
  @@ -21,7 +21,8 @@

  # TODO(crbug.com/1458042): Move this build file to third_party/libc++/BUILD.gn once submodule migration is done
  source_set("libunwind") {
  -  visibility = [ "//buildtools/third_party/libc++abi" ]
  +  visibility = ["//build/config:common_deps"]
  +  visibility += [ "//buildtools/third_party/libc++abi" ]
    if (is_android) {
      visibility += [ "//services/tracing/public/cpp" ]
    }
  ```

#### atomic Issue

* https://github.com/flutter/flutter/issues/75348
* https://github.com/flutter/buildroot/commit/bc399c09daea4df5ef557d274062ec01292ec906#diff-8a360dce0fe5c3b40bce86e8c3c91700626b9114bc8cfd4bc9a42089ee1ad4bfR4

* Error Messages from Android Studio when building Android App like PeerDataChannelClient
  ```
  [3/3] Linking CXX shared library /home/wom/jylee/webrtc_samples/android/peer_datachannel_client/build/intermediates/cmake/debug/obj/arm64-v8a/libhello-libs.so
  FAILED: /home/wom/jylee/webrtc_samples/android/peer_datachannel_client/build/intermediates/cmake/debug/obj/arm64-v8a/libhello-libs.so
  : && /home/wom/android-sdk/ndk/21.4.7075529/toolchains/llvm/prebuilt/linux-x86_64/bin/clang++ --target=aarch64-none-linux-android23 --gcc-toolchain=/home/wom/android-sdk/ndk/21.4.7075529/toolchains/llvm/prebuilt/linux-x86_64 --sysroot=/home/wom/android-sdk/ndk/21.4.7075529/toolchains/llvm/prebuilt/linux-x86_64/sysroot -fPIC -g -DANDROID -fdata-sections -ffunction-sections -funwind-tables -fstack-protector-strong -no-canonical-prefixes -D_FORTIFY_SOURCE=2 -Wformat -Werror=format-security   -O0 -fno-limit-debug-info  -Wl,--exclude-libs,libgcc.a -Wl,--exclude-libs,libgcc_real.a -Wl,--exclude-libs,libatomic.a -static-libstdc++ -Wl,--build-id -Wl,--fatal-warnings -Wl,--no-undefined -Qunused-arguments -shared -Wl,-soname,libhello-libs.so -o /home/wom/jylee/webrtc_samples/android/peer_datachannel_client/build/intermediates/cmake/debug/obj/arm64-v8a/libhello-libs.so CMakeFiles/hello-libs.dir/hello-libs.cpp.o CMakeFiles/hello-libs.dir/peer_datachannel_client.cpp.o  -landroid /home/wom/jylee/webrtc_samples/android/peer_datachannel_client/src/main/cpp/../../../../distribution/lib/arm64-v8a/libsioclient.a /home/wom/Data/webrtc_android//src/out/arm64-v8a/obj/libwebrtc.a -llog -latomic -lm && :
  /home/wom/Data/webrtc_android//src/out/arm64-v8a/obj/libwebrtc.a(peer_connection_interface.o): In function int std::__ndk1::__cxx_atomic_fetch_sub<int>(std::__ndk1::__cxx_atomic_base_impl<int>*, int, std::__ndk1::memory_order)':
  ./../../../../../android-sdk/ndk/21.4.7075529/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include/c++/v1/atomic:1036: undefined reference to __aarch64_ldadd4_acq_rel'
  /home/wom/Data/webrtc_android//src/out/arm64-v8a/obj/libwebrtc.a(peer_connection_interface.o): In function int std::__ndk1::__cxx_atomic_fetch_add<int>(std::__ndk1::__cxx_atomic_base_impl<int>*, int, std::__ndk1::memory_order)':
  ./../../../../../android-sdk/ndk/21.4.7075529/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include/c++/v1/atomic:1014: undefined reference to __aarch64_ldadd4_relax'
  /home/wom/Data/webrtc_android//src/out/arm64-v8a/obj/libwebrtc.a(rtc_certificate.o): In function int std::__ndk1::__cxx_atomic_fetch_add<int>(std::__ndk1::__cxx_atomic_base_impl<int>*, int, std::__ndk1::memory_order)':
  ./../../../../../android-sdk/ndk/21.4.7075529/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include/c++/v1/atomic:1014: undefined reference to __aarch64_ldadd4_relax'
  /home/wom/Data/webrtc_android//src/out/arm64-v8a/obj/libwebrtc.a(copy_on_write_buffer.o): In function int std::__ndk1::__cxx_atomic_fetch_add<int>(std::__ndk1::__cxx_atomic_base_impl<int>*, int, std::__ndk1::memory_order)':
  ./../../../../../android-sdk/ndk/21.4.7075529/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include/c++/v1/atomic:1014: undefined reference to __aarch64_ldadd4_relax'
  ./../../../../../android-sdk/ndk/21.4.7075529/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include/c++/v1/atomic:1014: undefined reference to __aarch64_ldadd4_relax'
  ./../../../../../android-sdk/ndk/21.4.7075529/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include/c++/v1/atomic:1014: undefined reference to __aarch64_ldadd4_relax'
  ```
* Solution
  ```diff
  src/build/config/android ((HEAD detached at bc10f9ffb))$ diff --color -u ./BUILD.gn.backup ./BUILD.gn
  --- ./BUILD.gn.backup   2024-10-11 20:35:04.270756362 +0900
  +++ ./BUILD.gn  2024-10-11 20:35:06.626717670 +0900
  @@ -22,6 +22,15 @@
      "-fno-short-enums",
    ]

  +  #
  +  # references
  +  #  - https://github.com/flutter/flutter/issues/75348
  +  #  - https://github.com/flutter/buildroot/commit/bc399c09daea4df5ef557d274062ec01292ec906#diff-8a360dce0fe5c3b40bce86e8c3c91700626b9114bc8cfd4bc9a42089ee1ad4bfR426
  +  #
  +  if (target_cpu == "arm64") {
  +    cflags += [ "-mno-outline-atomics" ]
  +  }
  +
    defines = [
      "ANDROID",


  ```
### Build Release Version
* `arm`
  ```
  src ((HEAD detached at branch-heads/6045))$ gn gen out/armeabi-v7a --args='is_debug=false use_custom_libcxx=false target_os="android" target_cpu="arm" android_ndk_version = "r21d" android_ndk_major_version=21 default_min_sdk_version=23'
  Done. Made 7287 targets from 362 files in 890ms
  src ((HEAD detached at branch-heads/6045))$ time ninja -C out/armeabi-v7a/ webrtc
  ninja: Entering directory `out/armeabi-v7a/'
  [4343/4343] AR obj/libwebrtc.a

  real    3m30.842s
  user    35m59.086s
  sys     2m15.902s
  ```
* `arm64`
  ```
  src ((HEAD detached at branch-heads/6045))$ gn gen out/arm64-v8a --args='is_debug=false use_custom_libcxx=false target_os="android" target_cpu="arm64" android_ndk_version = "r21d" android_ndk_major_version=21 default_min_sdk_version=23 android_ndk_root="/home/wom/android-sdk/ndk/21.4.7075529"'
  Done. Made 7287 targets from 362 files in 662ms
  src ((HEAD detached at branch-heads/6045))$ time ninja -C out/arm64-v8a/ webrtc
  ninja: Entering directory `out/arm64-v8a/'
  [4310/4310] AR obj/libwebrtc.a

  real    3m33.807s
  user    36m7.673s
  sys     2m17.012s
  ```
* `x86`
  ```
  src ((HEAD detached at branch-heads/6045))$ gn gen out/x86 --args='is_debug=false use_custom_libcxx=false target_os="android" target_cpu="x86" android_ndk_version = "r21d" android_ndk_major_version=21 default_min_sdk_version=23'
  src ((HEAD detached at branch-heads/6045))$ time ninja -C out/x86 webrtc
  ninja: Entering directory `out/x86'
  [4716/4716] AR obj/libwebrtc.a

  real	4m51.747s
  user	48m31.234s
  sys	3m9.728s
  ```
* `x64`
  ```
  src ((HEAD detached at branch-heads/6045))$ gn gen out/x86_64 --args='is_debug=false use_custom_libcxx=false target_os="android" target_cpu="x64" android_ndk_version = "r21d" android_ndk_major_version=21 default_min_sdk_version=23'
  Done. Made 7305 targets from 364 files in 660ms
  src ((HEAD detached at branch-heads/6045))$ time ninja -C out/x86_64/ webrtc
  ninja: Entering directory `out/x86_64/'
  [4582/4582] AR obj/libwebrtc.a

  real    3m53.798s
  user    40m1.787s
  sys     2m20.757s
  ```


## Copy headers for Android (for Android Studio 4.2.2 indexing)
```bash
time bash ./copy_headers.M119.sh <path>/webrtc_android/src/ <path>/webrtc_android/include

...
+ read -r file
++ dirname video/frame_decode_timing.h
+ RELATIVE_DIR=video
+ mkdir -p /home/wom/Data/webrtc_android/include/video
+ cp /home/wom/Data/webrtc_android/src//video/frame_decode_timing.h /home/wom/Data/webrtc_android/include/video/
+ read -r file
+ echo 'Header files copied successfully from the selected directories.'
Header files copied successfully from the selected directories.

real	0m23.685s
user	0m13.010s
sys	0m10.730s
```
```bash
webrtc_android$ tree include/ -d -L 1
include/
├── api
├── audio
├── base
├── call
├── common_audio
├── common_video
├── logging
├── media
├── modules
├── net
├── p2p
├── pc
├── rtc_base
├── rtc_tools
├── system_wrappers
├── third_party
└── video

17 directories
```

## Android Studio Settings
``` diff
diff --git a/android/local.properties b/android/local.properties
index dada0c3..f987210 100644
--- a/android/local.properties
+++ b/android/local.properties
@@ -4,5 +4,7 @@
 # Location of the SDK. This is only used by Gradle.
 # For customization when using a Version Control System, please read the
 # header note.
-#Fri Sep 20 17:04:32 KST 2024
+#Mon Sep 23 18:15:26 KST 2024
 sdk.dir=/home/wom/android-sdk
+webrtc.dir=/home/wom/Data/webrtc_android/
```
* Block the following lines Temporarily to sync gradle
```diff
diff --git a/android/peer_datachannel_client/src/main/cpp/CMakeLists.txt b/android/peer_datachannel_client/src/main/cpp/CMakeLists.txt
index a60f5ed..bb9efac 100644
--- a/android/peer_datachannel_client/src/main/cpp/CMakeLists.txt
+++ b/android/peer_datachannel_client/src/main/cpp/CMakeLists.txt
@@ -17,11 +17,11 @@

 cmake_minimum_required(VERSION 3.4.1)

-set(webrtc_out_DIR ${WEBRTC_DIR}/src/out/${ANDROID_ABI})
-set(webrtc_src_DIR ${WEBRTC_DIR}/src)
-add_library(lib_webrtc STATIC IMPORTED)
-set_target_properties(lib_webrtc PROPERTIES IMPORTED_LOCATION
-        ${webrtc_out_DIR}/obj/libwebrtc.a)
+#set(webrtc_out_DIR ${WEBRTC_DIR}/src/out/${ANDROID_ABI})
+#set(webrtc_src_DIR ${WEBRTC_DIR}/src)
+#add_library(lib_webrtc STATIC IMPORTED)
+#set_target_properties(lib_webrtc PROPERTIES IMPORTED_LOCATION
+#        ${webrtc_out_DIR}/obj/libwebrtc.a)

 # configure import libs
 set(distribution_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../distribution)

```