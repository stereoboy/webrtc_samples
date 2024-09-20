
# WebRTC C++ Native (NVIDIA Jetpack R36.3.0)

## Base Libraries
* Libraries to build from source

| Name | Description | Version | Size |
| --- | --- | --- | --- |
| WebRTC | chromium c++ implementation | commit id `d0c86830d00d6aa4608cd6f9970352e583f16308` | 22 G |
| ~~llvm-project~~ | ~~for libc++abi~~ | ~~commit id `a86df48c38e8138ad67050967cef63548bffb230`~~ | 4.5 G |
| llvm-project | for libc++ | commit id `c08d4ad25cf3f335e9b2e7b1b149eb1b486868f1` | 4.5 G |

* Prebuilt libraries

| Name | Description | Version | Size |
| --- | --- | --- | --- |
| llvm | for clang, lld | 18 |  |


### NVIDIA precompiled `libwebrtc.a`
* Hardware Acceleration in the WebRTC Framework
  * https://docs.nvidia.com/jetson/archives/r36.3/DeveloperGuide/SD/HardwareAccelerationInTheWebrtcFramework.html
  * https://developer.nvidia.com/embedded/jetson-linux-r363
* "Hello,how can i use libwebrtc 36.3 hardware acceleration"
  * https://forums.developer.nvidia.com/t/hello-how-can-i-use-libwebrtc-36-3-hardware-acceleration/297562

* Sample Source `webrtc_argus_camera_app` from `Driver Package (BSP) Sources`
  * https://developer.nvidia.com/downloads/embedded/l4t/r36_release_v3.0/sources/public_sources.tbz2


### Download Prebuilt `libwebrtc.a`

* `public_sources.tbz2` from `Driver Package (BSP) Sources`
  * https://developer.nvidia.com/downloads/embedded/l4t/r36_release_v3.0/sources/public_sources.tbz2

```
tar xjvf public_sources.tbz2
cd Linux_for_Tegra/source
tar xjvf webrtc_argus_camera_app_src.tbz2
 cp ./Linux_for_Tegra/source/webrtc_argus_camera_app/prebuilts/aarch64/libwebrtc.a <base>/webrtc_samples/cpp/prebuilts/
cp ./Linux_for_Tegra/source/webrtc_argus_camera_app/3rdparty/webrtc_headers_2023-06-23/src  <base>/webrtc_samples/cpp/prebuilts/webrtc_headers -rf
```


### clang v17
* Install clang on Ubuntu
  * https://apt.llvm.org/
  * https://ubuntuhandbook.org/index.php/2023/09/how-to-install-clang-17-or-16-in-ubuntu-22-04-20-04/
  ```
  wget https://apt.llvm.org/llvm.sh
  chmod +x llvm.sh
  # sudo ./llvm.sh <version number>
  sudo ./llvm.sh 17

  ```
* https://libcxx.llvm.org/UsingLibcxx.html#using-a-custom-built-libc

### WebRTC C++
```
cd cpp
mkdir webrtc-checkout
cd webrtc-checkout
fetch --nohooks webrtc    // about 10 minutes
gclient sync              // about 5 minutes
```
* Size: About 30G
* checkout commit-id `d0c86830d00d6aa4608cd6f9970352e583f16308`
    ```
    cd src
    git checkout d0c86830d00d6aa4608cd6f9970352e583f16308
    gclient sync -D
    ```
* modify code
    ```diff
    diff --git a/rtc_tools/rtc_event_log_to_text/converter.cc b/rtc_tools/rtc_event_log_to_text/converter.cc
    index 6bd3458a6f..4293710f7b 100644
    --- a/rtc_tools/rtc_event_log_to_text/converter.cc
    +++ b/rtc_tools/rtc_event_log_to_text/converter.cc
    @@ -175,7 +175,7 @@ bool Convert(std::string inputfile,
    auto bwe_probe_failure_handler =
        [&](const LoggedBweProbeFailureEvent& event) {
            fprintf(output, "BWE_PROBE_FAILURE %" PRId64 " id=%d reason=%d\n",
    -                event.log_time_ms(), event.id, event.failure_reason);
    +                event.log_time_ms(), event.id, static_cast<int>(event.failure_reason));
        };

    auto bwe_probe_success_handler =
    @@ -209,7 +209,7 @@ bool Convert(std::string inputfile,
    auto dtls_transport_state_handler =
        [&](const LoggedDtlsTransportState& event) {
            fprintf(output, "DTLS_TRANSPORT_STATE %" PRId64 " state=%d\n",
    -                event.log_time_ms(), event.dtls_transport_state);
    +                event.log_time_ms(), static_cast<int>(event.dtls_transport_state));
        };

    auto dtls_transport_writable_handler =
    ```
* Build
  * Option `clang_use_chrome_plugins=false`
    * https://github.com/CoatiSoftware/Sourcetrail/issues/852
  * Release version
    ```
    gn gen out/Release_use_custom_libcxx --args="is_debug=false clang_base_path=\"/lib/llvm-17/\" clang_use_chrome_plugins=false"
    Done. Made 1714 targets from 293 files in 951ms

    ninja -C out/Release_use_custom_libcxx                            // about 22 minutes
    ninja: Entering directory `out/Release_use_custom_libcxx'
    [6750/6750] STAMP obj/default.stamp
    ```
    ```
    ninja -C out/Release_use_custom_libcxx webrtc // This is not enough for abseil-cpp
    ```
  * Debug version
    ```
    gn gen out/Debug_use_custom_libcxx --args="clang_base_path=\"/lib/llvm-17/\" clang_use_chrome_plugins=false"
    Done. Made 1714 targets from 293 files in 1005ms

    ninja -C out/Debug_use_custom_libcxx                            // about 22 minutes
    ninja: Entering directory `out/Debug_use_custom_libcxx'
    [6763/6763] STAMP obj/default.stamp
    ```


* Build with `use_custom_libcxx=false` (Use host default toolchain's libstdc++ on Ubuntu)
  * Release version
    ```
    gn gen out/Release --args="is_debug=false clang_base_path=\"/lib/llvm-17/\" clang_use_chrome_plugins=false use_custom_libcxx=false"
    Done. Made 1762 targets from 295 files in 412ms

    ninja -C out/Release webrtc test_video_capturer platform_video_capturer  // about 5 minutes
    ninja: Entering directory `out/Release'
    [3343/3343] AR obj/libwebrtc.a

    ```
  * Debug version
    ```
    gn gen out/Debug  --args="clang_base_path=\"/lib/llvm-17/\" clang_use_chrome_plugins=false use_custom_libcxx=false"
    Done. Made 1762 targets from 295 files in 419ms

    ninja -C out/Debug  webrtc test_video_capturer platform_video_capturer  // about 5 minutes
    ninja -C out/Debug  webrtc test_video_capturer platform_video_capturer
    ninja: Entering directory `out/Debug'
    [3343/3343] AR obj/libwebrtc.a

    ```
### llvm for C++ stdlib
* get llvm original commit-id from libc++ of webrtc source tree
  ```
  webrtc-checkout/src/buildtools/third_party/libc++/trunk ((HEAD detached at 055b2e17a))$ git log -1 .
  commit 055b2e17ae4f0e2c025ad0c7508b01787df17758 (HEAD)
  Author: Advenam Tacet <advenam.tacet@trailofbits.com>
  Date:   Thu May 4 17:43:51 2023 -0700

      [ASan][libcxx] Annotating std::vector with all allocators

      This revision is a part of a series of patches extending
      AddressSanitizer C++ container overflow detection
      capabilities by adding annotations, similar to those existing
      in std::vector, to std::string and std::deque collections.
      These changes allow ASan to detect cases when the instrumented
      program accesses memory which is internally allocated by
      the collection but is still not in-use (accesses before or
      after the stored elements for std::deque, or between the size and
      capacity bounds for std::string).

      The motivation for the research and those changes was a bug,
      found by Trail of Bits, in a real code where an out-of-bounds read
      could happen as two strings were compared via a std::equals function
      that took iter1_begin, iter1_end, iter2_begin iterators
      (with a custom comparison function).
      When object iter1 was longer than iter2, read out-of-bounds on iter2
      could happen. Container sanitization would detect it.

      In revision D132522, support for non-aligned memory buffers (sharing
      first/last granule with other objects) was added, therefore the
      check for standard allocator is not necessary anymore.
      This patch removes the check in std::vector annotation member
      function (__annotate_contiguous_container) to support
      different allocators.

      Additionally, this revision fixes unpoisoning in std::vector.
      It guarantees that __alloc_traits::deallocate may access returned memory.
      Originally suggested in D144155 revision.

      If you have any questions, please email:
      - advenam.tacet@trailofbits.com
      - disconnect3d@trailofbits.com

      Reviewed By: #libc, #sanitizers, philnik, vitalybuka, ldionne

      Spies: mikhail.ramalho, manojgupta, ldionne, AntonBikineev, ayzhao, hans, EricWF, philnik, #sanitizers, libcxx-commits

      Differential Revision: https://reviews.llvm.org/D136765

      NOKEYCHECK=True
      GitOrigin-RevId: c08d4ad25cf3f335e9b2e7b1b149eb1b486868f1
  ```
```
sudo apt-get install ninja-build
```
* https://libcxx.llvm.org/BuildingLibcxx.html
* Find right version of clang


```
git clone https://github.com/llvm/llvm-project.git    # about 11 minutes
cd llvm-project

git checkout c08d4ad25cf3f335e9b2e7b1b149eb1b486868f1 # matching chrome-libcxx
mkdir build
```
```
cmake -G Ninja -S runtimes -B build -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind"  -DCMAKE_CXX_COMPILER=clang++-17 -DCMAKE_C_COMPILER=clang-17 -DLIBCXX_ABI_NAMESPACE=__Cr \
-DLIBCXX_ABI_UNSTABLE=ON \
-DLIBCXX_ENABLE_VENDOR_AVAILABILITY_ANNOTATIONS=OFF
```
```
llvm-project ((HEAD detached at c08d4ad25cf3))$ ninja -C build cxx cxxabi
ninja: Entering directory `build'
[1086/1086] Linking CXX static library lib/libc++.a
```

#### Download NVIDIA Precompiled `libwebrtc.a`
* download `WebRTC_R36.3.0_aarch64.tbz2` from https://developer.nvidia.com/embedded/jetson-linux-r363
```
mkdir precompiled
tar xjvf ./WebRTC_R36.3.0_aarch64.tbz2 -C ./precompiled
```
## `peer_clients`
### Dependencies

| Name | Description | Version |
| --- | --- | --- |
| socket.io-client | | latest |
| spdlog | | v1.9.2|
| abseil-cpp | | dc37a887fd * |

#### `abseil-cpp`
```
webrtc-checkout/src/third_party/abseil-cpp ((HEAD detached at 770155421d2))$ git log -1 .
commit ebdeb0dc6980674c597525d712f5826e7e081da0
Author: Mirko Bonadei <mbonadei@chromium.org>
Date:   Wed Jun 14 12:03:45 2023 +0000

    Roll abseil_revision 1285ca4b4f..dc37a887fd

    Change Log:
    https://chromium.googlesource.com/external/github.com/abseil/abseil-cpp/+log/1285ca4b4f..dc37a887fd
    Full diff:
    https://chromium.googlesource.com/external/github.com/abseil/abseil-cpp/+/1285ca4b4f..dc37a887fd

    Bug: None
    Change-Id: I18c63b7afbf721ce9d3e36e5b797786a1d40b84b
    Reviewed-on: https://chromium-review.googlesource.com/c/chromium/src/+/4613486
    Reviewed-by: Danil Chapovalov <danilchap@chromium.org>
    Commit-Queue: Mirko Bonadei <mbonadei@chromium.org>
    Cr-Commit-Position: refs/heads/main@{#1157458}
    NOKEYCHECK=True
    GitOrigin-RevId: 97d7b04952d774f89e9060f57800a207b4f1e30b

```

```
cd abseil-cpp
patch -p3 <  ./../../../webrtc-checkout/src/third_party/abseil-cpp/patches/0001-Turn-on-hardened-mode.patch
patch -p3 <  ./../../../webrtc-checkout/src/third_party/abseil-cpp/patches/0002-delete-unprefixed-annotations.patch
patch -p3 <  ./../../../webrtc-checkout/src/third_party/abseil-cpp/patches/0003-delete-static-initializer-in-stacktrace.patch
```

### Build
```
mkdir build_precompiled && cd build_precompiled
cmake -DUSE_PRECOMPILED_WEBRTC=ON -DUSE_CUSTOM_LIBCXX=ON .. && make -j
```
