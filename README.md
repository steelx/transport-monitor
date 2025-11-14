
### Build with Apple Clang
```bash
clang++ --std=c++17 -o test.out test.cpp
./test.out
```


### Build with CLion and CMake
```bash

# CLion "Reload CMake Project" after the above command
# CLion will automatically detect changes in CMakeLists.txt and reconfigure the project.

# OR
# 1. configure CMakeLists.txt to use Ninja and fetch content
cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release

```


