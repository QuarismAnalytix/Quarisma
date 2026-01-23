# Option 3: Backend-Specific Thread-Local Storage - Detailed Analysis

## Overview

Instead of using standard C++ `thread_local`, implement backend-specific thread-local storage solutions that leverage each backend's native capabilities.

---

## Current Backend Detection System

The codebase uses **function-level conditional compilation** with these flags:

```cpp
#if XSIGMA_HAS_OPENMP
    // OpenMP backend (Priority: 1 - Highest)
#elif XSIGMA_HAS_TBB
    // TBB backend (Priority: 2)
#else
    // Native std_thread backend (Priority: 3 - Fallback)
#endif
```

**Backend Priority**: OpenMP > TBB > Native (first available is used)

---

## Changes Required for Option 3

### 1. Update `Library/Core/parallel/parallel_tools.h`

#### Current Implementation (Option 1)
```cpp
struct parallel_tools_functor_internal<Functor, true> {
    Functor& f_;
    parallel_tools_functor_internal(Functor& f) : f_(f) {}
    void Execute(size_t first, size_t last) {
        thread_local unsigned char initialized = 0;
        if (!initialized) {
            this->f_.Initialize();
            initialized = 1;
        }
        this->f_(first, last);
    }
};
```

#### Option 3 Implementation
```cpp
struct parallel_tools_functor_internal<Functor, true> {
    Functor& f_;
    
#if XSIGMA_HAS_OPENMP
    // OpenMP backend: Use threadprivate directive
    // Note: threadprivate requires static storage duration
    static thread_local unsigned char initialized;
#elif XSIGMA_HAS_TBB
    // TBB backend: Use enumerable_thread_specific
    mutable tbb::enumerable_thread_specific<unsigned char> initialized_;
#else
    // Native backend: Use standard C++ thread_local
    // (no member needed - use local thread_local in Execute)
#endif
    
    parallel_tools_functor_internal(Functor& f) : f_(f)
#if XSIGMA_HAS_TBB
        , initialized_(0)
#endif
    {}
    
    void Execute(size_t first, size_t last) {
#if XSIGMA_HAS_OPENMP
        if (!initialized) {
            this->f_.Initialize();
            initialized = 1;
        }
#elif XSIGMA_HAS_TBB
        unsigned char& inited = initialized_.local();
        if (!inited) {
            this->f_.Initialize();
            inited = 1;
        }
#else
        thread_local unsigned char initialized = 0;
        if (!initialized) {
            this->f_.Initialize();
            initialized = 1;
        }
#endif
        this->f_(first, last);
    }
};
```

### 2. Add Required Includes

```cpp
#include <functional>   // For std::function
#include <type_traits>  // For std::enable_if

#include "common/export.h"
#include "parallel/common/parallel_tools_api.h"

#if XSIGMA_HAS_TBB
#include <tbb/enumerable_thread_specific.h>
#endif
```

### 3. Handle OpenMP Static Storage

For OpenMP's `threadprivate`, need to define static storage outside the struct:

```cpp
// In parallel_tools.h or separate file
#if XSIGMA_HAS_OPENMP
namespace xsigma {
namespace detail {
namespace parallel {
    // Static storage for OpenMP threadprivate
    thread_local unsigned char parallel_tools_functor_initialized = 0;
    #pragma omp threadprivate(parallel_tools_functor_initialized)
}
}
}
#endif
```

---

## Detailed Changes by Backend

### Backend 1: OpenMP (`XSIGMA_HAS_OPENMP=1`)

**Mechanism**: `#pragma omp threadprivate` directive

**Pros**:
- ✅ Native OpenMP support
- ✅ Integrated with OpenMP runtime
- ✅ Zero overhead
- ✅ Automatic cleanup

**Cons**:
- ❌ Requires static storage duration
- ❌ Less flexible (can't use in class members directly)
- ❌ Requires pragma directive
- ❌ Harder to reason about scope

**Implementation**:
```cpp
// Global static with threadprivate
thread_local unsigned char parallel_tools_functor_initialized = 0;
#pragma omp threadprivate(parallel_tools_functor_initialized)

// In Execute():
if (!parallel_tools_functor_initialized) {
    this->f_.Initialize();
    parallel_tools_functor_initialized = 1;
}
```

---

### Backend 2: TBB (`XSIGMA_HAS_TBB=1`)

**Mechanism**: `tbb::enumerable_thread_specific<T>`

**Pros**:
- ✅ Native TBB support
- ✅ Can enumerate all thread-local copies
- ✅ Integrated with TBB thread pool
- ✅ Better performance with TBB
- ✅ Can be class member

**Cons**:
- ❌ Requires TBB header
- ❌ Slightly more overhead than thread_local
- ❌ Requires initialization in constructor

**Implementation**:
```cpp
struct parallel_tools_functor_internal<Functor, true> {
    Functor& f_;
    mutable tbb::enumerable_thread_specific<unsigned char> initialized_;
    
    parallel_tools_functor_internal(Functor& f) 
        : f_(f), initialized_(0) {}
    
    void Execute(size_t first, size_t last) {
        unsigned char& inited = initialized_.local();
        if (!inited) {
            this->f_.Initialize();
            inited = 1;
        }
        this->f_(first, last);
    }
};
```

---

### Backend 3: Native std_thread (`XSIGMA_HAS_OPENMP=0 && XSIGMA_HAS_TBB=0`)

**Mechanism**: Standard C++ `thread_local`

**Pros**:
- ✅ Standard C++ feature
- ✅ Zero overhead
- ✅ Simple and clean
- ✅ No dependencies

**Cons**:
- ❌ Can't enumerate thread-local copies
- ❌ Less integration with thread pool

**Implementation**:
```cpp
void Execute(size_t first, size_t last) {
    thread_local unsigned char initialized = 0;
    if (!initialized) {
        this->f_.Initialize();
        initialized = 1;
    }
    this->f_(first, last);
}
```

---

## Files to Modify

| File | Changes |
|------|---------|
| `Library/Core/parallel/parallel_tools.h` | Add conditional compilation, includes, and backend-specific implementations |

---

## Files to Remove

None - all 10 files already removed in Option 1

---

## Build Impact

- ✅ No CMakeLists.txt changes needed
- ✅ Conditional compilation at compile-time
- ✅ No runtime overhead
- ✅ Backend selection automatic based on available libraries

---

## Testing Impact

- ✅ All existing tests should pass
- ✅ No new tests needed
- ✅ Behavior identical across backends

---

## Complexity Assessment

| Aspect | Complexity |
|--------|-----------|
| **Code changes** | MEDIUM |
| **Lines of code** | ~50-70 lines |
| **Conditional compilation** | 3 branches |
| **Testing** | LOW (existing tests sufficient) |
| **Maintenance** | MEDIUM (3 backends to maintain) |
| **Risk** | MEDIUM (backend-specific code) |

---

## Performance Characteristics

| Backend | Overhead | Scalability | Notes |
|---------|----------|-------------|-------|
| **OpenMP** | Zero | Excellent | Native integration |
| **TBB** | Minimal | Excellent | enumerable_thread_specific optimized |
| **Native** | Zero | Good | Standard C++ feature |

---

## Advantages Over Option 1

✅ **Better TBB integration** - Uses TBB's native thread-local storage
✅ **Better OpenMP integration** - Uses OpenMP's native threadprivate
✅ **Potential performance gains** - Backend-optimized implementations
✅ **Future extensibility** - Easy to add backend-specific optimizations

---

## Disadvantages vs Option 1

❌ **More complex code** - 3 different implementations
❌ **More maintenance** - Need to maintain 3 code paths
❌ **Harder to understand** - Conditional compilation adds complexity
❌ **More testing needed** - Need to test each backend separately

---

## Implementation Steps

1. Add includes for TBB (conditional)
2. Define OpenMP threadprivate static (conditional)
3. Update struct with conditional members
4. Update constructor with conditional initialization
5. Update Execute() with conditional implementation
6. Build and test with each backend
7. Verify no regressions

---

## Estimated Effort

- **Code changes**: 1-2 hours
- **Testing**: 1-2 hours
- **Documentation**: 30 minutes
- **Total**: 2.5-4.5 hours

---

## Recommendation

**Option 3 is recommended if**:
- ✅ You want maximum performance per backend
- ✅ You want native integration with each backend
- ✅ You're willing to maintain 3 code paths
- ✅ You need future extensibility

**Option 1 is recommended if**:
- ✅ You want simplicity
- ✅ You want minimal maintenance
- ✅ You want fastest implementation
- ✅ Performance is not critical


