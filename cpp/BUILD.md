
# WebRTC C++ Native (Milestone `M119`)

## Base Libraries
* Libraries to build from source

| Name | Description | Version | Size |
| --- | --- | --- | --- |
| WebRTC | chromium c++ implementation | `branch-heads/6045` | 22 G |
| llvm-project | for libc++, libc++abi | commit id `cf31d0eca85f4f5b273dd1ad8f76791ff726c28f` | 4.5 G |

* Prebuilt libraries

| Name | Description | Version | Size |
| --- | --- | --- | --- |
| llvm | for clang, lld | 18 |  |

### clang v18
* Install clang on Ubuntu
  * https://apt.llvm.org/
  * https://ubuntuhandbook.org/index.php/2023/09/how-to-install-clang-17-or-16-in-ubuntu-22-04-20-04/
  ```
  wget https://apt.llvm.org/llvm.sh
  chmod +x llvm.sh
  # sudo ./llvm.sh <version number>
  sudo ./llvm.sh 18

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
* checkout the milestone `119`
  ```
  cd src
  git checkout branch-heads/6045
  gclient sync
  ```
* Build with `use_custom_libcxx=true` (Default, Use in-tree libc++ (buildtools/third_party/libc++ and buildtools/third_party/libc++abi) instead of the system C++ library)
  * Release version
    ```
    gn gen out/Release_use_custom_libcxx --args='is_debug=false'
    Done. Made 1762 targets from 295 files in 412ms

    ninja -C out/Release_use_custom_libcxx                            // about 10 minutes
    ninja: Entering directory `out/Release_use_custom_libcxx'
    [7242/7242] STAMP obj/default.stamp
    ```
    ```
    ninja -C out/Release_use_custom_libcxx webrtc // This is not enough for abseil-cpp
    ```
  * Debug version
    ```
    gn gen out/Debug_use_custom_libcxx
    Done. Made 1762 targets from 295 files in 419ms

    ninja -C out/Debug_use_custom_libcxx                            // about 10 minutes
    ninja: Entering directory `out/Debug_use_custom_libcxx'
    [7242/7242] STAMP obj/default.stamp
    ```

* Build with `use_custom_libcxx=false` (Use host default toolchain's libstdc++ on Ubuntu)
  * Release version
    ```
    gn gen out/Release --args='is_debug=false use_custom_libcxx=false'
    Done. Made 1762 targets from 295 files in 412ms

    ninja -C out/Release webrtc test_video_capturer platform_video_capturer  // about 10 minutes
    ninja: Entering directory `out/Release'
    [7242/7242] STAMP obj/default.stamp
    ```
  * Debug version
    ```
    gn gen out/Debug  --args='use_custom_libcxx=false'
    Done. Made 1762 targets from 295 files in 419ms

    ninja -C out/Debug  webrtc test_video_capturer platform_video_capturer  // about 10 minutes
    ninja: Entering directory `out/Debug'
    [7242/7242] STAMP obj/default.stamp
    ```

### Packaging headers and libraries
```bash
cd webrtc-checkout
mkdir include
time bash ./copy_headers.sh ./src ./include
Directory: ./src//api
Directory: ./src//audio
Directory: ./src//base
Directory: ./src//call
Directory: ./src//common_audio
Directory: ./src//common_video
Directory: ./src//experiments
Directory: ./src//infra
Directory: ./src//logging
Directory: ./src//media
Directory: ./src//modules
Directory: ./src//net
Directory: ./src//p2p
Directory: ./src//pc
Directory: ./src//resources
Directory: ./src//rtc_base
Directory: ./src//rtc_tools
Directory: ./src//system_wrappers
Directory: ./src//third_party/abseil-cpp
Directory: ./src//video
Directory: ./src//sdk
Header files copied successfully from the selected directories.

real    0m45.189s
user    0m6.533s
sys     0m37.817s

```

```bash
tar czf libwebrtc.tar.gz ./include/ ./src/out/Debug/obj/libwebrtc.a  ./src/out/Release/obj/libwebrtc.a
```
```bash
ls -l -h
total 78M
drwxrwxr-x 20 wom wom 4.0K Jan 17 14:42 include
-rw-rw-r--  1 wom wom  78M Jan 17 14:44 libwebrtc.tar.gz
drwxrwxr-x 37 wom wom 4.0K May 22  2024 src

```

### llvm for C++ stdlib
* https://libcxx.llvm.org/BuildingLibcxx.html
* Find right version of clang


```
git clone https://github.com/llvm/llvm-project.git
cd llvm-project
git checkout cf31d0eca85f4f5b273dd1ad8f76791ff726c28f # matching chrome-libcxx
mkdir build
```
```
cmake -G Ninja -S runtimes -B build -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind"  -DCMAKE_CXX_COMPILER=clang++-18 -DCMAKE_C_COMPILER=clang-18 -DLIBCXX_ABI_NAMESPACE=__Cr \
-DLIBCXX_ABI_UNSTABLE=ON \
-DLIBCXX_ENABLE_VENDOR_AVAILABILITY_ANNOTATIONS=OFF \
-DLIBCXX_PSTL_CPU_BACKEND="std_thread" \
```
```
ninja -C build cxx cxxabi
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
mkdir build && cd build
cmake -DUSE_PRECOMPILED_WEBRTC=OFF -DUSE_CUSTOM_LIBCXX=OFF .. && make -j

mkdir build_use_custom_libcxx && cd build_use_custom_libcxx
cmake -DUSE_PRECOMPILED_WEBRTC=OFF -DUSE_CUSTOM_LIBCXX=ON .. && make -j
```

```
mkdir build_debug && cd build_debug
cmake -DUSE_PRECOMPILED_WEBRTC=OFF -DUSE_CUSTOM_LIBCXX=OFF -DCMAKE_BUILD_TYPE=Debug .. && make -j

mkdir build_debug_use_custom_libcxx && cd build_debug_use_custom_libcxx
cmake -DUSE_PRECOMPILED_WEBRTC=OFF -DUSE_CUSTOM_LIBCXX=ON -DCMAKE_BUILD_TYPE=Debug  .. && make -j
```

### Build with precompiled webrtc

```
mkdir precompiled && cp libwebrtc.tar.gz precompiled && cd precompiled
tar zxf libwebrtc.tar.gz
```
```
mkdir build && cd build
cmake -DUSE_PRECOMPILED_WEBRTC=ON -DUSE_CUSTOM_LIBCXX=OFF .. && make -j

mkdir build_use_custom_libcxx && cd build_use_custom_libcxx
cmake -DUSE_PRECOMPILED_WEBRTC=ON -DUSE_CUSTOM_LIBCXX=ON .. && make -j
```

### peer_audio_client
* Pulseaudio
  * https://freedesktop.org/software/pulseaudio/doxygen/index.html

* Archlinux's Pulseaudio Wiki
  * https://wiki.archlinux.org/title/PulseAudio

```
sudo apt-get install libpulse-dev
```
* `libpulse`
  ```
  pa_context_get_sink_info_list

  pa_context_get_source_info_list

  pa_context_set_default_sink

  pa_context_set_default_source
  ```

#### PulseAudio Setup for Linux
  ```
  pactl list sources
  ```

  ```
  pactl list sinks
  ```
  ```
  pactl info
  ```
  ```
  $ pactl get-default-sink
  alsa_output.pci-0000_00_1f.3.iec958-stereo
  $ pactl list short sinks
  1       alsa_output.pci-0000_00_1f.3.iec958-stereo      module-alsa-card.c      s16le 2ch 44100Hz       IDLE
  84      alsa_output.pci-0000_01_00.1.hdmi-stereo-extra1 module-alsa-card.c      s16le 2ch 44100Hz       IDLE
  $ pactl set-default-sink alsa_output.pci-0000_01_00.1.hdmi-stereo-extra1
  $ pactl get-default-sink
  alsa_output.pci-0000_01_00.1.hdmi-stereo-extra1
  ```
### peer_video_client
* GTK3 for visualization
  * https://docs.gtk.org/gtk3/getting_started.html
```
sudo apt-get install libgtk-3-dev libglib2.0-dev
```
* `libyuv`
```
sudo apt-get install libyuv-dev libyuv0
```
