# Complete Removal Checklist

## Pre-Removal Analysis ✓

- [x] Identified all 11 files to remove
- [x] Found single active usage in smp_tools.h (line 93)
- [x] Verified no iteration/aggregation patterns
- [x] Confirmed no external dependencies
- [x] Analyzed 4 viable options
- [x] Recommended Option 1 (thread_local)

---

## Pre-Implementation Verification

### Code Analysis
- [ ] Run: `grep -r "smp_thread_local" Library/Core Tests --include="*.h" --include="*.cpp" | grep -v "smp_thread_local_" | grep -v "smp_tools.h"`
  - Expected: No matches
- [ ] Run: `grep -r "\.begin()\|\.end()" Library/Core/smp/smp_tools.h`
  - Expected: No matches
- [ ] Run: `grep -r "smp_thread_local<.*>(" Library/Core/smp/smp_tools.h`
  - Expected: Only `initialized_(0)`

### Build Baseline
- [ ] Current build succeeds
- [ ] All tests pass
- [ ] Code coverage ≥ 98%
- [ ] No compiler warnings

---

## Implementation (Option 1)

### Step 1: Update smp_tools.h
- [ ] Remove line 26: `#include "smp_thread_local.h"`
- [ ] Update lines 92-94:
  ```cpp
  // FROM:
  smp_thread_local<unsigned char> initialized_;
  smp_tools_functor_internal(Functor& f) : f_(f), initialized_(0) {}
  
  // TO:
  smp_tools_functor_internal(Functor& f) : f_(f) {}
  ```
- [ ] Update lines 97-98:
  ```cpp
  // FROM:
  unsigned char& inited = this->initialized_.local();
  
  // TO:
  thread_local unsigned char initialized = 0;
  unsigned char& inited = initialized;
  ```

### Step 2: Update CMakeLists.txt
- [ ] Remove smp_thread_local.h from header list
- [ ] Remove smp_thread_local_api.h from header list
- [ ] Remove smp_thread_local_impl_abstract.h from header list
- [ ] Remove std_thread/smp_thread_local_impl.h from header list
- [ ] Remove std_thread/smp_thread_local_backend.h from header list
- [ ] Remove std_thread/smp_thread_local_backend.cpp from source list
- [ ] Remove openmp/smp_thread_local_impl.h from header list
- [ ] Remove openmp/smp_thread_local_backend.h from header list
- [ ] Remove openmp/smp_thread_local_backend.cpp from source list
- [ ] Remove tbb/smp_thread_local_impl.h from header list

### Step 3: Remove Files
- [ ] Delete Library/Core/smp/smp_thread_local.h
- [ ] Delete Library/Core/smp/common/smp_thread_local_api.h
- [ ] Delete Library/Core/smp/common/smp_thread_local_impl_abstract.h
- [ ] Delete Library/Core/smp/std_thread/smp_thread_local_impl.h
- [ ] Delete Library/Core/smp/std_thread/smp_thread_local_backend.h
- [ ] Delete Library/Core/smp/std_thread/smp_thread_local_backend.cpp
- [ ] Delete Library/Core/smp/openmp/smp_thread_local_impl.h
- [ ] Delete Library/Core/smp/openmp/smp_thread_local_backend.h
- [ ] Delete Library/Core/smp/openmp/smp_thread_local_backend.cpp
- [ ] Delete Library/Core/smp/tbb/smp_thread_local_impl.h

---

## Build & Test

### Build
- [ ] `cd Scripts`
- [ ] `python setup.py config.build.test.ninja.clang.python`
- [ ] Build succeeds with no errors
- [ ] Build succeeds with no warnings

### Testing
- [ ] `cd ../build_ninja_python`
- [ ] `ctest -V` passes all tests
- [ ] Code coverage ≥ 98%
- [ ] No performance regression

### Verification
- [ ] All 11 files removed
- [ ] smp_tools.h updated correctly
- [ ] CMakeLists.txt updated correctly
- [ ] No references to smp_thread_local remain
- [ ] No compiler warnings

---

## Post-Implementation

### Documentation
- [ ] Update CHANGELOG.md
- [ ] Update SMP module documentation
- [ ] Remove references to smp_thread_local from docs
- [ ] Update code comments if needed

### Cleanup
- [ ] Remove temporary files
- [ ] Clean build directories
- [ ] Verify git status

### Final Verification
- [ ] Run full test suite one more time
- [ ] Check for any missed references
- [ ] Verify no performance regression
- [ ] Code review completed

---

## Rollback Plan (If Needed)

If critical issues arise:
```bash
git checkout HEAD -- Library/Core/smp/
git checkout HEAD -- Docs/
```

---

## Sign-Off

- [ ] Implementation complete
- [ ] All tests passing
- [ ] Code review approved
- [ ] Ready to commit

**Estimated Total Time: 1.5-2 hours**


