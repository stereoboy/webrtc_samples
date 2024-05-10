## Chromium WebRTC Implementation
### References
* Official Development Document
  * https://webrtc.github.io/webrtc-org/native-code/development/
  * https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up
```
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git

export PATH=/path/to/depot_tools:$PATH
```

* Official Branch Details
  * https://chromiumdash.appspot.com/branches

* https://github.com/Tohntobshi/webrtcexample

### Source Download
```
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
### Configuration
```
gn gen out/Default
```
```
gn gen out/Release --args='is_debug=false'
```
* https://stackoverflow.com/questions/47348330/error-linking-webrtc-native-due-to-undefined-reference-to-methods-having-stdst
```
gn gen out/Custom --args='use_custom_libcxx=false use_custom_libcxx_for_host=false'
```
* https://github.com/webrtc-sdk/libwebrtc
* https://github.com/aisouard/libwebrtc
```
export ARCH=x64 # x86, x64, arm, arm64
gn gen out/Linux-$ARCH --args="target_os=\"linux\" target_cpu=\"$ARCH\" is_debug=false rtc_include_tests=false rtc_use_h264=true ffmpeg_branding=\"Chrome\" is_component_build=false use_rtti=true use_custom_libcxx=false rtc_enable_protobuf=false"
```
```
gn ls out/Default
```
```
gn args out/Default --list
gn args out/Default --list --short
```
### Build
```
ninja -C out/Default
```


* https://groups.google.com/g/discuss-webrtc/c/do0JGb44YmM
libc++-10-dev
libc++abi-10-dev

```
ninja -C out/Custom webrtc
```
```
apt-get install libc++abi-11-dev libc++-10-dev
```