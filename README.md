# ConsoleArrayVisualizer
Visualizes arrays on Windows terminal emulator.

Usage: Run the executable in the console.

# Build

### Visual Studio

Use the file `ConsoleArrayVisualizer.sln`

### CMake

MinGW LLVM build is supported for ARMv7.

```
md Build\Cmake
cd Build\CMake
cmake ..\.. -G Ninja -DCMAKE_TOOLCHAIN_FILE="Path to toolchain file.cmake" -DCMAKE_BUILD_TYPE=RelWithDebInfo
Ninja
```
