# LTO Technical Reference for Quarisma

**Purpose**: Detailed technical information about LTO implementation, compiler flags, and troubleshooting

---

## 1. Compiler-Specific LTO Implementation

### GCC LTO

**Compilation Flags**
```bash
-flto              # Enable LTO
-flto=auto         # Auto-detect number of parallel jobs
-flto=4            # Use 4 parallel jobs
-fno-lto           # Disable LTO for specific files
```

**Linker Tools**
```bash
gcc-ar             # Wrapper for ar (archive tool)
gcc-ranlib         # Wrapper for ranlib (archive indexing)
gcc-nm             # Wrapper for nm (symbol listing)
```

**CMake Integration**
```cmake
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_CXX_COMPILE_OBJECT "<CMAKE_CXX_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -o <OBJECT> -c <SOURCE>")
```

### Clang LTO

**Compilation Flags**
```bash
-flto              # Enable LTO (thin LTO by default in Clang 4.0+)
-flto=full         # Full LTO (slower, more optimization)
-flto=thin         # Thin LTO (faster, less optimization)
-fno-lto           # Disable LTO for specific files
```

**Linker Tools**
```bash
llvm-ar            # Optional: LLVM archive tool
llvm-ranlib        # Optional: LLVM archive indexing
llvm-nm            # Optional: LLVM symbol listing
```

**Thin LTO vs Full LTO**
- **Thin LTO**: Faster linking, less optimization (default)
- **Full LTO**: Slower linking, more optimization
- **Quarisma**: Uses default (Thin LTO)

### Apple Clang LTO

**Compilation Flags**
```bash
-flto              # Enable LTO
-flto=thin         # Thin LTO (default)
-flto=full         # Full LTO
```

**System Integration**
- Uses system linker (ld64)
- No special tools required
- Excellent LTO support

### MSVC LTO

**Compilation Flags**
```cpp
/GL                // Enable LTO during compilation
/Gw                // Optimize global data
```

**Linker Flags**
```cpp
/LTCG              // Link-Time Code Generation
/LTCG:PGI          // LTO with Profile-Guided Optimization
/LTCG:PGO          // LTO with Profile-Guided Optimization
```

**Limitations**
- Less aggressive optimization than GCC/Clang
- Limited cross-module inlining
- Requires `/GL` at compile time and `/LTCG` at link time

---

## 2. Quarisma CMake Configuration

### Root CMakeLists.txt

```cmake
# Line 32: LTO option
option(QUARISMA_ENABLE_LTO "Enable Link Time Optimization" ON)

# Lines 35-42: LTO configuration
if(QUARISMA_ENABLE_LTO)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
  message(STATUS "Link Time Optimization (LTO) enabled via CMAKE_INTERPROCEDURAL_OPTIMIZATION")
else()
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE OFF)
  message(STATUS "Link Time Optimization (LTO) disabled")
endif()
```

### Library/Core/CMakeLists.txt

```cmake
# Lines 134-141: Per-target LTO configuration
if(QUARISMA_ENABLE_LTO)
  set_target_properties(Core PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
  message(STATUS "Core library: Link Time Optimization (LTO) enabled")
else()
  message(STATUS "Core library: Link Time Optimization (LTO) disabled")
endif()
```

### Linker Configuration (linker.cmake)

```cmake
# Lines 29-34: LTO + Linker Interaction
if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
  message("LTO is enabled - skipping faster linker configuration to avoid memory issues")
  return()
endif()
```

**Rationale**: LTO with gold/mold linkers causes OOM errors

### Coverage Configuration (coverage.cmake)

```cmake
# Line 19: LTO disabled for coverage
if(QUARISMA_ENABLE_COVERAGE)
  set(QUARISMA_ENABLE_LTO OFF)
endif()
```

**Reason**: Coverage instrumentation incompatible with LTO IR

---

## 3. Build Script Integration

### Scripts/setup.py

**LTO Toggle Mechanism**
```python
# LTO is ON by default in release builds
# Adding 'lto' flag toggles it OFF

# Examples:
# python setup.py config.build.ninja.clang.release
#   → LTO enabled (default)

# python setup.py config.build.ninja.clang.release.lto
#   → LTO disabled (toggled OFF)
```

**CMake Flags Generated**
```bash
# LTO enabled
-DQUARISMA_ENABLE_LTO=ON

# LTO disabled
-DQUARISMA_ENABLE_LTO=OFF
```

---

## 4. Performance Characteristics

### Compilation Phase

**Time Overhead**: 10-30%
```
Non-LTO: 100 seconds
LTO:     110-130 seconds
```

**Reason**: Compiler generates IR instead of native code

### Linking Phase

**Time Overhead**: 20-50%
```
Non-LTO: 10 seconds
LTO:     12-15 seconds (small project)
LTO:     30-50 seconds (large project like Quarisma)
```

**Memory Usage**: 2-4x higher
```
Non-LTO: 500 MB
LTO:     1-2 GB
```

### Runtime Performance

**Improvement**: 5-15%
```
Non-LTO: 100 ms
LTO:     85-95 ms
```

**Varies by**:
- Code structure
- Optimization opportunities
- Cross-module function calls

### Binary Size

**Reduction**: 5-10%
```
Non-LTO: 100 MB
LTO:     90-95 MB
```

---

## 5. Troubleshooting Guide

### Issue: Out-of-Memory During Linking

**Symptoms**
```
error: linker killed by signal 9 (out of memory)
error: lld-link: error: out of memory
```

**Solutions**
1. **Disable LTO**
   ```bash
   cmake -B build -S . -DQUARISMA_ENABLE_LTO=OFF
   ```

2. **Increase Available Memory**
   ```bash
   # Linux: Increase swap
   sudo fallocate -l 4G /swapfile
   sudo chmod 600 /swapfile
   sudo mkswap /swapfile
   sudo swapon /swapfile
   ```

3. **Use Default Linker**
   - Quarisma automatically disables faster linkers with LTO
   - Verify: Check CMake output for linker selection

### Issue: Slow Incremental Builds

**Symptoms**
- Changing one file requires full re-linking
- Linking takes 30+ seconds

**Solutions**
1. **Disable LTO for Development**
   ```bash
   python setup.py config.build.test.ninja.clang.debug
   ```

2. **Use Thin LTO** (Clang only)
   - Already default in Quarisma
   - Faster than full LTO

### Issue: Debugging Difficulties

**Symptoms**
- Breakpoints not working
- Stack traces inaccurate
- Line numbers wrong

**Solutions**
1. **Use RelWithDebInfo Build**
   ```bash
   cmake -B build -S . -DCMAKE_BUILD_TYPE=RelWithDebInfo -DQUARISMA_ENABLE_LTO=OFF
   ```

2. **Disable LTO for Specific File**
   ```cmake
   set_source_files_properties(problematic.cpp PROPERTIES COMPILE_FLAGS "-fno-lto")
   ```

### Issue: Compiler Crash

**Symptoms**
```
internal compiler error
compiler crashed
```

**Solutions**
1. **Update Compiler**
   - GCC: Update to 7.0+
   - Clang: Update to 5.0+
   - MSVC: Update to VS 2017 15.7+

2. **Disable LTO**
   ```bash
   cmake -B build -S . -DQUARISMA_ENABLE_LTO=OFF
   ```

3. **Report Issue**
   - Include compiler version
   - Include error message
   - Include minimal reproduction case

---

## 6. Verification Commands

### Check LTO Support

```bash
# GCC
gcc -flto -c test.cpp -o test.o

# Clang
clang -flto -c test.cpp -o test.o

# MSVC
cl /GL test.cpp

# Apple Clang
clang -flto -c test.cpp -o test.o
```

### Verify LTO Enabled in Build

```bash
# Check CMake output
cmake -B build -S . | grep -i "LTO"

# Check compiler flags
cmake --build build --verbose | grep -i "lto\|GL\|LTCG"
```

### Measure Performance Impact

```bash
# Build without LTO
time cmake --build build_no_lto

# Build with LTO
time cmake --build build_lto

# Compare binary sizes
ls -lh build_no_lto/lib/libCore.so
ls -lh build_lto/lib/libCore.so
```

---

## 7. Platform-Specific Notes

### Linux (GCC/Clang)

**Recommended Settings**
```bash
cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=Release \
  -DQUARISMA_ENABLE_LTO=ON \
  -DQUARISMA_LINKER_CHOICE=default
```

**Known Issues**
- Gold linker incompatible with LTO (automatically skipped)
- Mold linker incompatible with LTO (automatically skipped)

### macOS (Apple Clang)

**Recommended Settings**
```bash
cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=Release \
  -DQUARISMA_ENABLE_LTO=ON
```

**Advantages**
- Excellent LTO support
- System linker handles LTO well
- No known issues

### Windows (MSVC/Clang)

**MSVC Settings**
```bash
cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=Release \
  -DQUARISMA_ENABLE_LTO=ON
```

**Clang Settings**
```bash
cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=Release \
  -DQUARISMA_ENABLE_LTO=ON \
  -DQUARISMA_LINKER_CHOICE=lld-link
```

**Known Issues**
- Limited CI testing with Windows DLLs
- MSVC LTO less aggressive than GCC/Clang

---

## 8. References

- [GCC LTO Documentation](https://gcc.gnu.org/wiki/LinkTimeOptimization)
- [Clang LTO Documentation](https://clang.llvm.org/docs/ThinLTO.html)
- [MSVC LTO Documentation](https://docs.microsoft.com/en-us/cpp/build/reference/ltcg-link-time-code-generation)
- [CMake IPO Documentation](https://cmake.org/cmake/help/latest/module/CheckIPOSupported.html)

