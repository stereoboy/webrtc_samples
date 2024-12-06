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
### Download `M116`(`5845`)
* https://webrtc.github.io/webrtc-org/native-code/android/
  ```
  mkdir webrtc_android_M116
  cd webrtc_android_M116
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

  [0:21:26] Still working on:
  [0:21:26]   src/third_party

  [0:21:27] Still working on:
  [0:21:27]   src/third_party
  Syncing projects: 100% (202/202), done.
  Running: git config --add remote.origin.fetch '+refs/tags/*:refs/tags/*'
  Running: git config diff.ignoreSubmodules dirty

  real	28m22.761s
  user	34m59.200s
  sys	3m37.664s
  ```
  ```
  cd src
  git checkout branch-heads/5845
  src ((HEAD detached at branch-heads/5845))$ time gclient sync -D
  Running hooks: 100% (31/31), done.

  real    9m32.739s
  user    16m56.937s
  sys     3m1.530s
  ```


### Code Modifications
* References
  * https://bugs.chromium.org/p/webrtc/issues/detail?id=13535&no_tracker_redirect=1

  ```diff
  src/build ((HEAD detached at 6c0e0e0c8))$ git diff
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
  src/buildtools ((HEAD detached at 3739a36))$ git diff
  diff --git a/third_party/libunwind/BUILD.gn b/third_party/libunwind/BUILD.gn
  index a8287bf..a948450 100644
  --- a/third_party/libunwind/BUILD.gn
  +++ b/third_party/libunwind/BUILD.gn
  @@ -20,7 +20,7 @@ config("libunwind_config") {
  }

  source_set("libunwind") {
  -  visibility = []
  +  visibility = ["//build/config:common_deps"]
    if (is_fuchsia) {
      visibility += [ "//buildtools/third_party/libc++abi" ]
    } else if (is_android) {

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