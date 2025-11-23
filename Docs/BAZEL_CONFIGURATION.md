# XSigma Bazel Configuration Guide

This document explains how to configure and build XSigma using Bazel, including enabling/disabling features, managing build flags, and upgrading Bazel versions.

## Table of Contents

1. [Quick Start](#quick-start)
2. [Build Configuration](#build-configuration)
3. [Feature Flags](#feature-flags)
4. [Compiler & Build Tool Selection](#compiler--build-tool-selection)
5. [Logging Backends](#logging-backends)
6. [Profiler Backends](#profiler-backends)
7. [GPU Support](#gpu-support)
8. [Sanitizers](#sanitizers)
9. [Common Build Configurations](#common-build-configurations)
10. [Bazel Version Management](#bazel-version-management)
11. [Troubleshooting](#troubleshooting)

## Quick Start

### Prerequisites

- **Bazel or Bazelisk** installed (recommended: Bazelisk for automatic version management)
- **Python 3.9+** for the build configuration script
- **C++17 compatible compiler** (Clang, GCC, or MSVC)

### Installation

```bash
# macOS
brew install bazelisk

# Linux (via npm)
npm install -g @bazel/bazelisk

# Windows
# Download from: https://github.com/bazelbuild/bazelisk/releases
# Or use Chocolatey: choco install bazelisk
```

### Basic Build

```bash
# Show configuration (no build)
python Scripts/setup_bazel.py config.release

# Build with default settings (Ninja + Clang, Debug)
python Scripts/setup_bazel.py build.test

# Release build with tests
python Scripts/setup_bazel.py build.test.release

# Clean and rebuild
python Scripts/setup_bazel.py clean.build.test.release
```

## Build Configuration

### Build Types

```bash
# Debug build (default, includes debug symbols)
python Scripts/setup_bazel.py build.debug

# Release build (optimized, no debug symbols)
python Scripts/setup_bazel.py build.release

# Release with debug info (optimized + debug symbols)
python Scripts/setup_bazel.py build.relwithdebinfo
```

### C++ Standard

```bash
# C++17 (default)
python Scripts/setup_bazel.py build.cxx17

# C++20
python Scripts/setup_bazel.py build.cxx20

# C++23
python Scripts/setup_bazel.py build.cxx23
```

### Vectorization Options

```bash
# SSE vectorization
python Scripts/setup_bazel.py build.release.sse

# AVX vectorization
python Scripts/setup_bazel.py build.release.avx

# AVX2 vectorization (recommended)
python Scripts/setup_bazel.py build.release.avx2

# AVX-512 vectorization
python Scripts/setup_bazel.py build.release.avx512
```

### Link-Time Optimization (LTO)

```bash
# Enable LTO for better optimization
python Scripts/setup_bazel.py build.release.lto.avx2
```

## Feature Flags

Feature flags enable/disable optional components. Use the `--config` flag or pass them as arguments to `setup_bazel.py`.

### Available Features

| Feature | Flag | Description | Default |
|---------|------|-------------|---------|
| TBB | `tbb` | Intel Threading Building Blocks | OFF |
| mimalloc | `mimalloc` | Microsoft mimalloc allocator | OFF |
| magic_enum | `magic_enum` | Magic enum library | OFF |
| OpenMP | `openmp` | OpenMP support | OFF |
| CUDA | `cuda` | NVIDIA CUDA support | OFF |
| HIP | `hip` | AMD HIP support | OFF |

### Enabling Features

```bash
# Enable TBB
python Scripts/setup_bazel.py build.release.tbb

# Enable mimalloc
python Scripts/setup_bazel.py build.release.mimalloc

# Enable multiple features
python Scripts/setup_bazel.py build.release.tbb.mimalloc.magic_enum

# Direct Bazel command
bazel build --config=tbb --config=mimalloc //...
```

## Compiler & Build Tool Selection

### Default Behavior

By default, XSigma uses **Ninja + Clang** on all platforms (Windows, macOS, Linux).

### Compiler Selection

```bash
# Clang (default)
python Scripts/setup_bazel.py build.release

# GCC
python Scripts/setup_bazel.py build.release.gcc

# MSVC (Windows only)
python Scripts/setup_bazel.py build.release.msvc
```

### Build Tool Selection

```bash
# Ninja (default)
python Scripts/setup_bazel.py build.release.ninja

# Xcode (macOS only)
python Scripts/setup_bazel.py build.release.xcode

# Visual Studio (Windows only)
python Scripts/setup_bazel.py build.release.vs22
```

### Visual Studio Versions (Windows)

```bash
# Visual Studio 2017
python Scripts/setup_bazel.py build.release.vs17

# Visual Studio 2019
python Scripts/setup_bazel.py build.release.vs19

# Visual Studio 2022
python Scripts/setup_bazel.py build.release.vs22

# Visual Studio 2026
python Scripts/setup_bazel.py build.release.vs26
```

## Logging Backends

XSigma supports multiple logging backends. Default is **Loguru**.

```bash
# Loguru (default)
python Scripts/setup_bazel.py build.release.logging_loguru

# Google glog
python Scripts/setup_bazel.py build.release.logging_glog

# Native fmt-based logging
python Scripts/setup_bazel.py build.release.logging_native
```

## Profiler Backends

XSigma supports multiple profiler backends. Default is **Kineto**.

```bash
# Kineto profiler (default)
python Scripts/setup_bazel.py build.release.kineto

# Native profiler
python Scripts/setup_bazel.py build.release.profiler_native

# ITT profiler
python Scripts/setup_bazel.py build.release.profiler_itt
```

## GPU Support

### CUDA Support

```bash
# Enable CUDA
python Scripts/setup_bazel.py build.release.cuda

# CUDA with specific allocation strategy
python Scripts/setup_bazel.py build.release.cuda.gpu_alloc_async

# Direct Bazel command
bazel build --config=cuda //...
```

### HIP Support (AMD ROCm)

```bash
# Enable HIP
python Scripts/setup_bazel.py build.release.hip

# HIP with specific allocation strategy
python Scripts/setup_bazel.py build.release.hip.gpu_alloc_pool_async

# Direct Bazel command
bazel build --config=hip //...
```

### GPU Allocation Strategies

```bash
# Synchronous allocation (default for CPU)
python Scripts/setup_bazel.py build.release.cuda.gpu_alloc_sync

# Asynchronous allocation
python Scripts/setup_bazel.py build.release.cuda.gpu_alloc_async

# Pool-based asynchronous allocation (default for GPU)
python Scripts/setup_bazel.py build.release.cuda.gpu_alloc_pool_async
```

## Sanitizers

Sanitizers help detect memory and threading issues. Available on Unix-like systems.

```bash
# AddressSanitizer (memory errors)
python Scripts/setup_bazel.py build.debug.sanitizer_asan

# ThreadSanitizer (data races)
python Scripts/setup_bazel.py build.debug.sanitizer_tsan

# UndefinedBehaviorSanitizer (undefined behavior)
python Scripts/setup_bazel.py build.debug.sanitizer_ubsan

# MemorySanitizer (uninitialized memory)
python Scripts/setup_bazel.py build.debug.sanitizer_msan
```

## Common Build Configurations

### Development Build (Fast Iteration)

```bash
python Scripts/setup_bazel.py build.test.debug
```

### Release Build (Optimized)

```bash
python Scripts/setup_bazel.py build.test.release.avx2.lto
```

### Release with Debug Info

```bash
python Scripts/setup_bazel.py build.test.relwithdebinfo.avx2
```

### GPU-Accelerated Build (CUDA)

```bash
python Scripts/setup_bazel.py build.test.release.cuda.avx2
```

### GPU-Accelerated Build (HIP)

```bash
python Scripts/setup_bazel.py build.test.release.hip.avx2
```

### Full-Featured Build

```bash
python Scripts/setup_bazel.py build.test.release.cuda.tbb.mimalloc.magic_enum.avx2.lto
```

### Debug with Sanitizers

```bash
python Scripts/setup_bazel.py build.test.debug.sanitizer_asan
```

### Xcode Build (macOS)

```bash
python Scripts/setup_bazel.py build.test.release.xcode
```

### Visual Studio Build (Windows)

```bash
python Scripts/setup_bazel.py build.test.release.vs22
```

## Bazel Version Management

### Automatic Version Management (Recommended)

Use **Bazelisk** for automatic Bazel version management. Bazelisk reads the `.bazelversion` file and automatically downloads the correct Bazel version.

```bash
# Check current Bazel version
bazelisk version

# Bazelisk automatically uses version from .bazelversion
bazelisk build //...
```

### Manual Version Management

If using Bazel directly (not Bazelisk):

```bash
# Check installed Bazel version
bazel version

# Upgrade Bazel
# macOS
brew upgrade bazel

# Linux (via npm)
npm install -g @bazel/bazelisk@latest

# Windows
# Download latest from: https://github.com/bazelbuild/bazel/releases
```

### Checking Bazel Version

```bash
# Show Bazel version
bazel version

# Show Bazelisk version
bazelisk version
```

### .bazelversion File

The `.bazelversion` file specifies the Bazel version to use:

```
# Current version in .bazelversion
7.0.0
```

To update:

```bash
# Edit .bazelversion
echo "7.1.0" > .bazelversion

# Bazelisk will automatically download and use this version
bazelisk build //...
```

## Troubleshooting

### Bazel Not Found

```bash
# Error: Neither bazel nor bazelisk found in PATH

# Solution: Install Bazelisk
# macOS
brew install bazelisk

# Linux
npm install -g @bazel/bazelisk

# Windows
# Download from: https://github.com/bazelbuild/bazelisk/releases
```

### Build Fails with Configuration Error

```bash
# Clean Bazel cache
bazel clean --expunge

# Rebuild
python Scripts/setup_bazel.py build.test.release
```

### CUDA Not Found

```bash
# Ensure CUDA is installed and in PATH
# Then rebuild with CUDA config
python Scripts/setup_bazel.py build.release.cuda
```

### Slow Builds

```bash
# Use release build with optimizations
python Scripts/setup_bazel.py build.release.avx2.lto

# Use parallel build
bazel build -j 8 //...
```

### Out of Memory During Build

```bash
# Reduce parallel jobs
bazel build -j 2 //...

# Or use Bazel's memory limit
bazel build --memory_limit_mb=4096 //...
```

## Configuration File Reference

### .bazelrc

The `.bazelrc` file contains Bazel configuration profiles:

- **Compiler configs**: `clang`, `gcc`, `msvc`
- **Build type configs**: `debug`, `release`, `relwithdebinfo`
- **Feature configs**: `cuda`, `hip`, `tbb`, `mimalloc`, etc.
- **Logging configs**: `logging_glog`, `logging_loguru`, `logging_native`
- **Profiler configs**: `kineto`, `native_profiler`, `itt`

### Scripts/setup_bazel.py

The Python build configuration script provides a user-friendly interface for building with Bazel. It:

- Parses command-line arguments
- Generates appropriate Bazel commands
- Manages compiler and build tool selection
- Handles feature flag configuration
- Provides build timing information

## Advanced Configuration

### Custom Bazel Flags

You can pass custom Bazel flags directly:

```bash
# Increase verbosity
bazel build --verbose_failures //...

# Use specific number of parallel jobs
bazel build -j 16 //...

# Show build timing
bazel build --profile=/tmp/profile.txt //...

# Disable Bazel server (useful for CI/CD)
bazel build --noserver //...
```

### Environment Variables

```bash
# Set JAVA_HOME for Bazel
export JAVA_HOME=/path/to/java

# Set compiler path
export CC=/usr/bin/clang
export CXX=/usr/bin/clang++

# Set CUDA path
export CUDA_HOME=/usr/local/cuda
export PATH=$CUDA_HOME/bin:$PATH
```

### Bazel Query

```bash
# List all available targets
bazel query //...

# List all test targets
bazel query 'kind(cc_test, //...)'

# Show dependencies
bazel query 'deps(//Library/Core:Core)'

# Show reverse dependencies
bazel query 'rdeps(//..., //Library/Core:Core)'
```

## Flag Definitions Reference

### Preprocessor Defines

When you enable a feature flag, XSigma automatically sets corresponding preprocessor defines:

| Flag | Enabled Define | Disabled Define |
|------|---|---|
| `enable_cuda` | `XSIGMA_ENABLE_CUDA`, `XSIGMA_HAS_CUDA=1` | `XSIGMA_HAS_CUDA=0` |
| `enable_hip` | `XSIGMA_ENABLE_HIP`, `XSIGMA_HAS_HIP=1` | `XSIGMA_HAS_HIP=0` |
| `enable_kineto` | `XSIGMA_ENABLE_KINETO`, `XSIGMA_HAS_KINETO=1` | `XSIGMA_HAS_KINETO=0` |
| `enable_tbb` | `XSIGMA_HAS_TBB` | (none) |
| `enable_mimalloc` | `XSIGMA_ENABLE_MIMALLOC` | (none) |
| `enable_magic_enum` | `XSIGMA_ENABLE_MAGICENUM` | (none) |
| `enable_openmp` | `XSIGMA_ENABLE_OPENMP` | (none) |

### Configuration Settings

Configuration settings in `bazel/BUILD.bazel` define how flags are matched:

```python
config_setting(
    name = "enable_cuda",
    define_values = {"xsigma_enable_cuda": "true"},
)
```

These are referenced in `select()` statements throughout BUILD files:

```python
select({
    "//bazel:enable_cuda": ["XSIGMA_ENABLE_CUDA", "XSIGMA_HAS_CUDA=1"],
    "//conditions:default": ["XSIGMA_HAS_CUDA=0"],
})
```

## Continuous Integration

### GitHub Actions Example

```yaml
name: Bazel Build

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - uses: bazelbuild/setup-bazel@v2
      - name: Build
        run: python Scripts/setup_bazel.py build.test.release
      - name: Run Tests
        run: bazel test //...
```

### Local CI Testing

```bash
# Run full test suite
python Scripts/setup_bazel.py build.test.release

# Run specific test
bazel test //Library/Core/Testing/Cxx:CoreCxxTests

# Run with verbose output
bazel test --test_output=all //Library/Core/Testing/Cxx:CoreCxxTests
```

## Performance Optimization

### Build Optimization

```bash
# Use release build with LTO and vectorization
python Scripts/setup_bazel.py build.release.lto.avx2

# Use remote caching (if available)
bazel build --remote_cache=grpc://cache-server:9092 //...

# Use local disk cache
bazel build --disk_cache=/tmp/bazel-cache //...
```

### Test Optimization

```bash
# Run tests in parallel
bazel test -j 8 //...

# Run only changed tests
bazel test --test_filter='*Changed*' //...

# Skip expensive tests
bazel test --test_tag_filters=-expensive //...
```

## See Also

- [Bazel Documentation](https://bazel.build/docs)
- [Bazelisk Documentation](https://github.com/bazelbuild/bazelisk)
- [XSigma Bazel Structure](BAZEL_STRUCTURE.md)
- [XSigma Bazel Build Examples](BAZEL_BUILD.md)
- [XSigma CMake Configuration](../Cmake/README.md)

