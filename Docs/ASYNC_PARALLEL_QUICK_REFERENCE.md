# Async Parallel Operations - Quick Reference

**For**: XSigma Developers
**Status**: Implementation Complete
**Date**: 2025-12-08

---

## Quick Start

### Include Header
```cpp
#include "parallel/parallel.h"  // Includes async_parallel_for/reduce
```

### Basic Usage
```cpp
// Async parallel for
auto handle = xsigma::async_parallel_for(0, 1000, 100,
    [](int64_t begin, int64_t end) {
        // Work on range [begin, end)
    });

// Wait for completion
handle.wait();
if (handle.has_error()) {
    // Handle error
}
```

---

## API Reference

### async_parallel_for
```cpp
template <class F>
async_handle<void> async_parallel_for(
    int64_t begin,        // Starting index (inclusive)
    int64_t end,          // Ending index (exclusive)
    int64_t grain_size,   // Chunk size
    const F& f            // Function: void(int64_t begin, int64_t end)
);
```

**Returns**: `async_handle<void>` for tracking completion

### async_parallel_reduce
```cpp
template <class scalar_t, class F, class SF>
async_handle<scalar_t> async_parallel_reduce(
    int64_t begin,        // Starting index
    int64_t end,          // Ending index
    int64_t grain_size,   // Chunk size
    scalar_t ident,       // Identity element
    const F& f,           // Reduction: scalar_t(int64_t, int64_t, scalar_t)
    const SF& sf          // Combine: scalar_t(scalar_t, scalar_t)
);
```

**Returns**: `async_handle<scalar_t>` containing result

### async_handle<T> Methods

| Method | Description |
|--------|-------------|
| `bool valid()` | Check if handle is valid |
| `bool is_ready()` | Non-blocking ready check |
| `void wait()` | Block until ready |
| `bool wait_for(int64_t ms)` | Wait with timeout (milliseconds) |
| `T get()` | Get result (blocks if not ready) |
| `bool has_error()` | Check if error occurred |
| `std::string get_error()` | Get error message |

---

## Common Patterns

### Pattern 1: Launch and Wait
```cpp
auto handle = xsigma::async_parallel_for(0, N, grain,
    [&](int64_t begin, int64_t end) { /* work */ });

handle.wait();  // Block until done
if (handle.has_error()) {
    XSIGMA_LOG_ERROR("Error: {}", handle.get_error());
}
```

### Pattern 2: Multiple Concurrent Operations
```cpp
auto h1 = xsigma::async_parallel_for(/*...*/);
auto h2 = xsigma::async_parallel_for(/*...*/);
auto h3 = xsigma::async_parallel_for(/*...*/);

// All run concurrently
h1.wait();
h2.wait();
h3.wait();
```

### Pattern 3: Reduction with Result
```cpp
auto handle = xsigma::async_parallel_reduce(
    0, N, grain, 0,
    [&](int64_t b, int64_t e, int id) {
        int sum = id;
        for (int64_t i = b; i < e; ++i) sum += data[i];
        return sum;
    },
    [](int a, int b) { return a + b; }
);

int result = handle.get();  // Blocks until ready
if (!handle.has_error()) {
    XSIGMA_LOG_INFO("Sum: {}", result);
}
```

### Pattern 4: Polling
```cpp
auto handle = xsigma::async_parallel_for(/*...*/);

while (!handle.is_ready()) {
    // Do other work
    do_other_work();
}

// Now ready
```

### Pattern 5: Timeout
```cpp
auto handle = xsigma::async_parallel_for(/*...*/);

if (!handle.wait_for(5000)) {  // 5 seconds
    XSIGMA_LOG_ERROR("Operation timed out");
} else if (handle.has_error()) {
    XSIGMA_LOG_ERROR("Error: {}", handle.get_error());
} else {
    XSIGMA_LOG_INFO("Success!");
}
```

### Pattern 6: Batch Processing
```cpp
std::vector<async_handle<void>> handles;

for (int i = 0; i < num_batches; ++i) {
    handles.emplace_back(xsigma::async_parallel_for(
        0, batch_size, grain,
        [&, i](int64_t b, int64_t e) { process(i, b, e); }
    ));
}

for (auto& h : handles) {
    h.wait();
}
```

### Pattern 7: Concurrent Statistics
```cpp
// Launch all reductions concurrently
auto sum_h = xsigma::async_parallel_reduce(/*sum*/);
auto max_h = xsigma::async_parallel_reduce(/*max*/);
auto min_h = xsigma::async_parallel_reduce(/*min*/);

// Get results (all computed in parallel)
double sum = sum_h.get();
double max = max_h.get();
double min = min_h.get();
```

---

## Performance Tips

### ✅ Do

1. **Use for operations > 1ms duration**
   - Overhead is ~50-100 μs
   - Break-even at ~1ms work

2. **Limit concurrent operations**
   - Max: ~2 * `num_interop_threads`
   - Default inter-op threads: `hardware_concurrency`

3. **Choose appropriate grain size**
   - Too small: overhead dominates
   - Too large: poor load balancing
   - Recommended: `total_work / (4 * num_threads)`

4. **Launch multiple ops then wait**
   ```cpp
   auto h1 = async_parallel_for(/*...*/);
   auto h2 = async_parallel_for(/*...*/);
   auto h3 = async_parallel_for(/*...*/);
   // All launch quickly, then:
   h1.wait(); h2.wait(); h3.wait();
   ```

### ❌ Don't

1. **Don't use for tiny operations**
   - Operation < 1ms? Use sync version

2. **Don't launch too many concurrent ops**
   - More than thread pool size = queuing delay

3. **Don't call wait() immediately**
   ```cpp
   // BAD:
   auto h = async_parallel_for(/*...*/);
   h.wait();  // No benefit over sync!

   // GOOD:
   auto h = async_parallel_for(/*...*/);
   do_other_work();  // Overlap computation
   h.wait();
   ```

4. **Don't ignore errors**
   ```cpp
   // ALWAYS check:
   auto h = async_parallel_for(/*...*/);
   h.wait();
   if (h.has_error()) {
       XSIGMA_LOG_ERROR("Error: {}", h.get_error());
   }
   ```

---

## Testing

### Run Tests
```bash
# Build tests
cmake -DXSIGMA_BUILD_TESTS=ON ..
make TestAsyncParallel

# Run
./TestAsyncParallel
```

### Run Benchmarks
```bash
# Build benchmarks
cmake -DXSIGMA_ENABLE_BENCHMARK=ON ..
make BenchmarkAsyncParallel

# Run
./BenchmarkAsyncParallel
```

---

## Common Issues

### Issue 1: Operation Never Completes

**Symptom**: `wait()` hangs forever

**Causes**:
- Calling `wait()` from within async operation (deadlock)
- Thread pool exhausted

**Solution**:
- Don't call `wait()` inside async operation
- Use `wait_for()` with timeout to detect
- Check thread pool configuration

### Issue 2: Poor Performance

**Symptom**: Async slower than sync

**Causes**:
- Operation too small (<1ms)
- Too many concurrent operations
- Grain size too small

**Solution**:
- Use sync version for small operations
- Limit concurrent ops to ~2 * num_interop_threads
- Increase grain_size

### Issue 3: Memory Issues

**Symptom**: Memory leak or high usage

**Causes**:
- Handles not moved, causing copies
- Lambdas capturing large objects by value

**Solution**:
- Always move handles: `auto h2 = std::move(h1);`
- Capture by reference: `[&data]` not `[data]`

---

## Error Handling

### Exception-Free Interface

XSigma uses **no exceptions** in public API:

```cpp
auto handle = xsigma::async_parallel_for(/*...*/);
handle.wait();

// Check explicitly:
if (handle.has_error()) {
    std::string error = handle.get_error();
    XSIGMA_LOG_ERROR("Error: {}", error);
    // Handle error...
}
```

### Error Messages

Errors include context:
- `"Exception in async_parallel_for: <message>"`
- `"Unknown error in async_parallel_for"`

---

## Backend Compatibility

| Backend | Support | Notes |
|---------|---------|-------|
| **Native** | ✅ Excellent | Uses inter-op thread pool |
| **OpenMP** | ✅ Good | Independent parallel regions |
| **TBB** | ✅ Excellent | Native task groups (best) |

All backends fully supported.

---

## Files Reference

### Implementation
- [parallel.h](../Library/Core/parallel/parallel.h) - API functions
- [async_handle.h](../Library/Core/parallel/async_handle.h) - Handle class

### Tests
- [TestAsyncParallel.cpp](../Library/Core/Testing/Cxx/TestAsyncParallel.cpp) - 25 tests
- [BenchmarkAsyncParallel.cpp](../Library/Core/Testing/Cxx/BenchmarkAsyncParallel.cpp) - 18 benchmarks

### Documentation
- [ASYNC_PARALLEL_DESIGN.md](ASYNC_PARALLEL_DESIGN.md) - Full design
- [ASYNC_PARALLEL_IMPLEMENTATION.md](ASYNC_PARALLEL_IMPLEMENTATION.md) - Implementation
- [ASYNC_PARALLEL_EXAMPLES.cpp](ASYNC_PARALLEL_EXAMPLES.cpp) - Usage examples

---

## Quick Troubleshooting

| Problem | Solution |
|---------|----------|
| `handle.valid()` returns false | Handle not initialized or moved from |
| `wait()` hangs | Check for deadlock, use `wait_for()` |
| Slow performance | Check operation size, grain size, concurrency |
| Memory leak | Use move semantics, capture by reference |
| Compilation error | Check includes: `#include "parallel/parallel.h"` |

---

## Need Help?

1. **Check documentation**: [ASYNC_PARALLEL_DESIGN.md](ASYNC_PARALLEL_DESIGN.md)
2. **See examples**: [ASYNC_PARALLEL_EXAMPLES.cpp](ASYNC_PARALLEL_EXAMPLES.cpp)
3. **Run tests**: `./TestAsyncParallel`
4. **Check benchmarks**: `./BenchmarkAsyncParallel`

---

**Version**: 1.0
**Last Updated**: 2025-12-08
**Status**: ✅ Ready to Use
