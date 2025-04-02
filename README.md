# ConsoleArrayVisualizer (WIP)

Visualizes arrays on Windows terminal emulator.

### Demo with [Alacritty](https://github.com/alacritty/alacritty)
<details closed>
<summary>Videos</summary>

Shellsort, Quicksort, Heapsort on 840 elements, with delays:

https://github.com/user-attachments/assets/8383624c-5651-4755-88d4-7f378d377c12

Parallel Shellsort with 6 extra threads on 16777216 elements, without delays:

https://github.com/user-attachments/assets/9abadf2c-0d15-4b4f-9a0f-9544249eb9dd

</details>

## Features

### Current

+ Single array visualization
+ Read & Write counters
+ Fixed set of marker types
+ Delays
+ Large array support
+ Average of overlapped elements
+ Support for most if not all concurrent algorithms  
  *Updates are not thread-safe on the same element. In another words, multiple threads shouldn't try to access the same element at the same time.*
+ Thread pool
+ FPS limit (constant value in code)
+ Performance: Fast  
  *It can run Shellsort on 1 million elements in 8 seconds. Great improvements can be made in the future.*

### Future

+ Multiple arrays visualization
+ Virtual terminal renderer
+ Cross platform support
+ Ability to disable average of overlapped elements for performance
+ Optimize atomic accesses' memory order
+ Use a subset of C++
+ Improve API
+ Comparision / branches counter
+ GPU shader renderer
+ Command line improvements & GUI
+ More types of visualization (eg: Gradient)
+ Stability test
+ Zooming

## Build

### Visual Studio

Use the file `ConsoleArrayVisualizer.sln`

### CMake

MinGW build is supported; Toolchain file is optional.

```
md Build\CMake
cd Build\CMake
cmake ..\.. -G Ninja -DCMAKE_TOOLCHAIN_FILE="Path to toolchain file.cmake" -DCMAKE_BUILD_TYPE=RelWithDebInfo
Ninja
```
