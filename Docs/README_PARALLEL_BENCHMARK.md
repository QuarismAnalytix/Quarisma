# XSigma SMP Backend Benchmark Guide

## Overview

This guide explains how to run comprehensive benchmarks comparing all four PARALLEL (Symmetric Multi-Processing) execution modes in XSigma:

1. **Sequential execution** - No parallelism, single thread
2. **`--parallel.std`** - C++ standard library threading (std::thread)
3. **`--parallel.openmp`** - OpenMP backend
4. **`--parallel.tbb`** - Intel TBB (Threading Building Blocks) backend

## Benchmark File

**Location**: `Library/Core/Testing/Cxx/BenchmarkParallelBackends.cpp`

**Executable**: `bin/benchmark_parallelbackends` (after building)

## Workloads

The benchmark includes seven computationally heavy workloads designed to demonstrate clear performance differences between sequential and parallel execution:

### 1. **Numerical Integration** (Simpson's Rule)
- Integrates f(x) = sin(x) * exp(-x/10)
- Tests floating-point intensive operations
- Workload sizes: 1K - 500K iterations

### 2. **Dense Matrix Multiplication** (O(n³))
- Small matrix multiplications repeated many times
- Tests cache-friendly computation patterns
- Matrix sizes: 16x16, repeated 10K+ times

### 3. **Prime Number Checking** (Trial Division)
- Tests irregular workload with variable computation time
- CPU-intensive with unpredictable patterns
- Range: 10K - 20K+ numbers

### 4. **Monte Carlo Pi Estimation**
- Random number generation and floating-point operations
- Each sample is independent (embarrassingly parallel)
- Sample sizes: 5M - 10M+ samples

### 5. **Image Convolution** (2D Gaussian Filter)
- 5x5 kernel applied to 2D image data
- Tests local memory access patterns
- Image sizes: 512x512 - 1024x1024 pixels

### 6. **Large Matrix Multiplication**
- Dense matrix multiplication C = A * B
- Tests scalability with large datasets
- Matrix sizes: 128x128 - 256x256

### 7. **Statistical Computations**
- Parallel reduction for sum and sum-of-squares
- Tests structured reduction operations
- Dataset sizes: 1M - 50M elements

## Building and Running Benchmarks

### Method 1: Using setup.py (Recommended)

#### Step 1: Build with std::thread backend

```bash
# Configure for std::thread backend
python3 Scripts/setup.py config --parallel.std

# Build with benchmarks enabled
python3 Scripts/setup.py config.build

# Run the benchmark and save results
./bin/benchmark_parallelbackends --benchmark_out=results_std.json --benchmark_out_format=json
./bin/benchmark_parallelbackends --benchmark_out=results_std.txt --benchmark_out_format=console > results_std.txt
```

#### Step 2: Build with OpenMP backend

```bash
# Clean previous build
python3 Scripts/setup.py clean

# Configure for OpenMP backend
python3 Scripts/setup.py config --parallel.openmp

# Build
python3 Scripts/setup.py config.build

# Run and save results
./bin/benchmark_parallelbackends --benchmark_out=results_openmp.json --benchmark_out_format=json
./bin/benchmark_parallelbackends > results_openmp.txt
```

#### Step 3: Build with Intel TBB backend

```bash
# Clean previous build
python3 Scripts/setup.py clean

# Configure for TBB backend
python3 Scripts/setup.py config --parallel.tbb

# Build
python3 Scripts/setup.py config.build

# Run and save results
./bin/benchmark_parallelbackends --benchmark_out=results_tbb.json --benchmark_out_format=json
./bin/benchmark_parallelbackends > results_tbb.txt
```

### Method 2: Using CMake directly

```bash
# Build with std::thread backend
mkdir -p build_std && cd build_std
cmake -DXSIGMA_PARALLEL_BACKEND=std -DXSIGMA_ENABLE_BENCHMARK=ON ..
cmake --build . --target benchmark_parallelbackends
./bin/benchmark_parallelbackends > ../results_std.txt

# Build with OpenMP backend
cd ..
mkdir -p build_openmp && cd build_openmp
cmake -DXSIGMA_PARALLEL_BACKEND=openmp -DXSIGMA_ENABLE_BENCHMARK=ON ..
cmake --build . --target benchmark_parallelbackends
./bin/benchmark_parallelbackends > ../results_openmp.txt

# Build with TBB backend
cd ..
mkdir -p build_tbb && cd build_tbb
cmake -DXSIGMA_PARALLEL_BACKEND=tbb -DXSIGMA_ENABLE_BENCHMARK=ON ..
cmake --build . --target benchmark_parallelbackends
./bin/benchmark_parallelbackends > ../results_tbb.txt
```

## Benchmark Options

### Running Specific Benchmarks

```bash
# Run only matrix multiplication benchmarks
./bin/benchmark_parallelbackends --benchmark_filter=MatrixMult

# Run only sequential benchmarks
./bin/benchmark_parallelbackends --benchmark_filter=Sequential

# Run only parallel benchmarks
./bin/benchmark_parallelbackends --benchmark_filter=Parallel
```

### Controlling Benchmark Runtime

```bash
# Run with minimum time of 5 seconds per benchmark
./bin/benchmark_parallelbackends --benchmark_min_time=5.0

# Run with specific number of iterations
./bin/benchmark_parallelbackends --benchmark_repetitions=10
```

### Output Formats

```bash
# JSON output (for programmatic analysis)
./bin/benchmark_parallelbackends --benchmark_out_format=json --benchmark_out=results.json

# CSV output (for spreadsheet import)
./bin/benchmark_parallelbackends --benchmark_out_format=csv --benchmark_out=results.csv

# Console output with color
./bin/benchmark_parallelbackends --benchmark_color=true
```

## Interpreting Results

### Output Format

Each benchmark reports:
- **Time** - Average execution time per iteration (in ns, μs, or ms)
- **CPU** - Total CPU time consumed
- **Iterations** - Number of times the benchmark was run
- **ItemsProcessed** - Throughput metric (items/second)
- **Label** - Indicates "Sequential" or "Parallel_T{num_threads}"

### Example Output

```
------------------------------------------------------------------------
Benchmark                              Time             CPU   Iterations
------------------------------------------------------------------------
BM_MatrixMult_Sequential/128        4235 ms         4228 ms            1   Sequential
BM_MatrixMult_Parallel/128/2        2187 ms         4361 ms            1   Parallel_T2
BM_MatrixMult_Parallel/128/4        1134 ms         4518 ms            1   Parallel_T4
BM_MatrixMult_Parallel/128/8         612 ms         4876 ms            1   Parallel_T8
```

### Calculating Speedup

**Speedup** = Sequential Time / Parallel Time

From the example above:
- **2 threads**: 4235 / 2187 = 1.94x speedup
- **4 threads**: 4235 / 1134 = 3.73x speedup
- **8 threads**: 4235 / 612 = 6.92x speedup

### Efficiency

**Parallel Efficiency** = Speedup / Number of Threads

From the example above:
- **2 threads**: 1.94 / 2 = 97% efficient
- **4 threads**: 3.73 / 4 = 93% efficient
- **8 threads**: 6.92 / 8 = 87% efficient

## Comparing Backends

### Automated Comparison Script

Create a script `compare_backends.sh`:

```bash
#!/bin/bash

# Array of backends to test
BACKENDS=("std" "openmp" "tbb")

for backend in "${BACKENDS[@]}"; do
    echo "========================================="
    echo "Testing $backend backend"
    echo "========================================="

    # Clean and configure
    python3 Scripts/setup.py clean
    python3 Scripts/setup.py config --parallel.$backend

    # Build
    python3 Scripts/setup.py config.build

    # Run benchmark
    ./bin/benchmark_parallelbackends \
        --benchmark_out=results_${backend}.json \
        --benchmark_out_format=json

    # Also save console output
    ./bin/benchmark_parallelbackends > results_${backend}.txt
done

echo "All backends tested. Results saved to results_*.json and results_*.txt"
```

Run with:
```bash
chmod +x compare_backends.sh
./compare_backends.sh
```

### Analysis Tools

#### Python Analysis Script

```python
import json
import pandas as pd

# Load results from all backends
backends = ['std', 'openmp', 'tbb']
results = {}

for backend in backends:
    with open(f'results_{backend}.json', 'r') as f:
        results[backend] = json.load(f)['benchmarks']

# Extract specific benchmark for comparison
def get_benchmark_time(backend, name):
    for bench in results[backend]:
        if bench['name'] == name:
            return bench['real_time']
    return None

# Compare specific workload across backends
workload = 'BM_MatrixMult_Parallel/128/4'
print(f"\nComparison for {workload}:")
print("-" * 60)
for backend in backends:
    time = get_benchmark_time(backend, workload)
    print(f"{backend:10s}: {time:10.2f} ms")
```

## Performance Tips

### 1. System Configuration

```bash
# Disable CPU frequency scaling for consistent results
sudo cpupower frequency-set --governor performance

# Set thread affinity to physical cores
export OMP_PROC_BIND=true
export OMP_PLACES=cores
```

### 2. Benchmark Isolation

```bash
# Run with elevated priority
sudo nice -n -20 ./bin/benchmark_parallelbackends

# Disable turbo boost for consistent results
echo 1 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo
```

### 3. Backend-Specific Tuning

#### OpenMP
```bash
export OMP_NUM_THREADS=8
export OMP_PROC_BIND=spread
export OMP_PLACES=threads
```

#### TBB
```bash
export TBB_NUM_THREADS=8
```

#### std::thread
```bash
export PARALLEL_MAX_THREADS=8
```

## Expected Results

### Typical Speedup Patterns

**Highly Parallel Workloads** (Monte Carlo, Image Convolution):
- Near-linear speedup up to physical core count
- 4 threads → ~3.8-3.9x speedup
- 8 threads → ~7.2-7.8x speedup

**CPU-Bound Workloads** (Matrix Multiplication, Integration):
- Good speedup with some overhead
- 4 threads → ~3.2-3.6x speedup
- 8 threads → ~5.5-6.8x speedup

**Irregular Workloads** (Prime Checking):
- Variable speedup depending on load balancing
- 4 threads → ~2.8-3.4x speedup
- 8 threads → ~4.5-6.0x speedup

### Backend Differences

**OpenMP**:
- Generally fastest for simple parallel loops
- Excellent static load balancing
- Lowest overhead for large grain sizes

**Intel TBB**:
- Best for irregular workloads
- Dynamic work stealing handles load imbalance
- Slightly higher overhead but better scalability

**std::thread**:
- Predictable performance
- Good for all workload types
- No external dependencies

## Troubleshooting

### Build Issues

**Problem**: Benchmark target not found
```bash
ninja: error: unknown target 'benchmark_parallelbackends'
```

**Solution**: Ensure benchmarks are enabled
```bash
python3 Scripts/setup.py config --enable-benchmark
python3 Scripts/setup.py config.build
```

**Problem**: File excluded during build
```
Excluding legacy SMP benchmark (files being consolidated): BenchmarkSMPBackends.cpp
```

**Solution**: The file was renamed to `BenchmarkParallelBackends.cpp` to avoid this exclusion.

### Runtime Issues

**Problem**: Low speedup with many threads

**Possible Causes**:
1. Grain size too small (excessive overhead)
2. Memory bandwidth limitation
3. Cache contention
4. Hyperthreading vs physical cores

**Solution**: Adjust grain size and thread count to match physical cores.

## Contributing

To add new workloads to the benchmark:

1. Add the workload function in the `benchmark_detail` namespace
2. Create sequential and parallel benchmark functions
3. Register benchmarks with appropriate parameters
4. Update this README with the new workload description

## License

SPDX-License-Identifier: GPL-3.0-or-later OR Commercial

See file header in BenchmarkParallelBackends.cpp for full license information.

## Contact

For questions or issues:
- GitHub Issues: https://github.com/xsigma/xsigma/issues
- Documentation: https://docs.xsigma.co.uk
