# Asynchronous Parallel Operations Design Document

**Author**: Claude Sonnet 4.5
**Date**: 2025-12-08
**Status**: Design Proposal
**Version**: 1.0

## Executive Summary

This document presents a comprehensive design for adding asynchronous versions of `parallel_for` and `parallel_reduce` to the XSigma parallel module. The design covers API signatures, backend compatibility, thread pool integration, error handling, and implementation strategies that align with XSigma's no-exception policy and coding standards.

---

## 1. Current Architecture Analysis

### 1.1 Existing Parallel Module Structure

The XSigma parallel module provides:

- **Synchronous Operations**: `parallel_for()` and `parallel_reduce()`
- **Three Backend Implementations**:
  - **OpenMP Backend**: Uses `#pragma omp parallel` with static work distribution
  - **TBB Backend**: Uses `tbb::parallel_for` and `tbb::parallel_reduce` with work-stealing
  - **Native Backend**: Custom thread pool with task queue and manual synchronization
- **Two Parallelism Types**:
  - **Intra-op Parallelism**: Within operations (parallel_for/reduce) - controlled by `set_num_threads()`
  - **Inter-op Parallelism**: Between operations via `launch()` - controlled by `set_num_interop_threads()`

### 1.2 Key Design Characteristics

- **No Exceptions Policy**: Uses `XSIGMA_CHECK` macros and return values
- **RAII Patterns**: `parallel_guard` and `thread_id_guard` for state management
- **Nested Parallelism Prevention**: `in_parallel_region()` prevents nested parallel calls
- **Thread-Local State**: Thread ID and parallel region flags are thread-local
- **Conditional Compilation**: Backend selection via `XSIGMA_HAS_OPENMP` and `XSIGMA_HAS_TBB`

### 1.3 Thread Pool Architecture

**Native Backend**:
- Separate thread pools for intra-op and inter-op parallelism
- `_get_intraop_pool()`: Singleton intra-op thread pool (size = `num_threads - 1`)
- `get_interop_pool()`: Singleton inter-op thread pool (configurable size)
- Producer-consumer pattern with mutex and condition variables

**OpenMP/TBB Backends**:
- No separate inter-op pool; `launch()` executes inline
- Thread management delegated to OpenMP/TBB runtime

---

## 2. API Design

### 2.1 Proposed Function Signatures

```cpp
namespace xsigma {

/**
 * @brief Future-like handle for asynchronous parallel operations
 *
 * Provides a type-safe, no-exception interface for async results.
 * Compatible with XSigma's no-exception policy.
 *
 * @tparam T Result type (void for async_parallel_for)
 */
template <typename T>
class async_handle {
public:
    /**
     * @brief Checks if the async operation has completed
     * @return true if completed, false if still running
     * Thread Safety: Thread-safe
     */
    bool is_ready() const noexcept;

    /**
     * @brief Waits for the async operation to complete
     * Blocks until the operation finishes
     * Thread Safety: Thread-safe
     */
    void wait() const noexcept;

    /**
     * @brief Waits for the operation with a timeout
     * @param timeout_ms Timeout in milliseconds
     * @return true if completed within timeout, false otherwise
     * Thread Safety: Thread-safe
     */
    bool wait_for(int64_t timeout_ms) const noexcept;

    /**
     * @brief Gets the result (blocking if not ready)
     * @return Result value, or default-constructed T on error
     * Thread Safety: Thread-safe, but only one thread should call get()
     *
     * @note For void specialization, this just waits
     * @note If an error occurred, returns default-constructed T
     *       Use has_error() to check for errors
     */
    T get() noexcept;

    /**
     * @brief Checks if an error occurred during execution
     * @return true if error occurred, false otherwise
     * Thread Safety: Thread-safe
     */
    bool has_error() const noexcept;

    /**
     * @brief Gets error message if an error occurred
     * @return Error message string, empty if no error
     * Thread Safety: Thread-safe
     */
    std::string get_error() const noexcept;
};

/**
 * @brief Asynchronous version of parallel_for
 *
 * Launches a parallel for loop that executes asynchronously.
 * Returns immediately with an async_handle that can be used to
 * wait for completion or check status.
 *
 * @tparam F Callable type with signature void(int64_t begin, int64_t end)
 *
 * @param begin Starting index (inclusive)
 * @param end Ending index (exclusive)
 * @param grain_size Minimum elements per chunk
 * @param f Function to execute on each chunk
 *
 * @return async_handle<void> for tracking completion
 *
 * Thread Safety: Thread-safe
 * Backend: All backends (OpenMP, TBB, Native)
 *
 * Behavior:
 * - Does NOT prevent nested parallelism check (async operations are independent)
 * - Uses inter-op thread pool for native backend
 * - For OpenMP/TBB: uses std::async or creates new parallel region
 *
 * Example:
 * @code
 * auto handle = xsigma::async_parallel_for(0, 1000, 100,
 *     [&data](int64_t begin, int64_t end) {
 *         for (int64_t i = begin; i < end; ++i) {
 *             data[i] = expensive_computation(i);
 *         }
 *     });
 *
 * // Do other work...
 *
 * handle.wait();  // Wait for completion
 * if (handle.has_error()) {
 *     XSIGMA_LOG_ERROR("Async operation failed: {}", handle.get_error());
 * }
 * @endcode
 */
template <class F>
async_handle<void> async_parallel_for(
    int64_t begin,
    int64_t end,
    int64_t grain_size,
    const F& f);

/**
 * @brief Asynchronous version of parallel_reduce
 *
 * Launches a parallel reduction that executes asynchronously.
 * Returns immediately with an async_handle containing the future result.
 *
 * @tparam scalar_t Result type
 * @tparam F Reduction function: scalar_t(int64_t begin, int64_t end, scalar_t identity)
 * @tparam SF Combine function: scalar_t(scalar_t a, scalar_t b)
 *
 * @param begin Starting index (inclusive)
 * @param end Ending index (exclusive)
 * @param grain_size Minimum elements per chunk
 * @param ident Identity element
 * @param f Reduction function
 * @param sf Combine function (must be associative)
 *
 * @return async_handle<scalar_t> containing the reduction result
 *
 * Thread Safety: Thread-safe
 * Backend: All backends (OpenMP, TBB, Native)
 *
 * Example:
 * @code
 * auto handle = xsigma::async_parallel_reduce(
 *     0, 10000, 2500, 0,
 *     [&data](int64_t begin, int64_t end, int identity) {
 *         int partial_sum = identity;
 *         for (int64_t i = begin; i < end; ++i) {
 *             partial_sum += data[i];
 *         }
 *         return partial_sum;
 *     },
 *     [](int a, int b) { return a + b; }
 * );
 *
 * // Do other work...
 *
 * int sum = handle.get();  // Blocks until ready
 * if (handle.has_error()) {
 *     XSIGMA_LOG_ERROR("Reduction failed: {}", handle.get_error());
 * }
 * @endcode
 */
template <class scalar_t, class F, class SF>
async_handle<scalar_t> async_parallel_reduce(
    int64_t begin,
    int64_t end,
    int64_t grain_size,
    scalar_t ident,
    const F& f,
    const SF& sf);

}  // namespace xsigma
```

### 2.2 Design Rationale

**Why not `std::future<T>`?**
- `std::future` uses exceptions for error reporting (`std::future::get()` can throw)
- XSigma has a no-exception policy
- Custom `async_handle` provides exception-free interface

**Why separate from synchronous versions?**
- Clear API distinction between sync and async operations
- Async operations have different threading behavior (bypass nested parallelism check)
- Allows optimization opportunities specific to async execution

**Why bypass nested parallelism check?**
- Async operations are launched intentionally for concurrent execution
- User explicitly wants multiple parallel operations running simultaneously
- Nested parallelism check is for accidental nesting within single operation

---

## 3. Backend Compatibility Analysis

### 3.1 OpenMP Backend

**Capabilities**:
- OpenMP 3.0+ supports task-based parallelism
- Can create independent parallel regions from different threads
- `#pragma omp task` can be used for async operations

**Implementation Strategy**:
```cpp
template <typename F>
async_handle<void> async_parallel_for_impl(int64_t begin, int64_t end,
                                            int64_t grain_size, const F& f) {
    auto shared_state = std::make_shared<async_state<void>>();

    // Launch on inter-op thread pool (or std::async)
    std::thread([=]() {
        try {
            // Create new parallel region (independent of caller's region)
            parallel_for(begin, end, grain_size, f);
            shared_state->set_ready();
        } catch (...) {
            shared_state->set_error("Async parallel_for failed");
        }
    }).detach();

    return async_handle<void>(shared_state);
}
```

**Considerations**:
- OpenMP parallel regions are independent; no conflict with async execution
- Thread affinity may be lost when launching from different thread
- Need to ensure OpenMP thread pool is available in launched thread

### 3.2 TBB Backend

**Capabilities**:
- TBB's task arena allows isolated parallel execution
- `tbb::task_group` provides async task execution
- Work-stealing scheduler handles concurrent parallel operations efficiently

**Implementation Strategy**:
```cpp
template <typename F>
async_handle<void> async_parallel_for_impl(int64_t begin, int64_t end,
                                            int64_t grain_size, const F& f) {
    auto shared_state = std::make_shared<async_state<void>>();

    // Use TBB task group for async execution
    auto task_group = std::make_shared<tbb::task_group>();
    task_group->run([=]() {
        try {
            parallel_for(begin, end, grain_size, f);
            shared_state->set_ready();
        } catch (...) {
            shared_state->set_error("Async parallel_for failed");
        }
    });

    return async_handle<void>(shared_state, task_group);
}
```

**Considerations**:
- TBB handles nested parallelism gracefully
- Task arena ensures isolated execution
- Best async support among all backends

### 3.3 Native Backend

**Capabilities**:
- Full control over thread pool
- Separate inter-op thread pool already exists
- Can implement custom async execution

**Implementation Strategy**:
```cpp
template <typename F>
async_handle<void> async_parallel_for_impl(int64_t begin, int64_t end,
                                            int64_t grain_size, const F& f) {
    auto shared_state = std::make_shared<async_state<void>>();

    // Submit to inter-op thread pool
    launch([=]() {
        try {
            // Execute parallel_for on this thread
            // It will use intra-op thread pool for parallelism
            parallel_for(begin, end, grain_size, f);
            shared_state->set_ready();
        } catch (...) {
            shared_state->set_error("Async parallel_for failed");
        }
    });

    return async_handle<void>(shared_state);
}
```

**Considerations**:
- Natural fit with existing inter-op thread pool
- Async operations use inter-op pool, which then uses intra-op pool
- Two-level thread pool hierarchy works well for async

### 3.4 Backend Comparison Summary

| Feature | OpenMP | TBB | Native |
|---------|--------|-----|--------|
| Async Support | Manual (std::thread) | Native (task_group) | Natural (inter-op pool) |
| Nested Parallelism | Safe (independent regions) | Excellent (work stealing) | Safe (two-level pools) |
| Thread Pool Conflict | None | None | None |
| Implementation Complexity | Medium | Low | Low |
| Performance | Good | Excellent | Good |
| Recommendation | ✓ Supported | ✓ Preferred | ✓ Supported |

**All backends can support async operations effectively.**

---

## 4. Thread Pool Integration

### 4.1 Native Backend Thread Pool Usage

**Two-Level Hierarchy**:
```
Main Thread
    ├─ Inter-op Pool (for async operations)
    │   ├─ Worker 1 → launches intra-op parallel work
    │   ├─ Worker 2 → launches intra-op parallel work
    │   └─ Worker 3 → launches intra-op parallel work
    └─ Intra-op Pool (for sync parallel operations)
        ├─ Worker 1
        ├─ Worker 2
        └─ Worker 3
```

**Async Operation Flow**:
1. User calls `async_parallel_for()` from main thread
2. Task submitted to **inter-op thread pool**
3. Inter-op worker picks up task and calls `parallel_for()`
4. `parallel_for()` uses **intra-op thread pool** for parallel execution
5. When done, inter-op worker marks async_handle as ready

**Advantages**:
- Clear separation: async uses inter-op, sync uses intra-op
- No thread pool exhaustion (separate pools)
- Natural async execution model

**Potential Issues**:
- **Thread Pool Exhaustion**: If all inter-op threads launch async operations that need intra-op threads
- **Deadlock Risk**: If async operation waits for another async operation

**Mitigation**:
- Document limitation: "Do not call `wait()` on async_handle from within async operation"
- Recommend appropriate inter-op pool size (typically smaller than intra-op)
- Provide `async_handle::wait_for()` with timeout to detect deadlocks

### 4.2 OpenMP/TBB Backend Thread Usage

**OpenMP**:
- Async operations launch on `std::thread` or reuse current thread
- Each async operation creates independent OpenMP parallel region
- No shared thread pool (OpenMP manages threads internally)

**TBB**:
- Async operations use `tbb::task_group`
- TBB's scheduler manages all tasks in unified thread pool
- Work-stealing prevents thread starvation

**No special configuration needed for OpenMP/TBB backends.**

### 4.3 Nested Parallelism Interactions

**Current Behavior** (synchronous):
```cpp
parallel_for(0, 100, 10, [](int64_t b, int64_t e) {
    // in_parallel_region() == true
    parallel_for(0, 10, 1, [](int64_t b2, int64_t e2) {
        // Executes SEQUENTIALLY (nested parallelism prevented)
    });
});
```

**Async Behavior** (proposed):
```cpp
auto handle = async_parallel_for(0, 100, 10, [](int64_t b, int64_t e) {
    // in_parallel_region() == true (still checked internally)
    // But async launch happens from outside parallel region
});

// Multiple concurrent async operations
auto h1 = async_parallel_for(...);
auto h2 = async_parallel_for(...);  // Runs concurrently with h1
auto h3 = async_parallel_reduce(...);  // Also runs concurrently
```

**Key Design Decision**:
- Async operations **bypass** the `in_parallel_region()` check at launch time
- Inside each async operation, normal nested parallelism prevention applies
- This allows concurrent async operations while preventing accidental nesting

---

## 5. Error Handling Design

### 5.1 XSigma No-Exception Policy

XSigma uses:
- `XSIGMA_CHECK(condition, message)` - throws `xsigma::Error` exception
- `XSIGMA_THROW(message)` - throws exception
- Exceptions used for unrecoverable errors, not control flow

**Problem**: `std::future::get()` throws exceptions, incompatible with no-exception preference for APIs.

### 5.2 Proposed Error Handling Mechanism

**`async_handle` Error Interface**:
```cpp
template <typename T>
class async_handle {
private:
    std::shared_ptr<async_state<T>> state_;

public:
    // Non-throwing interface
    bool has_error() const noexcept { return state_->has_error(); }
    std::string get_error() const noexcept { return state_->error_msg(); }

    // get() returns default-constructed T on error
    T get() noexcept {
        wait();
        if (has_error()) {
            return T{};  // Return default value
        }
        return state_->get_value();
    }
};
```

**Internal State**:
```cpp
template <typename T>
class async_state {
private:
    std::mutex mtx_;
    std::condition_variable cv_;
    bool ready_ = false;
    bool has_error_ = false;
    std::string error_msg_;
    T value_;  // Only valid if ready_ && !has_error_

public:
    void set_ready() {
        std::lock_guard<std::mutex> lock(mtx_);
        ready_ = true;
        cv_.notify_all();
    }

    void set_error(std::string msg) {
        std::lock_guard<std::mutex> lock(mtx_);
        has_error_ = true;
        error_msg_ = std::move(msg);
        ready_ = true;
        cv_.notify_all();
    }

    void set_value(T val) {
        std::lock_guard<std::mutex> lock(mtx_);
        value_ = std::move(val);
        ready_ = true;
        cv_.notify_all();
    }
};
```

### 5.3 Error Propagation Strategy

**Within Async Operation**:
```cpp
template <typename F>
async_handle<void> async_parallel_for_impl(...) {
    auto state = std::make_shared<async_state<void>>();

    launch([=]() {
        try {
            parallel_for(begin, end, grain_size, f);
            state->set_ready();
        } catch (const xsigma::Error& e) {
            // Catch XSigma errors
            state->set_error(std::string("XSigma error: ") + e.what());
        } catch (const std::exception& e) {
            // Catch standard exceptions
            state->set_error(std::string("Exception: ") + e.what());
        } catch (...) {
            // Catch everything else
            state->set_error("Unknown error occurred");
        }
    });

    return async_handle<void>(state);
}
```

**User-Side Error Handling**:
```cpp
// Pattern 1: Check after wait
auto handle = async_parallel_for(...);
handle.wait();
if (handle.has_error()) {
    XSIGMA_LOG_ERROR("Async operation failed: {}", handle.get_error());
    // Handle error...
}

// Pattern 2: Check before get
auto handle = async_parallel_reduce(...);
auto result = handle.get();  // Returns default-constructed T on error
if (handle.has_error()) {
    XSIGMA_LOG_ERROR("Reduction failed: {}", handle.get_error());
    // Handle error...
}

// Pattern 3: Timeout with error check
auto handle = async_parallel_for(...);
if (!handle.wait_for(5000)) {  // 5 second timeout
    XSIGMA_LOG_ERROR("Async operation timed out");
} else if (handle.has_error()) {
    XSIGMA_LOG_ERROR("Async operation failed: {}", handle.get_error());
}
```

### 5.4 Alternative: `std::expected` (C++23)

**Future Enhancement** (when C++23 is adopted):
```cpp
// C++23 version
template <typename T>
using async_handle = std::expected<T, std::string>;

auto handle = async_parallel_reduce(...);
auto result = handle.get();  // Returns std::expected<int, std::string>

if (result.has_value()) {
    int sum = result.value();
} else {
    XSIGMA_LOG_ERROR("Error: {}", result.error());
}
```

**Recommendation**: Start with custom `async_handle`, migrate to `std::expected` when C++23 is available.

---

## 6. Use Cases and Examples

### 6.1 Use Case 1: Concurrent Matrix Operations

**Scenario**: Launch multiple independent matrix multiplications concurrently.

```cpp
#include "parallel/parallel.h"

void concurrent_matrix_operations(
    const std::vector<Matrix>& matrices_a,
    const std::vector<Matrix>& matrices_b,
    std::vector<Matrix>& results)
{
    std::vector<xsigma::async_handle<void>> handles;

    // Launch all matrix multiplications asynchronously
    for (size_t i = 0; i < matrices_a.size(); ++i) {
        auto handle = xsigma::async_parallel_for(
            0, matrices_a[i].rows(), 64,
            [&, i](int64_t row_begin, int64_t row_end) {
                for (int64_t row = row_begin; row < row_end; ++row) {
                    for (int64_t col = 0; col < matrices_b[i].cols(); ++col) {
                        double sum = 0.0;
                        for (int64_t k = 0; k < matrices_a[i].cols(); ++k) {
                            sum += matrices_a[i](row, k) * matrices_b[i](k, col);
                        }
                        results[i](row, col) = sum;
                    }
                }
            });
        handles.push_back(std::move(handle));
    }

    // Wait for all operations to complete
    for (auto& handle : handles) {
        handle.wait();
        if (handle.has_error()) {
            XSIGMA_LOG_ERROR("Matrix operation failed: {}", handle.get_error());
        }
    }
}
```

**Benefits**:
- All matrix operations run concurrently
- Each operation internally uses parallel_for for row-level parallelism
- Total time = max(individual operation times) instead of sum

### 6.2 Use Case 2: Pipelined Data Processing

**Scenario**: Process data in stages where each stage can start as soon as some data is ready.

```cpp
#include "parallel/parallel.h"

struct ProcessingPipeline {
    std::vector<double> stage1_output;
    std::vector<double> stage2_output;
    std::vector<double> stage3_output;

    void process(const std::vector<double>& input) {
        // Stage 1: Preprocessing (async)
        auto h1 = xsigma::async_parallel_for(
            0, input.size(), 1000,
            [&](int64_t begin, int64_t end) {
                for (int64_t i = begin; i < end; ++i) {
                    stage1_output[i] = preprocess(input[i]);
                }
            });

        // Wait for stage 1 to complete
        h1.wait();

        // Stage 2: Main computation (async)
        auto h2 = xsigma::async_parallel_for(
            0, stage1_output.size(), 1000,
            [&](int64_t begin, int64_t end) {
                for (int64_t i = begin; i < end; ++i) {
                    stage2_output[i] = compute(stage1_output[i]);
                }
            });

        // Stage 3: Can start as soon as stage 2 completes
        auto h3 = xsigma::async_parallel_for(
            0, stage2_output.size(), 1000,
            [&](int64_t begin, int64_t end) {
                for (int64_t i = begin; i < end; ++i) {
                    stage3_output[i] = postprocess(stage2_output[i]);
                }
            });

        // Wait for final stage
        h2.wait();
        h3.wait();
    }
};
```

**Benefits**:
- Clear pipeline structure
- Each stage uses parallelism internally
- Easy to add stages or modify pipeline

### 6.3 Use Case 3: Parallel Reductions with Different Operations

**Scenario**: Compute multiple statistics (sum, max, min) on same dataset concurrently.

```cpp
#include "parallel/parallel.h"

struct DataStatistics {
    double sum;
    double max_val;
    double min_val;
    double mean;
};

DataStatistics compute_statistics(const std::vector<double>& data) {
    // Launch sum reduction
    auto sum_handle = xsigma::async_parallel_reduce(
        0, data.size(), 1000, 0.0,
        [&data](int64_t begin, int64_t end, double identity) {
            double partial_sum = identity;
            for (int64_t i = begin; i < end; ++i) {
                partial_sum += data[i];
            }
            return partial_sum;
        },
        [](double a, double b) { return a + b; }
    );

    // Launch max reduction
    auto max_handle = xsigma::async_parallel_reduce(
        0, data.size(), 1000, std::numeric_limits<double>::lowest(),
        [&data](int64_t begin, int64_t end, double identity) {
            double partial_max = identity;
            for (int64_t i = begin; i < end; ++i) {
                partial_max = std::max(partial_max, data[i]);
            }
            return partial_max;
        },
        [](double a, double b) { return std::max(a, b); }
    );

    // Launch min reduction
    auto min_handle = xsigma::async_parallel_reduce(
        0, data.size(), 1000, std::numeric_limits<double>::max(),
        [&data](int64_t begin, int64_t end, double identity) {
            double partial_min = identity;
            for (int64_t i = begin; i < end; ++i) {
                partial_min = std::min(partial_min, data[i]);
            }
            return partial_min;
        },
        [](double a, double b) { return std::min(a, b); }
    );

    // Collect results
    DataStatistics stats;
    stats.sum = sum_handle.get();
    stats.max_val = max_handle.get();
    stats.min_val = min_handle.get();
    stats.mean = stats.sum / data.size();

    // Check for errors
    if (sum_handle.has_error() || max_handle.has_error() || min_handle.has_error()) {
        XSIGMA_LOG_ERROR("Statistics computation failed");
    }

    return stats;
}
```

**Benefits**:
- Three reductions run concurrently (instead of sequentially)
- Each reduction internally parallelized
- ~3x speedup compared to sequential reductions

### 6.4 Use Case 4: Background Computation

**Scenario**: Launch expensive computation in background while doing other work.

```cpp
#include "parallel/parallel.h"

void process_with_background_computation() {
    std::vector<double> data(10000000);
    std::vector<double> results(10000000);

    // Launch expensive computation in background
    auto handle = xsigma::async_parallel_for(
        0, data.size(), 10000,
        [&](int64_t begin, int64_t end) {
            for (int64_t i = begin; i < end; ++i) {
                results[i] = expensive_computation(data[i]);
            }
        });

    // Do other work while computation runs
    prepare_output_file();
    allocate_resources();
    validate_inputs();

    // Wait for computation to finish
    handle.wait();

    if (!handle.has_error()) {
        save_results(results);
    } else {
        XSIGMA_LOG_ERROR("Background computation failed: {}", handle.get_error());
    }
}
```

**Benefits**:
- Overlap computation with I/O or other non-computational work
- Better CPU utilization
- Reduced total execution time

---

## 7. Performance Considerations

### 7.1 Overhead Analysis

**Additional Overhead of Async Operations**:

1. **Shared State Allocation** (~100-200 bytes)
   - `std::shared_ptr<async_state<T>>`
   - Mutex, condition variable, result storage
   - **Impact**: Negligible (one-time allocation)

2. **Thread Launch Overhead**
   - **Native**: Task submission to inter-op pool (~1-5 μs)
   - **TBB**: `task_group::run()` (~1-5 μs)
   - **OpenMP**: `std::thread` creation (~10-50 μs) or task spawn (~1-5 μs)
   - **Impact**: Small compared to parallel work (typically milliseconds)

3. **Synchronization Overhead**
   - `wait()`: Mutex lock + condition variable wait (~1-10 μs when ready)
   - `is_ready()`: Atomic load (~10-100 ns)
   - **Impact**: Negligible

4. **Exception Handling**
   - Try-catch block in async wrapper
   - **Impact**: Zero (no exceptions = no overhead)

**Total Overhead**: 10-100 μs per async operation

**Break-even Point**: Async operation worthwhile if work > 1 ms (typical parallel_for/reduce runs 10-1000 ms)

### 7.2 Scalability Considerations

**Concurrent Async Operations**:
- **Native Backend**: Limited by inter-op thread pool size (default: hardware_concurrency)
- **TBB Backend**: Excellent (work-stealing handles arbitrary concurrency)
- **OpenMP Backend**: Good (limited by OpenMP thread pool size)

**Recommended Limits**:
- Maximum concurrent async operations: `2 * num_interop_threads`
- If exceeded, operations queue (no failure, just waiting)

**Memory Usage**:
- Per async operation: ~200 bytes (async_state) + closure size
- 1000 concurrent operations: ~200 KB (acceptable)

### 7.3 Performance Best Practices

1. **Grain Size Selection**
   - Async operations: Use larger grain sizes (reduce task overhead)
   - Recommended: `grain_size >= total_work / (4 * num_threads)`

2. **Number of Async Operations**
   - Don't launch more concurrent async ops than inter-op threads
   - Use `std::vector<async_handle>` to batch operations

3. **Wait Strategy**
   - Avoid `wait()` immediately after launch (defeats purpose)
   - Launch multiple operations, then wait on all
   - Use `wait_for()` for timeout-based waiting

4. **Thread Pool Configuration**
   - Inter-op threads: Set to `hardware_concurrency / 2` for async workloads
   - Intra-op threads: Set to `hardware_concurrency` for maximum parallelism
   - Avoid: `num_interop_threads > num_intraop_threads` (can cause underutilization)

---

## 8. Implementation Plan

### 8.1 Phase 1: Core Infrastructure (Week 1-2)

**Files to Create**:
- `Library/Core/parallel/async_handle.h` - async_handle class template
- `Library/Core/parallel/async_state.h` - Internal state management

**Files to Modify**:
- `Library/Core/parallel/parallel.h` - Add async function declarations
- `Library/Core/parallel/parallel.cpp` - Add async function implementations

**Implementation Steps**:
1. Create `async_state<T>` class template with mutex/cv synchronization
2. Create `async_handle<T>` class template with wait/get/error methods
3. Add helper functions for backend-specific async launch
4. Write unit tests for async_handle

**Deliverables**:
- Working `async_handle` class with full API
- Unit tests passing
- Documentation

### 8.2 Phase 2: Async Parallel For (Week 3)

**Implementation Steps**:
1. Implement `async_parallel_for()` template in `parallel.h`
2. Add backend-specific implementations:
   - OpenMP: Use `std::thread` + `parallel_for`
   - TBB: Use `tbb::task_group` + `parallel_for`
   - Native: Use `launch()` + `parallel_for`
3. Add error handling and exception catching
4. Write unit tests and benchmarks

**Test Cases**:
- Empty range (begin >= end)
- Single element range
- Large range with various grain sizes
- Concurrent async operations
- Error propagation
- Timeout behavior

### 8.3 Phase 3: Async Parallel Reduce (Week 4)

**Implementation Steps**:
1. Implement `async_parallel_reduce()` template in `parallel.h`
2. Add result value handling in `async_state<T>`
3. Add backend-specific implementations (similar to async_parallel_for)
4. Write unit tests and benchmarks

**Test Cases**:
- Sum reduction
- Max/min reduction
- Custom reduction operations
- Concurrent reductions
- Error handling
- Result retrieval

### 8.4 Phase 4: Documentation and Examples (Week 5)

**Documentation**:
- API documentation in header files
- User guide with examples
- Performance tuning guide
- Migration guide (sync to async)

**Examples**:
- Matrix multiplication benchmark
- Statistics computation
- Pipelined data processing
- Background computation pattern

**Benchmarks**:
- Overhead measurement
- Scalability test (1-100 concurrent operations)
- Comparison with std::async
- Comparison with synchronous versions

### 8.5 Phase 5: Testing and Refinement (Week 6)

**Testing**:
- Integration tests with real workloads
- Thread safety testing (ThreadSanitizer)
- Memory leak testing (Valgrind/AddressSanitizer)
- Performance regression testing

**Refinement**:
- Optimize hot paths
- Improve error messages
- Add debug logging
- Performance tuning

---

## 9. Alternative Approaches Considered

### 9.1 Option A: Using `std::async`

**Approach**:
```cpp
template <typename F>
std::future<void> async_parallel_for(int64_t begin, int64_t end,
                                      int64_t grain_size, const F& f) {
    return std::async(std::launch::async, [=]() {
        parallel_for(begin, end, grain_size, f);
    });
}
```

**Pros**:
- Standard C++ facility
- Minimal implementation effort
- Well-tested and understood

**Cons**:
- **Exception-based error handling** (violates XSigma policy)
- Less control over thread management
- `std::async` may launch unlimited threads (implementation-defined)
- Difficult to integrate with XSigma thread pools

**Decision**: ❌ Rejected due to exception-based error handling

### 9.2 Option B: Callback-Based Async

**Approach**:
```cpp
template <typename F, typename Callback>
void async_parallel_for(int64_t begin, int64_t end, int64_t grain_size,
                        const F& f, Callback on_complete) {
    launch([=]() {
        parallel_for(begin, end, grain_size, f);
        on_complete();  // Call callback when done
    });
}
```

**Pros**:
- No need for future/handle abstraction
- Event-driven pattern familiar in some contexts

**Cons**:
- No way to wait for completion (blocks further work)
- Difficult to handle errors
- Callback hell for multiple async operations
- Not composable

**Decision**: ❌ Rejected due to poor composability and error handling

### 9.3 Option C: Task-Based API

**Approach**:
```cpp
class parallel_task {
public:
    parallel_task& then(std::function<void()> f);
    parallel_task& on_error(std::function<void(std::string)> f);
    void execute();
};

auto task = parallel_for_task(begin, end, grain_size, f)
    .then([]() { std::cout << "Done!\n"; })
    .on_error([](auto msg) { std::cerr << "Error: " << msg << "\n"; });
task.execute();
```

**Pros**:
- Composable task chains
- Fluent API
- Clear separation of concerns

**Cons**:
- Complex implementation
- Heavyweight abstraction
- Overkill for simple async operations
- Not needed for initial version

**Decision**: ❌ Rejected for initial version (could be future enhancement)

### 9.4 Selected Approach: Custom `async_handle`

**Why Selected**:
- ✅ No-exception error handling (compatible with XSigma)
- ✅ Simple, focused API
- ✅ Integrates well with XSigma thread pools
- ✅ Provides all necessary functionality (wait, get, error checking)
- ✅ Similar to familiar future/promise pattern
- ✅ Extensible to std::expected in future

---

## 10. Open Questions and Future Work

### 10.1 Open Questions

1. **Should async operations use separate thread pool or reuse intra-op pool?**
   - **Current Decision**: Use inter-op pool (separate)
   - **Rationale**: Clear separation, avoids conflicts
   - **Alternative**: Share pool (simpler but risk of exhaustion)

2. **Should we provide `when_all()` / `when_any()` for multiple async_handles?**
   - **Current Decision**: Defer to Phase 2
   - **Rationale**: Not essential for MVP, can be added later

3. **How to handle cancellation of async operations?**
   - **Current Decision**: Not supported in MVP
   - **Rationale**: Complex to implement correctly, rarely needed
   - **Future**: Add `async_handle::cancel()` if needed

4. **Should async operations be allowed from within parallel regions?**
   - **Current Decision**: Yes (no restriction)
   - **Rationale**: User knows what they're doing if launching async from parallel region

### 10.2 Future Enhancements

**Phase 2 Features**:
- `when_all(handles...)` - Wait for all handles
- `when_any(handles...)` - Wait for any handle
- `async_handle::cancel()` - Cancel running operation
- `async_handle::get_progress()` - Query progress

**Phase 3 Features**:
- `async_handle::then(f)` - Chain operations
- `parallel_task` abstraction for complex workflows
- Priority-based async execution
- NUMA-aware async execution

**C++23 Migration**:
- Replace `async_handle` with `std::expected<T, std::string>`
- Use `std::jthread` instead of `std::thread`
- Use C++23 `std::execution` policies

---

## 11. Conclusion and Recommendations

### 11.1 Summary

This design proposes adding asynchronous versions of `parallel_for` and `parallel_reduce` to XSigma with the following key features:

- **Custom `async_handle<T>`** for exception-free async results
- **Backend Compatibility**: Works with OpenMP, TBB, and Native backends
- **Thread Pool Integration**: Uses inter-op pool (native) or backend-specific async facilities
- **Error Handling**: No-exception interface compatible with XSigma policy
- **Performance**: Minimal overhead (~10-100 μs per operation)

### 11.2 Recommendations

1. **Approve this design** and proceed with implementation
2. **Start with Phase 1** (async_handle infrastructure) - 2 weeks
3. **Implement async_parallel_for first** (Phase 2) as proof of concept
4. **Add comprehensive tests** before merging to main branch
5. **Document performance characteristics** and best practices
6. **Consider TBB backend** as preferred for async workloads

### 11.3 Success Criteria

- ✅ All backends support async operations
- ✅ No exceptions used in async API
- ✅ Overhead < 100 μs per async operation
- ✅ Thread safety verified with sanitizers
- ✅ Documentation and examples complete
- ✅ Performance tests show expected concurrency benefits

### 11.4 Risks and Mitigation

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| Thread pool exhaustion | High | Medium | Document limits, add wait_for() with timeout |
| Deadlock in nested async | High | Low | Document restriction: no wait() in async |
| Performance overhead | Medium | Low | Benchmark early, optimize hot paths |
| Backend incompatibility | High | Low | Test all backends thoroughly |
| Error handling complexity | Medium | Medium | Simple error model, comprehensive tests |

---

## 12. References

### 12.1 Internal Documentation

- [parallel.h](../Library/Core/parallel/parallel.h) - Current parallel module API
- [thread_pool.h](../Library/Core/parallel/thread_pool.h) - Thread pool implementation
- [parallel_guard.h](../Library/Core/parallel/parallel_guard.h) - RAII guards

### 12.2 External References

- **OpenMP 5.0 Specification**: https://www.openmp.org/specifications/
- **Intel TBB Documentation**: https://www.intel.com/content/www/us/en/docs/onetbb/
- **C++ Concurrency in Action** by Anthony Williams (2nd Edition)
- **std::async documentation**: https://en.cppreference.com/w/cpp/thread/async
- **std::expected proposal**: https://wg21.link/p0323

### 12.3 Related Work

- **Boost.Fiber**: Fiber-based cooperative multitasking
- **Folly Futures**: Facebook's future/promise implementation
- **HPX**: High-performance parallelism library
- **executors proposal**: C++26 execution context

---

**Document Version**: 1.0
**Last Updated**: 2025-12-08
**Status**: Awaiting Review and Approval
**Next Steps**: Review by XSigma architecture team, feedback incorporation, implementation planning
