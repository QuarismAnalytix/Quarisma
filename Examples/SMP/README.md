# Quarisma SMP (Symmetric Multi-Processing) Examples

This directory contains comprehensive examples demonstrating the usage of Quarisma's parallel APIs for symmetric multi-processing (SMP) on shared-memory systems.

## Overview

The Quarisma SMP module provides high-performance parallel APIs for multi-threaded computation:

- **`parallelize_1d()`**: Work-stealing based 1D parallelization with minimal overhead
- **`parallel_for()`**: Chunk-based parallel iteration over ranges
- **`parallel_reduce()`**: Efficient parallel reduction operations
- **Thread Configuration**: Backend selection and thread pool management

## Examples

### 1. `example_parallelize_1d.cxx`

Demonstrates high-performance 1D data-parallel work distribution using work-stealing load balancing.

**Key Features:**
- Basic vector initialization in parallel
- Matrix row-wise operations
- Image processing simulation (pixel-wise operations)
- Uneven workload distribution with work-stealing
- Data reduction operations with atomic operations

**Use Cases:**
- Large-scale data processing
- Element-wise operations on vectors/arrays
- Pixel-wise image processing
- Uneven workload scenarios

**Performance:**
- Minimal overhead (~0.1-0.2 μs per work item)
- Excellent load balancing through work-stealing
- Blocking execution (waits for all work to complete)

### 2. `example_parallel_for.cxx`

Demonstrates chunk-based parallel iteration with configurable grain sizes.

**Key Features:**
- Basic parallel iteration with automatic grain size
- Grain size tuning for performance optimization
- Chunk-based matrix processing
- Nested parallelism (outer and inner parallel loops)
- Parallel reduction within parallel_for

**Use Cases:**
- Range-based processing
- Chunk-based computations
- Nested parallel regions
- Aggregation operations

**Performance Tuning:**
- Small grain sizes: Better load balancing, higher overhead
- Large grain sizes: Lower overhead, potential load imbalance
- Automatic grain size: Recommended for most cases

### 3. `example_parallel_reduce.cxx`

Demonstrates efficient parallel reduction operations for aggregations and statistics.

**Key Features:**
- Basic sum reduction
- Min/Max reduction
- Custom reduction with structs (statistics computation)
- Vector reduction (element-wise operations)
- Histogram computation

**Use Cases:**
- Computing aggregations (sum, min, max)
- Statistical analysis
- Combining results from multiple threads
- Histogram and frequency analysis

**Reduction Pattern:**
```cpp
result = parallel_reduce(
    begin, end, initial_value,
    [](int64_t begin, int64_t end, T init) {
        // Compute local reduction
        return local_result;
    },
    [](T a, T b) {
        // Combine two results
        return combined_result;
    }
);
```

### 4. `example_thread_configuration.cxx`

Demonstrates thread pool configuration, backend selection, and performance tuning.

**Key Features:**
- Query current configuration (backend, thread count)
- Initialize thread pool with specific thread count
- Performance comparison with different thread counts
- Nested parallelism control
- Single-threaded mode for debugging
- Backend selection

**Configuration Options:**
- Thread count: Automatic or explicit
- Nested parallelism: Enable/disable
- Single-threaded mode: For debugging race conditions
- Backend selection: STDThread, TBB, etc. (if available)

## Building Examples

### Option 1: Using setup.py (Recommended)

```bash
# Build with Ninja + Clang
python setup.py config.build.examples.ninja.clang

# Build with Visual Studio 2022
python setup.py config.build.examples.vs22

# Build with other configurations
python setup.py config.build.examples.xcode
python setup.py config.build.examples.gcc
```

### Option 2: Manual CMake Configuration

```bash
# Configure with examples enabled
cmake -DQUARISMA_ENABLE_EXAMPLES=ON -B build

# Build
cmake --build build

# Run examples
./build/bin/example_parallelize_1d
./build/bin/example_parallel_for
./build/bin/example_parallel_reduce
./build/bin/example_thread_configuration
```

## Running Examples

After building, run individual examples:

```bash
# Run parallelize_1d example
./build/bin/example_parallelize_1d

# Run parallel_for example
./build/bin/example_parallel_for

# Run parallel_reduce example
./build/bin/example_parallel_reduce

# Run thread configuration example
./build/bin/example_thread_configuration
```

## Performance Tips

### 1. Choosing the Right API

- **`parallelize_1d()`**: Use for fine-grained, uniform work distribution
- **`parallel_for()`**: Use for chunk-based processing with configurable grain sizes
- **`parallel_reduce()`**: Use for aggregation and reduction operations

### 2. Grain Size Tuning

For `parallel_for()`:
- Small grain sizes (1000-10000): Better load balancing, higher overhead
- Large grain sizes (100000+): Lower overhead, potential imbalance
- Automatic (-1): Recommended for most cases

### 3. Thread Count

- Optimal thread count ≈ number of CPU cores
- Use `GetEstimatedNumberOfThreads()` to query available threads
- Explicit initialization: `Initialize(num_threads)`

### 4. Nested Parallelism

- Enable for nested parallel regions: `SetNestedParallelism(true)`
- Disable for sequential outer loops: `SetNestedParallelism(false)`
- Can significantly impact performance

### 5. Debugging

- Use single-threaded mode: `Initialize(1)`
- Helps identify race conditions and synchronization issues
- Disable nested parallelism for simpler debugging

## Real-World Use Cases

### Data Processing
```cpp
// Process large dataset in parallel
parallelize_1d([&data](size_t i) {
    data[i] = process(data[i]);
}, data.size());
```

### Matrix Operations
```cpp
// Parallel matrix multiplication
parallel_for(0, rows, grain_size, [&](int64_t begin, int64_t end) {
    for (int64_t i = begin; i < end; ++i) {
        // Compute row i
    }
});
```

### Statistical Analysis
```cpp
// Compute statistics in parallel
auto stats = parallel_reduce(
    0, data.size(), Statistics(),
    [&data](int64_t begin, int64_t end, Statistics init) {
        // Compute local statistics
        return init;
    },
    [](const Statistics& a, const Statistics& b) {
        // Combine statistics
        return combined;
    }
);
```

## Cross-Platform Compatibility

All examples are designed to work on:
- **Windows**: Visual Studio 2022, Clang
- **Linux**: GCC, Clang
- **macOS**: Clang, Xcode

## Requirements

- C++17 or later
- Quarisma Core library
- Multi-core processor (for meaningful parallelism)

## Further Reading

- [Quarisma SMP Documentation](../../Docs/smp/)
- [Parallel API Reference](../../Library/Core/smp_new/parallel/)
- [Thread Configuration Guide](../../Library/Core/smp/)

## Contributing

To add new examples:
1. Create a new `.cxx` file in this directory
2. Follow the naming convention: `example_*.cxx`
3. Add the file to `CMakeLists.txt`
4. Update this README with a description
5. Ensure cross-platform compatibility

## License

These examples are part of the Quarisma project and follow the same license terms.
