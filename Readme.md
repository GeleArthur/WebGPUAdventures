# WebGPU Adventure

## Dawn only
> I am only supporting dawn right now. Because if I want to use emscripting I am at the mercy of dawn with there out of date interface.

> If you want to build with emscripting you have to manual update it your self [instructions](https://github.com/emscripten-core/emscripten/blob/main/system/include/webgpu/README.md). (written on 07/feb/2024).

## Build instruction
### Windows
```bash
mkdir build
cd build
cmake ..
```

### Browser
```bash
mkdir build-web
cd build-web
# To view cpp file in chrome with https://chromewebstore.google.com/detail/cc++-devtools-support-dwa/pdcpmagijalfljmkmjngeonclgbbannb
emcmake cmake -DCMAKE_BUILD_TYPE=Debug ..  
```