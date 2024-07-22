
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
    gn gen out/Release --args="is_debug=false clang_base_path=\"/lib/llvm-17/\" clang_use_chrome_plugins=false"
    Done. Made 1714 targets from 293 files in 951ms

    ninja -C out/Release                            // about 22 minutes
    ninja: Entering directory `out/Release'
    [7242/7242] STAMP obj/default.stamp
    ```
    ```
    ninja -C out/Release webrtc // This is not enough for abseil-cpp
    ```
  * Debug version
    ```
    gn gen out/Default --args="clang_base_path=\"/lib/llvm-17/\" clang_use_chrome_plugins=false"
    Done. Made 1714 targets from 293 files in 1005ms

    ninja -C out/Default                            // about 22 minutes
    ninja: Entering directory `out/Default'
    [7242/7242] STAMP obj/default.stamp
    ```

### llvm for C++ stdlib
```
sudo apt-get install ninja-build
```
* https://libcxx.llvm.org/BuildingLibcxx.html
* Find right version of clang


```
git clone https://github.com/llvm/llvm-project.git    # about 11 minutes
cd llvm-project
git checkout af7467ce9f447d6fe977b73db1f03a18d6bbd511 # matching chrome-libcxx-abi

git checkout 3c4b673af05f53e8a4d1a382b5c86367ea512c9e # matching chrome-libcxx
mkdir build
```
```
cmake -G Ninja -S runtimes -B build -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind"  -DCMAKE_CXX_COMPILER=clang++-17 -DCMAKE_C_COMPILER=clang-17 -DLIBCXX_ABI_NAMESPACE=__Cr \
-DLIBCXX_ABI_UNSTABLE=ON \
-DLIBCXX_ENABLE_VENDOR_AVAILABILITY_ANNOTATIONS=OFF
```
```
ninja -C build cxx cxxabi  # about 3 minutes
ninja: Entering directory `build'
[1190/1190] Linking CXX static library lib/libc++.a
```

## `peer_clients`
### Libraries

| Name | Description | Version |
| --- | --- | --- |
| socket.io-client | | latest |
| spdlog | | v1.9.2|



