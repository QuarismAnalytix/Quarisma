# Quarisma SMP Performance Analysis and Usage Recommendations

**Document Version:** 1.0
**Date:** 2025-11-02
**Status:** Preliminary Analysis

---

## Executive Summary

This document provides a comprehensive performance analysis comparing Quarisma's two parallel execution systems:
- **`smp`** - Legacy parallel execution framework (VTK-based, supports STDThread and TBB backends)
- **`smp_new`** - Modern parallel execution framework (PyTorch-inspired, supports Native, OpenMP, and TBB backends)

### Key Findings

| Metric | smp | smp_new | Winner |
|--------|-----|---------|--------|
| **API Simplicity** | Complex (functor-based) | Simple (lambda-friendly) | ✅ smp_new |
| **Feature Completeness** | Transform, Fill, Sort, For | parallel_for, parallel_reduce, parallelize_1d | ✅ smp |
| **Backend Flexibility** | STDThread, TBB | Native, OpenMP, TBB | ✅ smp_new |
| **Work Stealing** | No | Yes (Native backend) | ✅ smp_new |
| **Nested Parallelism** | Limited | Full support | ✅ smp_new |
| **Thread Pool Management** | Global | Intra-op + Inter-op pools | ✅ smp_new |

---

## 1. Architecture Comparison

### 1.1 smp Module Architecture

**Design Philosophy:** VTK-inspired parallel utilities with backend abstraction

**Key Components:**
- `tools` - Main API class with static methods
- `tools_impl<BackendType>` - Backend-specific implementations
- `tools_api` - Singleton managing backend selection
- Backends: `STDThread` (default), `TBB` (optional)

**Strengths:**
- ✅ Mature, battle-tested codebase
- ✅ Rich API: `For`, `Transform`, `Fill`, `Sort`
- ✅ Iterator-based operations
- ✅ Functor support with `Initialize()` and `Reduce()` methods

**Weaknesses:**
- ❌ Complex functor-based API (less lambda-friendly)
- ❌ No built-in work-stealing
- ❌ Limited nested parallelism support
- ❌ Single global thread pool

**Example Usage:**
```cpp
#include "smp/tools.h"

std::vector<int> data(1000, 0);

// Parallel for loop
tools::For(0, 1000, 100, [&data](int begin, int end) {
    for (int i = begin; i < end; ++i) {
        data[i] = i * 2;
    }
});

// Transform operation
std::vector<int> input(1000), output(1000);
tools::Transform(input.begin(), input.end(), output.begin(),
    [](int x) { return x * 2; });

// Fill operation
tools::Fill(data.begin(), data.end(), 42);

// Sort operation
tools::Sort(data.begin(), data.end());
```

### 1.2 smp_new Module Architecture

**Design Philosophy:** PyTorch-inspired parallel execution with modern C++ features

**Key Components:**
- `parallel_api` - Main API with `parallel_for`, `parallel_reduce`, `parallelize_1d`
- `TaskThreadPoolBase` - Abstract thread pool interface
- `ThreadPool` - Concrete implementation with work-stealing
- Backends: `Native` (default), `OpenMP`, `TBB`
- Dual thread pools: Intra-op (computation) + Inter-op (task parallelism)

**Strengths:**
- ✅ Modern, lambda-friendly API
- ✅ Work-stealing scheduler (Native backend)
- ✅ Dual thread pool architecture (intra-op + inter-op)
- ✅ Full nested parallelism support
- ✅ NUMA-aware thread binding
- ✅ Exception propagation
- ✅ Lazy initialization

**Weaknesses:**
- ❌ Newer codebase (less battle-tested)
- ❌ Missing convenience APIs (Transform, Fill, Sort)
- ❌ No iterator-based operations

**Example Usage:**
```cpp
#include "smp_new/parallel/parallel_api.h"

using namespace quarisma::smp_new::parallel;

std::vector<int> data(1000, 0);

// Parallel for loop
parallel_for(0, 1000, 100, [&data](int64_t begin, int64_t end) {
    for (int64_t i = begin; i < end; ++i) {
        data[i] = static_cast<int>(i * 2);
    }
});

// Parallel reduce
int64_t sum = parallel_reduce(
    0, 1000, 100, 0LL,
    [&data](int64_t begin, int64_t end, int64_t init) -> int64_t {
        int64_t local_sum = init;
        for (int64_t i = begin; i < end; ++i) {
            local_sum += data[i];
        }
        return local_sum;
    },
    [](int64_t a, int64_t b) -> int64_t { return a + b; });

// Work-stealing parallel execution
parallelize_1d([&data](size_t i) { data[i] = i * 2; }, 1000);
```

---

## 2. Performance Characteristics

### 2.1 Workload Classification

#### Memory-Bound Workloads
**Characteristics:**
- Simple operations (array writes, copies)
- Performance limited by memory bandwidth
- Low CPU utilization per thread
- Cache effects dominate

**Examples:**
- Array initialization: `data[i] = i`
- Memory copies: `output[i] = input[i]`
- Simple transformations: `data[i] = data[i] * 2`

#### Compute-Bound Workloads
**Characteristics:**
- Heavy mathematical operations
- Performance limited by CPU throughput
- High CPU utilization per thread
- Less sensitive to cache effects

**Examples:**
- Trigonometric functions: `sin(x) * cos(x)`
- Square roots: `sqrt(x)`
- Complex calculations: `x^2 + 2x + 1`

#### Mixed Workloads
**Characteristics:**
- Combination of memory and compute operations
- Balanced CPU and memory usage
- Realistic application scenarios

**Examples:**
- Financial calculations with data access
- Matrix operations
- Statistical computations

### 2.2 Data Size Impact

| Size Category | Range | Characteristics | Recommended Approach |
|---------------|-------|-----------------|---------------------|
| **Small** | 100-1,000 | Overhead dominates | Sequential or coarse-grained parallelism |
| **Medium** | 10,000-100,000 | Sweet spot for parallelism | Medium grain size (100-1,000) |
| **Large** | 1,000,000+ | Excellent parallelism | Coarse grain size (10,000+) |

### 2.3 Grain Size Tuning

**Grain Size** = Number of items processed per parallel task

| Grain Size | Use Case | Pros | Cons |
|------------|----------|------|------|
| **Fine (1-100)** | Small datasets, load balancing | Better load distribution | High overhead |
| **Medium (100-1,000)** | General purpose | Balanced overhead/parallelism | May underutilize on large data |
| **Coarse (1,000-10,000)** | Large datasets, compute-bound | Low overhead | Poor load balancing |
| **Auto (0)** | Unknown workload | Adaptive | May not be optimal |

**Grain Size Formula (smp):**
```cpp
// STDThread backend auto grain calculation
int threadNumber = GetNumberOfThreads();
int estimateGrain = (last - first) / (threadNumber * 4);
grain = (estimateGrain > 0) ? estimateGrain : 1;
```

**Grain Size Formula (smp_new):**
```cpp
// Native backend: User-specified or default
// Work-stealing handles load balancing automatically
```

---

## 3. Benchmark Results

### 3.1 Test Environment

- **Platform:** macOS ARM64 (Apple Silicon)
- **Compiler:** Clang 21.1.2
- **C++ Standard:** C++17
- **Build Type:** Release with coverage instrumentation
- **Hardware Concurrency:** Variable (detected at runtime)

### 3.2 Memory-Bound Performance

#### Small Dataset (100 items)

| Operation | smp (μs) | smp_new (μs) | Speedup | Winner |
|-----------|----------|--------------|---------|--------|
| Array Write | TBD | TBD | TBD | TBD |
| Overhead | TBD | TBD | TBD | TBD |

**Analysis:** For small datasets, overhead dominates. Sequential execution may be faster.

#### Medium Dataset (10,000 items)

| Operation | smp (μs) | smp_new (μs) | Speedup | Winner |
|-----------|----------|--------------|---------|--------|
| Array Write | TBD | TBD | TBD | TBD |
| Throughput (items/sec) | TBD | TBD | TBD | TBD |

**Analysis:** Medium datasets show the benefits of parallelism.

#### Large Dataset (1,000,000 items)

| Operation | smp (μs) | smp_new (μs) | Speedup | Winner |
|-----------|----------|--------------|---------|--------|
| Array Write | TBD | TBD | TBD | TBD |
| Throughput (items/sec) | TBD | TBD | TBD | TBD |
| Scalability | TBD | TBD | TBD | TBD |

**Analysis:** Large datasets maximize parallel efficiency.

### 3.3 Compute-Bound Performance

#### Small Dataset (1,000 items, heavy math)

| Operation | smp (μs) | smp_new (μs) | Speedup | Winner |
|-----------|----------|--------------|---------|--------|
| sin/cos/sqrt | TBD | TBD | TBD | TBD |

**Analysis:** Compute-bound workloads benefit more from parallelism even at smaller sizes.

#### Medium Dataset (50,000 items, heavy math)

| Operation | smp (μs) | smp_new (μs) | Speedup | Winner |
|-----------|----------|--------------|---------|--------|
| sin/cos/sqrt | TBD | TBD | TBD | TBD |

**Analysis:** Optimal parallelism for compute-intensive operations.

### 3.4 Reduction Performance

| Dataset Size | smp (μs) | smp_new (μs) | Speedup | Winner |
|--------------|----------|--------------|---------|--------|
| 1,000 | TBD | TBD | TBD | TBD |
| 1,000,000 | TBD | TBD | TBD | TBD |

**Analysis:** `smp_new::parallel_reduce` provides built-in reduction support, while `smp` requires manual implementation.

### 3.5 Transform Performance

| Dataset Size | smp (μs) | smp_new (μs) | Speedup | Winner |
|--------------|----------|--------------|---------|--------|
| 1,000 | TBD | TBD | TBD | TBD |

**Analysis:** `smp::Transform` is a convenience API not available in `smp_new`.

---

## 4. Usage Recommendations

### 4.1 Decision Tree

```
START
  |
  ├─ Need Transform/Fill/Sort convenience APIs?
  |    YES → Use smp
  |    NO  → Continue
  |
  ├─ Need parallel_reduce?
  |    YES → Use smp_new
  |    NO  → Continue
  |
  ├─ Need nested parallelism?
  |    YES → Use smp_new
  |    NO  → Continue
  |
  ├─ Need work-stealing for load balancing?
  |    YES → Use smp_new (Native backend)
  |    NO  → Continue
  |
  ├─ Dataset size < 1,000?
  |    YES → Consider sequential execution
  |    NO  → Continue
  |
  ├─ Prefer modern lambda-based API?
  |    YES → Use smp_new
  |    NO  → Use smp
```

### 4.2 Scenario-Based Recommendations

#### Scenario 1: Simple Array Operations (Memory-Bound)
**Workload:** Initialize arrays, copy data, simple transformations
**Data Size:** 10,000 - 1,000,000 items
**Recommendation:** **smp** with `Transform` or `Fill` APIs
**Rationale:**
- Convenience APIs reduce boilerplate
- Performance difference minimal for memory-bound operations
- Simpler code maintenance

**Example:**
```cpp
// smp - Recommended
std::vector<int> data(100000);
tools::Fill(data.begin(), data.end(), 42);

// smp_new - More verbose
parallel_for(0, 100000, 1000, [&data](int64_t begin, int64_t end) {
    for (int64_t i = begin; i < end; ++i) {
        data[i] = 42;
    }
});
```

#### Scenario 2: Heavy Mathematical Computations (Compute-Bound)
**Workload:** Trigonometric functions, complex calculations
**Data Size:** 1,000 - 1,000,000 items
**Recommendation:** **smp_new** with Native backend
**Rationale:**
- Work-stealing provides better load balancing
- Lower overhead for compute-intensive tasks
- Better scalability with thread count

**Example:**
```cpp
// smp_new - Recommended
parallel_for(0, size, 100, [&data](int64_t begin, int64_t end) {
    for (int64_t i = begin; i < end; ++i) {
        double x = static_cast<double>(i) * 0.01;
        data[i] = std::sin(x) * std::cos(x) + std::sqrt(x + 1.0);
    }
});
```

#### Scenario 3: Reduction Operations
**Workload:** Sum, product, min/max, count
**Data Size:** Any
**Recommendation:** **smp_new** with `parallel_reduce`
**Rationale:**
- Built-in reduction support
- Thread-safe by design
- No manual synchronization required

**Example:**
```cpp
// smp_new - Recommended
int64_t sum = parallel_reduce(
    0, size, 1000, 0LL,
    [&data](int64_t begin, int64_t end, int64_t init) -> int64_t {
        int64_t local_sum = init;
        for (int64_t i = begin; i < end; ++i) {
            local_sum += data[i];
        }
        return local_sum;
    },
    [](int64_t a, int64_t b) -> int64_t { return a + b; });

// smp - Requires manual synchronization
std::atomic<int64_t> sum{0};
tools::For(0, size, 1000, [&data, &sum](int begin, int end) {
    int64_t local_sum = 0;
    for (int i = begin; i < end; ++i) {
        local_sum += data[i];
    }
    sum.fetch_add(local_sum, std::memory_order_relaxed);
});
```

#### Scenario 4: Sorting Large Datasets
**Workload:** Sort arrays
**Data Size:** 10,000+ items
**Recommendation:** **smp** with `Sort` API
**Rationale:**
- Optimized parallel sort implementation
- TBB backend uses `tbb::parallel_sort`
- No equivalent in smp_new

**Example:**
```cpp
// smp - Recommended
std::vector<int> data(1000000);
// ... fill data ...
tools::Sort(data.begin(), data.end());

// smp_new - No built-in parallel sort
// Must use std::sort (sequential) or implement manually
```

#### Scenario 5: Nested Parallelism
**Workload:** Parallel operations within parallel operations
**Data Size:** Any
**Recommendation:** **smp_new** with nested parallelism enabled
**Rationale:**
- Dual thread pool architecture (intra-op + inter-op)
- Prevents thread pool exhaustion
- Better resource management

**Example:**
```cpp
// smp_new - Recommended
parallel_for(0, outer_size, 10, [](int64_t i_begin, int64_t i_end) {
    for (int64_t i = i_begin; i < i_end; ++i) {
        // Nested parallel operation
        parallel_for(0, inner_size, 10, [i](int64_t j_begin, int64_t j_end) {
            for (int64_t j = j_begin; j < j_end; ++j) {
                // Process (i, j)
            }
        });
    }
});
```

#### Scenario 6: Small Datasets (< 1,000 items)
**Workload:** Any
**Data Size:** < 1,000 items
**Recommendation:** **Sequential execution** or **smp_new** with large grain size
**Rationale:**
- Parallel overhead exceeds benefits
- Sequential code is simpler and faster
- If parallelism required, use coarse grain to minimize overhead

**Example:**
```cpp
// Sequential - Recommended for small datasets
for (int i = 0; i < 100; ++i) {
    data[i] = i * 2;
}

// If parallelism required, use large grain
parallel_for(0, 100, 100, [&data](int64_t begin, int64_t end) {
    for (int64_t i = begin; i < end; ++i) {
        data[i] = static_cast<int>(i * 2);
    }
});
```

### 4.3 Backend Selection Guidelines

#### smp Backends

| Backend | When to Use | Pros | Cons |
|---------|-------------|------|------|
| **STDThread** (default) | General purpose, no TBB dependency | Portable, no external dependencies | No work-stealing, limited nested parallelism |
| **TBB** | High-performance computing, nested parallelism | Work-stealing, excellent scalability | External dependency, larger binary |

**Configuration:**
```cpp
// Set backend
tools::SetBackend("STDThread");  // or "TBB"

// Initialize with thread count
tools::Initialize(8);  // Use 8 threads

// Enable nested parallelism (TBB only)
tools::SetNestedParallelism(true);
```

#### smp_new Backends

| Backend | When to Use | Pros | Cons |
|---------|-------------|------|------|
| **Native** (default) | General purpose, work-stealing needed | Work-stealing, dual pools, NUMA support | Newer codebase |
| **OpenMP** | Integration with MKL, existing OpenMP code | Industry standard, MKL integration | Requires OpenMP support |
| **TBB** | Maximum performance, nested parallelism | Proven scalability, task-based | External dependency |

**Configuration:**
```cpp
using namespace quarisma::smp_new::parallel;

// Set backend
set_backend(0);  // 0=Native, 1=OpenMP, 2=Auto, 3=TBB

// Set thread counts
set_num_intraop_threads(8);  // Computation threads
set_num_interop_threads(4);  // Task parallelism threads
```

---

## 5. Migration Guide

### 5.1 Migrating from smp to smp_new

#### Parallel For Loop

**Before (smp):**
```cpp
#include "smp/tools.h"

tools::For(0, size, grain, [&data](int begin, int end) {
    for (int i = begin; i < end; ++i) {
        data[i] = i * 2;
    }
});
```

**After (smp_new):**
```cpp
#include "smp_new/parallel/parallel_api.h"

using namespace quarisma::smp_new::parallel;

parallel_for(0, size, grain, [&data](int64_t begin, int64_t end) {
    for (int64_t i = begin; i < end; ++i) {
        data[i] = static_cast<int>(i * 2);
    }
});
```

**Key Changes:**
- Include path: `smp/tools.h` → `smp_new/parallel/parallel_api.h`
- Function: `tools::For` → `parallel_for`
- Index type: `int` → `int64_t`
- Namespace: `quarisma` → `quarisma::smp_new::parallel`

#### Transform Operation

**Before (smp):**
```cpp
tools::Transform(input.begin(), input.end(), output.begin(),
    [](int x) { return x * 2; });
```

**After (smp_new):**
```cpp
// No direct equivalent - use parallel_for
parallel_for(0, static_cast<int64_t>(input.size()), grain,
    [&input, &output](int64_t begin, int64_t end) {
        for (int64_t i = begin; i < end; ++i) {
            output[i] = input[i] * 2;
        }
    });
```

#### Fill Operation

**Before (smp):**
```cpp
tools::Fill(data.begin(), data.end(), 42);
```

**After (smp_new):**
```cpp
// No direct equivalent - use parallel_for
parallel_for(0, static_cast<int64_t>(data.size()), grain,
    [&data](int64_t begin, int64_t end) {
        for (int64_t i = begin; i < end; ++i) {
            data[i] = 42;
        }
    });
```

#### Sort Operation

**Before (smp):**
```cpp
tools::Sort(data.begin(), data.end());
```

**After (smp_new):**
```cpp
// No parallel sort in smp_new - use std::sort or implement manually
std::sort(data.begin(), data.end());
```

### 5.2 Migrating from smp_new to smp

Generally not recommended unless you need:
- `Transform`, `Fill`, or `Sort` convenience APIs
- Compatibility with existing VTK-based code

---

## 6. Best Practices

### 6.1 General Guidelines

1. **Profile Before Parallelizing**
   - Measure sequential performance first
   - Identify bottlenecks with profiling tools
   - Ensure parallel overhead is justified

2. **Choose Appropriate Grain Size**
   - Start with auto grain (0) for unknown workloads
   - Tune based on profiling results
   - Larger grain for memory-bound, smaller for compute-bound

3. **Avoid False Sharing**
   - Ensure threads write to separate cache lines
   - Use padding or thread-local storage when necessary

4. **Minimize Synchronization**
   - Use lock-free algorithms when possible
   - Prefer `parallel_reduce` over manual atomic operations
   - Batch updates to reduce contention

5. **Test Scalability**
   - Measure performance with varying thread counts
   - Check for diminishing returns
   - Monitor CPU utilization

### 6.2 Common Pitfalls

#### Pitfall 1: Over-Parallelizing Small Datasets

**Problem:**
```cpp
// Overhead exceeds benefits
parallel_for(0, 10, 1, [&data](int64_t begin, int64_t end) {
    for (int64_t i = begin; i < end; ++i) {
        data[i] = i;
    }
});
```

**Solution:**
```cpp
// Use sequential execution
for (int i = 0; i < 10; ++i) {
    data[i] = i;
}
```

#### Pitfall 2: Race Conditions in Reductions

**Problem:**
```cpp
// Race condition!
int sum = 0;
tools::For(0, size, grain, [&sum, &data](int begin, int end) {
    for (int i = begin; i < end; ++i) {
        sum += data[i];  // Multiple threads writing to sum
    }
});
```

**Solution:**
```cpp
// Use parallel_reduce
int sum = parallel_reduce(
    0, size, grain, 0,
    [&data](int64_t begin, int64_t end, int init) -> int {
        int local_sum = init;
        for (int64_t i = begin; i < end; ++i) {
            local_sum += data[i];
        }
        return local_sum;
    },
    [](int a, int b) -> int { return a + b; });
```

#### Pitfall 3: Excessive Grain Size

**Problem:**
```cpp
// Only one thread does all the work
parallel_for(0, 1000, 1000, [&data](int64_t begin, int64_t end) {
    for (int64_t i = begin; i < end; ++i) {
        data[i] = expensive_computation(i);
    }
});
```

**Solution:**
```cpp
// Use smaller grain for better load distribution
parallel_for(0, 1000, 100, [&data](int64_t begin, int64_t end) {
    for (int64_t i = begin; i < end; ++i) {
        data[i] = expensive_computation(i);
    }
});
```

---

## 7. Future Work

### 7.1 Planned Enhancements

**smp_new:**
- [ ] Add `Transform`, `Fill`, `Sort` convenience APIs
- [ ] Implement parallel sort algorithm
- [ ] Add iterator-based operations
- [ ] Improve auto grain size heuristics
- [ ] Add performance monitoring/profiling hooks

**smp:**
- [ ] Add work-stealing support to STDThread backend
- [ ] Improve nested parallelism support
- [ ] Add dual thread pool architecture
- [ ] Modernize API to be more lambda-friendly

### 7.2 Benchmark Execution

To run the comprehensive benchmarks:

```bash
cd build_ninja_coverage
./bin/CoreCxxBenchmark --benchmark_filter="BM_SMP.*|BM_SMPNew.*"
```

To run specific benchmark categories:

```bash
# Memory-bound benchmarks
./bin/CoreCxxBenchmark --benchmark_filter=".*MemoryBound.*"

# Compute-bound benchmarks
./bin/CoreCxxBenchmark --benchmark_filter=".*ComputeBound.*"

# Reduction benchmarks
./bin/CoreCxxBenchmark --benchmark_filter=".*Reduce.*"

# Transform benchmarks
./bin/CoreCxxBenchmark --benchmark_filter=".*Transform.*"
```

---

## 8. Conclusion

Both `smp` and `smp_new` are viable parallel execution frameworks with different strengths:

**Use `smp` when:**
- You need convenience APIs (`Transform`, `Fill`, `Sort`)
- You're working with existing VTK-based code
- You prefer a mature, battle-tested codebase

**Use `smp_new` when:**
- You need `parallel_reduce` functionality
- You require nested parallelism
- You want work-stealing for better load balancing
- You prefer modern, lambda-friendly APIs
- You need dual thread pool architecture

**General Recommendation:**
- **New projects:** Start with `smp_new` for modern features and better scalability
- **Existing projects:** Continue with `smp` unless specific `smp_new` features are needed
- **Performance-critical code:** Benchmark both and choose based on results

---

## Appendix A: API Reference

### smp API Summary

```cpp
// Initialization
tools::Initialize(num_threads);
tools::SetBackend("STDThread" | "TBB");
tools::SetNestedParallelism(bool);

// Parallel operations
tools::For(first, last, grain, functor);
tools::For(begin_iter, end_iter, grain, functor);
tools::Transform(in_begin, in_end, out_begin, transform_func);
tools::Transform(in1_begin, in1_end, in2_begin, out_begin, binary_func);
tools::Fill(begin, end, value);
tools::Sort(begin, end);
tools::Sort(begin, end, comparator);

// Queries
const char* backend = tools::GetBackend();
int threads = tools::GetEstimatedNumberOfThreads();
bool nested = tools::GetNestedParallelism();
bool parallel = tools::IsParallelScope();
```

### smp_new API Summary

```cpp
using namespace quarisma::smp_new::parallel;

// Configuration
set_backend(0|1|2|3);  // Native|OpenMP|Auto|TBB
set_num_intraop_threads(num);
set_num_interop_threads(num);

// Parallel operations
parallel_for(begin, end, grain_size, func);
T result = parallel_reduce(begin, end, grain_size, identity, map_func, reduce_func);
parallelize_1d(func, range, flags);

// Task launching
launch(task_func);
intraop_launch(task_func);

// Queries
int backend = get_backend();
int threads = get_num_intraop_threads();
bool in_parallel = in_parallel_region();
```

---

**Document End**
