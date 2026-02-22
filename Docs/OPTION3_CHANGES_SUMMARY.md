# Option 3: Complete Changes Summary

## Overview

This document summarizes all changes needed to implement Option 3 (Backend-Specific Thread-Local Storage).

---

## File to Modify

**Single File**: `Library/Core/parallel/parallel_tools.h`

---

## Change 1: Add Conditional Include for TBB

**Location**: Lines 21-26 (after existing includes)

**Current**:
```cpp
#include <functional>   // For std::function
#include <type_traits>  // For std::enable_if

#include "common/export.h"
#include "parallel/common/parallel_tools_api.h"
```

**New**:
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

## Change 2: Add OpenMP Threadprivate Static

**Location**: Lines 27-32 (after includes, before namespace)

**New Code**:
```cpp
namespace quarisma
{
namespace detail
{
namespace parallel
{

#if QUARISMA_HAS_OPENMP
// Static storage for OpenMP threadprivate
// Each thread gets its own copy automatically
thread_local unsigned char parallel_tools_functor_initialized = 0;
#pragma omp threadprivate(parallel_tools_functor_initialized)
#endif
```

---

## Change 3: Update Struct Definition

**Location**: Lines 84-108 (entire struct replacement)

**Current**:
```cpp
template <typename Functor>
struct parallel_tools_functor_internal<Functor, true>
{
    Functor& f_;
    parallel_tools_functor_internal(Functor& f) : f_(f) {}
    void Execute(size_t first, size_t last)
    {
        thread_local unsigned char initialized = 0;
        if (!initialized)
        {
            this->f_.Initialize();
            initialized = 1;
        }
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

**New**:
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

## Summary of Changes

| Change | Type | Lines | Impact |
|--------|------|-------|--------|
| Add TBB include | Include | 3 | Conditional |
| Add OpenMP threadprivate | Static | 5 | Conditional |
| Update struct | Code | 60 | Major |
| **Total** | | **~70** | **Medium** |

---

## Files Affected

| File | Changes | Impact |
|------|---------|--------|
| `Library/Core/parallel/parallel_tools.h` | 3 changes | MODIFIED |
| `Library/Core/CMakeLists.txt` | None | NO CHANGE |
| All other files | None | NO CHANGE |

---

## Conditional Compilation Branches

```
#if QUARISMA_HAS_OPENMP
    // Branch 1: OpenMP backend
    // Uses: #pragma omp threadprivate
    // Storage: Static thread_local
    // Overhead: Zero
#elif QUARISMA_HAS_TBB
    // Branch 2: TBB backend
    // Uses: tbb::enumerable_thread_specific
    // Storage: Member variable
    // Overhead: Minimal
#else
    // Branch 3: Native backend
    // Uses: Standard C++ thread_local
    // Storage: Local thread_local
    // Overhead: Zero
#endif
```

---

## Build Requirements

**No new dependencies**:
- ✅ TBB already available (QUARISMA_HAS_TBB flag)
- ✅ OpenMP already available (QUARISMA_HAS_OPENMP flag)
- ✅ Standard C++ already available

---

## Testing Strategy

1. **Build with TBB backend**
   - Verify TBB enumerable_thread_specific works
   - Run all tests

2. **Build with OpenMP backend**
   - Verify threadprivate works
   - Run all tests

3. **Build with Native backend**
   - Verify thread_local works
   - Run all tests

---

## Verification Checklist

- [ ] Code compiles without errors
- [ ] Code compiles without warnings
- [ ] All tests pass with TBB backend
- [ ] All tests pass with OpenMP backend
- [ ] All tests pass with Native backend
- [ ] No performance regression
- [ ] Documentation updated
- [ ] Code review completed

---

## Rollback Instructions

If issues arise:

```bash
# Revert to Option 1 (current implementation)
git checkout HEAD -- Library/Core/parallel/parallel_tools.h

# Or revert to original parallel_thread_local
git checkout HEAD~2 -- Library/Core/parallel/
```

---

## Estimated Effort

- **Code changes**: 1-2 hours
- **Testing**: 1-2 hours
- **Verification**: 30 minutes
- **Total**: 2.5-4.5 hours

---

## Next Steps

1. Review `OPTION3_DETAILED_ANALYSIS.md` for technical details
2. Review `OPTION3_IMPLEMENTATION_GUIDE.md` for step-by-step instructions
3. Review `OPTION1_VS_OPTION3_COMPARISON.md` for decision guidance
4. Decide: Proceed with Option 3 or keep Option 1?


