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
### Build Release Version
* `arm`
  ```
  src ((HEAD detached at branch-heads/6045))$ gn gen out/armeabi-v7a --args='target_os="android" target_cpu="arm"'
  Done. Made 7290 targets from 366 files in 591ms


  ```
* `arm64`
  ```
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