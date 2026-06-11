# nostalgia

## Build

```bash
cmake -G "Ninja Multi-Config" -S . -B build -D CMAKE_C_COMPILER=clang -D CMAKE_CXX_COMPILER=clang++
cmake --build build --parallel 12 --config Debug
```
