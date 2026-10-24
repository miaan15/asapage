asapage - ah sh** another project about game engine

## Run
### Require
- Ninja
- CMake
- Vulkan
```
# Window
winget install KhronosGroup.VulkanSDK
```

### Run
```
cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/asapage
```