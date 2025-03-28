
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
gclient sync -D           // about 5 minutes
```
* Size: About 30G
* checkout the milestone `119`
  ```
  cd src
  git checkout branch-heads/6045
  gclient sync -D
  ```
* Build with `use_custom_libcxx=true` (Default, Use in-tree libc++ (buildtools/third_party/libc++ and buildtools/third_party/libc++abi) instead of the system C++ library)
  * Release version
    ```
    src ((HEAD detached at branch-heads/6045))$ gn gen out/Release_use_custom_libcxx --args='is_debug=false'
    Done. Made 1762 targets from 295 files in 539ms

    src ((HEAD detached at branch-heads/6045))$ time ninja -C out/Release_use_custom_libcxx webrtc
    ninja: Entering directory `out/Release_use_custom_libcxx'
    [3836/3836] AR obj/libwebrtc.a

    real    3m34.578s
    user    32m39.614s
    sys     2m10.273s
    ```
  * Debug version
    ```
    src ((HEAD detached at branch-heads/6045))$ gn gen out/Debug_use_custom_libcxx
    Done. Made 1762 targets from 295 files in 431ms

    src ((HEAD detached at branch-heads/6045))$ time ninja -C out/Debug_use_custom_libcxx webrtc
    ninja: Entering directory `out/Debug_use_custom_libcxx'
    [3836/3836] AR obj/libwebrtc.a

    real    3m9.703s
    user    28m49.911s
    sys     2m17.805s
    ```

* Build with `use_custom_libcxx=false` (Use host default toolchain's libstdc++ on Ubuntu)
  * Release version
    ```
    src ((HEAD detached at branch-heads/6045))$ gn gen out/Release --args='is_debug=false use_custom_libcxx=false'
    Done. Made 1760 targets from 291 files in 1237ms

    src ((HEAD detached at branch-heads/6045))$ time ninja -C out/Release webrtc
    ninja: Entering directory `out/Release'
    [3768/3768] AR obj/libwebrtc.a

    real    2m40.945s
    user    27m41.764s
    sys     1m49.503s
    ```
  * Debug version
    ```
    src ((HEAD detached at branch-heads/6045))$ gn gen out/Debug  --args='use_custom_libcxx=false'
    Done. Made 1760 targets from 291 files in 378ms

    src ((HEAD detached at branch-heads/6045))$ time ninja -C out/Debug webrtc
    ninja: Entering directory `out/Debug'
    [3768/3768] AR obj/libwebrtc.a

    real    2m18.006s
    user    23m53.467s
    sys     1m50.566s
    ```

### Packaging headers and libraries
```bash
cd webrtc-checkout
mkdir include
time bash ~/jylee/webrtc_samples/cpp/copy_headers.sh ./src ./include/
Directory: ./src/api
Directory: ./src/audio
Directory: ./src/base
Directory: ./src/call
Directory: ./src/common_audio
Directory: ./src/common_video
Directory: ./src/experiments
Directory: ./src/infra
Directory: ./src/logging
Directory: ./src/media
Directory: ./src/modules
Directory: ./src/net
Directory: ./src/p2p
Directory: ./src/pc
Directory: ./src/resources
Directory: ./src/rtc_base
Directory: ./src/rtc_tools
Directory: ./src/system_wrappers
Directory: ./src/test
Directory: ./src/third_party/abseil-cpp
Directory: ./src/video
Directory: ./src/sdk
Header files copied successfully from the selected directories.

real	0m12.591s
user	0m9.538s
sys	0m3.488s
```
```bash
tar cvzf libwebrtc-ubuntu-x86_64.M119.tar.gz ./include/ \
./src/out/Debug/obj/libwebrtc.a  \
./src/out/Release/obj/libwebrtc.a \
./src/test/vcm_capturer.cc \
./src/test/platform_video_capturer.cc \
./src/test/test_video_capturer.cc
```
```bash
ls -l -h
total 78M
drwxrwxr-x 20 wom wom 4.0K Jan 17 14:42 include
-rw-rw-r--  1 wom wom  78M Jan 17 14:44 libwebrtc-ubuntu-x86_64.M119.tar.gz
drwxrwxr-x 37 wom wom 4.0K May 22  2024 src
```

### Use Package
```bash
mkdir prebuilt
tar xzvf libwebrtc-ubuntu-x86_64.M119.tar.gz -C ./prebuilt/
```

### llvm for C++ stdlib
* get llvm original commit-id from libc++ of webrtc source tree
  ```
  webrtc-checkout/src/third_party/libc++/src ((HEAD detached at 7cf98622a))$ git log -1 .
  commit 7cf98622abaf832e2d4784889ebc69d5b6fde4d8 (HEAD)
  Author: ZhangYin <zhangyin2018@iscas.ac.cn>
  Date:   Fri Sep 29 22:32:54 2023 +0800

      [libcxx] <experimental/simd> Add _LIBCPP_HIDE_FROM_ABI to internal br… (#66977)

      …oadcast functions

      NOKEYCHECK=True
      GitOrigin-RevId: cf31d0eca85f4f5b273dd1ad8f76791ff726c28f
  ```
```
sudo apt-get install ninja-build
```
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
llvm-project ((HEAD detached at cf31d0eca85f))$ ninja -C build cxx cxxabi
ninja: Entering directory `build'
[1161/1161] Linking CXX static library lib/libc++.a
```


## `peer_clients`
### Dependencies

| Name | Description | Version |
| --- | --- | --- |
| socket.io-client | | latest |
| spdlog | | v1.9.2|
| abseil-cpp | | 6ab667fd8d * |

#### `abseil-cpp`
```
webrtc_6045/src/third_party/abseil-cpp ((HEAD detached at 6b9e7118bf8))$ git log -1 .
commit 5872ee3c045ef92477844db6af375e2c12e87726
Author: Danil Chapovalov <danilchap@chromium.org>
Date:   Thu Sep 28 07:57:13 2023 +0000

    Roll abseil_revision d91f39ab5b..6ab667fd8d

    Change Log:
    https://chromium.googlesource.com/external/github.com/abseil/abseil-cpp/+log/d91f39ab5b..6ab667fd8d
    Full diff:
    https://chromium.googlesource.com/external/github.com/abseil/abseil-cpp/+/d91f39ab5b..6ab667fd8d

    Bug: None
    Change-Id: I9cefcca21aa0d3a142c9a9f637cc31783f7c8f67
    Reviewed-on: https://chromium-review.googlesource.com/c/chromium/src/+/4887263
    Reviewed-by: Mirko Bonadei <mbonadei@chromium.org>
    Commit-Queue: Danil Chapovalov <danilchap@chromium.org>
    Cr-Commit-Position: refs/heads/main@{#1202406}
    NOKEYCHECK=True
    GitOrigin-RevId: d55c179d96e3bc5fe4df07b73482de65546b657f

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
