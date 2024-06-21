
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

### clang
* Install clang on Ubuntu
  * https://apt.llvm.org/
  * https://ubuntuhandbook.org/index.php/2023/09/how-to-install-clang-17-or-16-in-ubuntu-22-04-20-04/
  ```
  wget https://apt.llvm.org/llvm.sh
  chmod +x llvm.sh
  sudo ./llvm.sh <version number>
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
* checkout the milestone `119`
```
cd src
git checkout branch-heads/6045
gclient sync
```
* Build
  * Release version
    ```
    gn gen out/Release --args='is_debug=false'
    Done. Made 1762 targets from 295 files in 412ms

    ninja -C out/Release                            // about 10 minutes
    ninja: Entering directory `out/Release'
    [7242/7242] STAMP obj/default.stamp
    ```
    ```
    ninja -C out/Release webrtc // This is not enough for absl
    ```
  * Debug version
    ```
    gn gen out/Default
    Done. Made 1762 targets from 295 files in 419ms

    ninja -C out/Default                            // about 10 minutes
    ninja: Entering directory `out/Default'
    [7242/7242] STAMP obj/default.stamp
    ```

### llvm for lib
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
### Libraries

| Name | Description | Version |
| --- | --- | --- |
| socket.io-client | | latest |
| spdlog | | v1.9.2|



