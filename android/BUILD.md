## Base Code
* NDK Official Samples
  * https://github.com/android/ndk-samples/tree/3a94e115ced511c9f95f505b273332d53c6b0aca/hello-libs
  * commit-id `3a94e115ced511c9f95f505b273332d53c6b0aca`

## Build `libwebrtc.a`
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

* `arm`
  ```
  src ((HEAD detached at branch-heads/6045))$ gn gen out/Debug_arm --args='target_os="android" target_cpu="arm"'
  Done. Made 7290 targets from 366 files in 591ms


  ```
* `arm64`
  ```
  ```
* `x86`
  ```
  ```
* `x64`
  ```
  ```
