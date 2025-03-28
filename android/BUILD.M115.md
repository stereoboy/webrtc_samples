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
### Download `M115`(`5790`)
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
  ```
  ```
  cd src
  git checkout branch-heads/5790
  src ((HEAD detached at branch-heads/5790))$ time gclient sync -D
  Running hooks: 100% (31/31), done.

  real    9m32.739s
  user    16m56.937s
  sys     3m1.530s
  ```
```
time fetch --nohooks webrtc_android
webrtc_android$: command not found
wom@wom:~/Data/webrtc_android_$ time fetch --nohooks webrtc_android
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

________ running 'git -c core.deltaBaseCacheLimit=2g clone --no-checkout --progress https://webrtc.googlesource.com/src.git /home/wom/Data/webrtc_android_/_gclient_src_5e39fg33' in '/home/wom/Data/webrtc_android_'
Cloning into '/home/wom/Data/webrtc_android_/_gclient_src_5e39fg33'...
remote: Finding sources: 100% (62/62)
remote: Total 459613 (delta 333165), reused 459590 (delta 333165)
Receiving objects: 100% (459613/459613), 398.99 MiB | 32.73 MiB/s, done.
Resolving deltas: 100% (333165/333165), done.
Syncing projects:   4% ( 9/200) src/examples/androidtests/third_party/gradle
[0:02:50] Still working on:
[0:02:50]   src/third_party
[0:02:50]   src/tools

[0:03:00] Still working on:
[0:03:00]   src/third_party
[0:03:00]   src/tools

[0:03:10] Still working on:
[0:03:10]   src/third_party
[0:03:10]   src/tools

[0:03:15] Still working on:
[0:03:15]   src/third_party
[0:03:15]   src/tools
Syncing projects:   6% (13/200) src/tools/resultdb:infra/tools/result_adapter/${platform}
[0:04:16] Still working on:
[0:04:16]   src/third_party

[0:04:26] Still working on:
[0:04:26]   src/third_party

[0:04:36] Still working on:
[0:04:36]   src/third_party

[0:04:46] Still working on:
[0:04:46]   src/third_party

[0:04:56] Still working on:
[0:04:56]   src/third_party

[0:05:06] Still working on:
[0:05:06]   src/third_party

[0:05:17] Still working on:
[0:05:17]   src/third_party

[0:05:27] Still working on:
[0:05:27]   src/third_party

[0:05:37] Still working on:
[0:05:37]   src/third_party

[0:05:47] Still working on:
[0:05:47]   src/third_party

[0:05:57] Still working on:
[0:05:57]   src/third_party

[0:06:07] Still working on:
[0:06:07]   src/third_party

[0:06:17] Still working on:
[0:06:17]   src/third_party

[0:06:27] Still working on:
[0:06:27]   src/third_party

[0:06:37] Still working on:
[0:06:37]   src/third_party

[0:06:47] Still working on:
[0:06:47]   src/third_party

[0:06:57] Still working on:
[0:06:57]   src/third_party

[0:07:08] Still working on:
[0:07:08]   src/third_party

[0:07:18] Still working on:
[0:07:18]   src/third_party

[0:07:28] Still working on:
[0:07:28]   src/third_party

[0:07:38] Still working on:
[0:07:38]   src/third_party
Syncing projects: 100% (202/202), done.
Running: git config --add remote.origin.fetch '+refs/tags/*:refs/tags/*'
Running: git config diff.ignoreSubmodules dirty

real	11m22.222s
user	31m30.157s
sys	3m9.673s
```
### Code Modifications
* References
  * https://bugs.chromium.org/p/webrtc/issues/detail?id=13535&no_tracker_redirect=1

  ```diff
  src/build ((HEAD detached at bc10f9ffb))$ git diff .
  diff --git a/config/BUILD.gn b/config/BUILD.gn
  index 53511ac36..572ce7965 100644
  --- a/config/BUILD.gn
  +++ b/config/BUILD.gn
  @@ -240,6 +240,8 @@ group("common_deps") {

    if (use_custom_libcxx) {
      public_deps += [ "//buildtools/third_party/libc++" ]
  +  } else {
  +    public_deps += [ "//buildtools/third_party/libunwind" ]
    }

    if (use_afl) {
  ```

  ```diff
  diff --git a/third_party/libunwind/BUILD.gn b/third_party/libunwind/BUILD.gn
  index ff1517d..11a6b7b 100644
  --- a/third_party/libunwind/BUILD.gn
  +++ b/third_party/libunwind/BUILD.gn
  @@ -21,7 +21,8 @@ config("libunwind_config") {

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
  diff --git a/config/android/BUILD.gn b/config/android/BUILD.gn
  index 9e481132f..4c524a3b4 100644
  --- a/config/android/BUILD.gn
  +++ b/config/android/BUILD.gn
  @@ -21,6 +21,16 @@ config("compiler") {
      "-ffunction-sections",
      "-fno-short-enums",
    ]
  +
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
  src ((HEAD detached at branch-heads/5790))$ gn gen out/arm64-v8a --args='is_debug=false use_custom_libcxx=false target_os="android" target_cpu="arm64" android_ndk_version = "r21d" android_ndk_major_version=21 default_min_sdk_version=23'
  Done. Made 7114 targets from 359 files in 619ms
  src ((HEAD detached at branch-heads/5790))$ time ninja -C out/arm64-v8a webrtc
  ninja: Entering directory `out/arm64-v8a'
  [4191/4191] AR obj/libwebrtc.a

  real    3m18.811s
  user    33m0.662s
  sys     2m4.274s
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

### Build Debug Version
* `arm64`
  ```
  src ((HEAD detached at branch-heads/6045))$ gn gen out/arm64-v8a_debug --args='use_custom_libcxx=false target_os="android" target_cpu="arm64" android_ndk_version = "r21d" android_ndk_major_version=21 default_min_sdk_version=23 android_ndk_root="/home/wom/android-sdk/ndk/21.4.7075529"'
  Done. Made 7287 targets from 362 files in 631ms
  src ((HEAD detached at branch-heads/5790))$ time ninja -C out/arm64-v8a_debug/ webrtc
  ninja: Entering directory `out/arm64-v8a_debug/'
  [4191/4191] AR obj/libwebrtc.a

  real    3m15.234s
  user    32m3.908s
  sys     2m7.606s
  ```

## Copy headers for Android (for Android Studio 4.2.2 indexing)
```bash
bash copy_headers.M115.sh <path>/webrtc_android_M115/src/ <path>/webrtc_android_M115/include
Directory: /home/wom/Data/webrtc_android_M115/src//api
Directory: /home/wom/Data/webrtc_android_M115/src//audio
Directory: /home/wom/Data/webrtc_android_M115/src//base
Directory: /home/wom/Data/webrtc_android_M115/src//call
Directory: /home/wom/Data/webrtc_android_M115/src//common_audio
Directory: /home/wom/Data/webrtc_android_M115/src//common_video
Directory: /home/wom/Data/webrtc_android_M115/src//experiments
Directory: /home/wom/Data/webrtc_android_M115/src//infra
Directory: /home/wom/Data/webrtc_android_M115/src//logging
Directory: /home/wom/Data/webrtc_android_M115/src//media
Directory: /home/wom/Data/webrtc_android_M115/src//modules
Directory: /home/wom/Data/webrtc_android_M115/src//net
Directory: /home/wom/Data/webrtc_android_M115/src//p2p
Directory: /home/wom/Data/webrtc_android_M115/src//pc
Directory: /home/wom/Data/webrtc_android_M115/src//resources
Directory: /home/wom/Data/webrtc_android_M115/src//rtc_base
Directory: /home/wom/Data/webrtc_android_M115/src//rtc_tools
Directory: /home/wom/Data/webrtc_android_M115/src//system_wrappers
Directory: /home/wom/Data/webrtc_android_M115/src//third_party/abseil-cpp
Directory: /home/wom/Data/webrtc_android_M115/src//video
Directory: /home/wom/Data/webrtc_android_M115/src//sdk
Header files copied successfully from the selected directories.
```
```bash
webrtc_android_M115$ tree include/ -d -L 1
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
+webrtc.dir=/home/wom/Data/webrtc_android_M115/
```