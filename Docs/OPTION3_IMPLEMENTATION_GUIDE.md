# Option 3: Implementation Guide - Backend-Specific Thread-Local Storage

## Overview

This guide provides step-by-step instructions to implement Option 3, which uses backend-specific thread-local storage solutions instead of standard C++ `thread_local`.

---

## Step 1: Understand Current State

**Current Implementation (Option 1)**:
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

**Target Implementation (Option 3)**:
- OpenMP: Use `#pragma omp threadprivate` with static storage
- TBB: Use `tbb::enumerable_thread_specific<unsigned char>`
- Native: Keep standard C++ `thread_local`

---

## Step 2: Add Required Includes

**File**: `Library/Core/parallel/parallel_tools.h`

Add conditional include for TBB:

```cpp
#include <functional>   // For std::function
#include <type_traits>  // For std::enable_if

#include "common/export.h"
#include "parallel/common/parallel_tools_api.h"

#if QUARISMA_HAS_TBB
#include <tbb/enumerable_thread_specific.h>
#endif
```

---

## Step 3: Define OpenMP Threadprivate Static

**Location**: In `Library/Core/parallel/parallel_tools.h` before the struct definition

```cpp
namespace quarisma {
namespace detail {
namespace parallel {

#if QUARISMA_HAS_OPENMP
// Static storage for OpenMP threadprivate
// Each thread gets its own copy automatically
thread_local unsigned char parallel_tools_functor_initialized = 0;
#pragma omp threadprivate(parallel_tools_functor_initialized)
#endif

// ... rest of code
```

---

## Step 4: Update Struct Definition

**Location**: `Library/Core/parallel/parallel_tools.h` lines 88-108

Replace the entire struct with:

```cpp
template <typename Functor>
struct parallel_tools_functor_internal<Functor, true>
{
    Functor& f_;

#if QUARISMA_HAS_TBB
    // TBB backend: Use enumerable_thread_specific
    mutable tbb::enumerable_thread_specific<unsigned char> initialized_;
#endif

    parallel_tools_functor_internal(Functor& f) : f_(f)
#if QUARISMA_HAS_TBB
        , initialized_(0)
#endif
    {}

    void Execute(size_t first, size_t last)
    {
#if QUARISMA_HAS_OPENMP
        // OpenMP backend: Use threadprivate static
        if (!parallel_tools_functor_initialized)
        {
            this->f_.Initialize();
            parallel_tools_functor_initialized = 1;
        }
#elif QUARISMA_HAS_TBB
        // TBB backend: Use enumerable_thread_specific
        unsigned char& inited = initialized_.local();
        if (!inited)
        {
            this->f_.Initialize();
            inited = 1;
        }
#else
        // Native backend: Use standard C++ thread_local
        thread_local unsigned char initialized = 0;
        if (!initialized)
        {
            this->f_.Initialize();
            initialized = 1;
        }
#endif
        this->f_(first, last);
    }

    void parallel_for(size_t first, size_t last, size_t grain)
    {
        auto& SMPToolsAPI = parallel_tools_api::instance();
        SMPToolsAPI.parallel_for(first, last, grain, *this);
        this->f_.Reduce();
    }

    parallel_tools_functor_internal<Functor, true>& operator=(
        const parallel_tools_functor_internal<Functor, true>&);
    parallel_tools_functor_internal(const parallel_tools_functor_internal<Functor, true>&);
};
```

---

## Step 5: Verify Includes

Ensure these headers are available:

**For TBB**:
- `#include <tbb/enumerable_thread_specific.h>` (conditional)

**For OpenMP**:
- Standard C++ `thread_local` (built-in)
- `#pragma omp threadprivate` (built-in)

**For Native**:
- Standard C++ `thread_local` (built-in)

---

## Step 6: Build and Test

### Build with TBB Backend
```bash
cd Scripts
python setup.py config.build.test.ninja.clang.python
```

Expected output:
```
[SUCCESS] Build completed successfully
100% tests passed, 0 tests failed out of 2
```

### Build with OpenMP Backend
```bash
# Set OpenMP enabled (if available)
cd Scripts
python setup.py config.build.test.ninja.clang.python
```

### Build with Native Backend
```bash
# Disable both TBB and OpenMP
cd Scripts
python setup.py config.build.test.ninja.clang.python
```

---

## Step 7: Verify Behavior

### Check Compilation
```bash
# Verify no compilation errors
cd build_ninja_python
ninja -v 2>&1 | grep -i "error\|warning"
```

### Run Tests
```bash
cd build_ninja_python
ctest -V
```

### Verify Backend Selection
```bash
# Check which backend is being used
cd build_ninja_python
./bin/CoreCxxTests --gtest_filter="*Parallel*" -v
```

---

## Step 8: Code Review Checklist

- [ ] All includes are conditional and correct
- [ ] OpenMP threadprivate is properly defined
- [ ] TBB enumerable_thread_specific is properly initialized
- [ ] Native thread_local is used as fallback
- [ ] All three code paths are tested
- [ ] No compilation warnings
- [ ] All tests pass
- [ ] No performance regression

---

## Potential Issues and Solutions

### Issue 1: OpenMP Threadprivate Scope
**Problem**: `#pragma omp threadprivate` requires static storage duration
**Solution**: Define static variable at namespace scope before struct

### Issue 2: TBB Header Not Found
**Problem**: `#include <tbb/enumerable_thread_specific.h>` fails
**Solution**: Ensure TBB is properly linked (check CMakeLists.txt)

### Issue 3: Mutable Member in TBB
**Problem**: `initialized_` needs to be mutable for const methods
**Solution**: Mark as `mutable tbb::enumerable_thread_specific<...>`

### Issue 4: Constructor Initialization
**Problem**: Conditional member initialization is complex
**Solution**: Use conditional initialization list with `#if` directives

---

## Rollback Plan

If issues arise:

```bash
# Revert to Option 1 (standard C++ thread_local)
git checkout HEAD -- Library/Core/parallel/parallel_tools.h

# Or revert to original parallel_thread_local
git checkout HEAD~1 -- Library/Core/parallel/
```

---

## Success Criteria

✅ Code compiles without errors
✅ Code compiles without warnings
✅ All tests pass (2/2)
✅ No performance regression
✅ Backend-specific code paths verified
✅ Documentation updated

---

## Estimated Time

- Code changes: 1-2 hours
- Testing: 1-2 hours
- Verification: 30 minutes
- **Total: 2.5-4.5 hours**


