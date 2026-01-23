# Implementation Guide: Removing `parallel_thread_local`

## Pre-Removal Verification Checklist

### 1. Verify No External Dependencies
```bash
# Check if parallel_thread_local is used outside parallel_tools.h
grep -r "parallel_thread_local" --include="*.h" --include="*.cpp" \
  Library/Core Tests --exclude-dir=ThirdParty | \
  grep -v "parallel_tools.h" | grep -v "parallel_thread_local_"
```

**Expected Result**: Only matches in parallel_thread_local_* files themselves

### 2. Check for Iteration Usage
```bash
# Look for .begin(), .end(), or iteration patterns
grep -r "\.begin()\|\.end()" Library/Core/parallel/parallel_tools.h
```

**Expected Result**: No matches (iteration not used)

### 3. Check for Exemplar Pattern
```bash
# Look for constructor calls with parameters
grep -r "parallel_thread_local<.*>(" Library/Core/parallel/parallel_tools.h
```

**Expected Result**: Only `initialized_(0)` (single parameter)

---

## Step-by-Step Removal (Option 1: `thread_local`)

### Step 1: Update parallel_tools.h
```cpp
// REMOVE: #include "parallel_thread_local.h"

// CHANGE FROM:
struct parallel_tools_functor_internal<Functor, true> {
    Functor& f_;
    parallel_thread_local<unsigned char> initialized_;
    parallel_tools_functor_internal(Functor& f) : f_(f), initialized_(0) {}
    void Execute(size_t first, size_t last) {
        unsigned char& inited = this->initialized_.local();
        if (!inited) {
            this->f_.Initialize();
            inited = 1;
        }
        this->f_(first, last);
    }
};

// CHANGE TO:
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

### Step 2: Remove Files
```bash
rm Library/Core/parallel/parallel_thread_local.h
rm Library/Core/parallel/common/parallel_thread_local_api.h
rm Library/Core/parallel/common/parallel_thread_local_impl_abstract.h
rm Library/Core/parallel/std_thread/parallel_thread_local_impl.h
rm Library/Core/parallel/std_thread/parallel_thread_local_backend.h
rm Library/Core/parallel/std_thread/parallel_thread_local_backend.cpp
rm Library/Core/parallel/openmp/parallel_thread_local_impl.h
rm Library/Core/parallel/openmp/parallel_thread_local_backend.h
rm Library/Core/parallel/openmp/parallel_thread_local_backend.cpp
rm Library/Core/parallel/tbb/parallel_thread_local_impl.h
```

### Step 3: Update CMakeLists.txt
Remove parallel_thread_local files from build configuration

### Step 4: Verify Build
```bash
cd Scripts
python setup.py config.build.test.ninja.clang.python
```

### Step 5: Run Tests
```bash
cd build_ninja_python
ctest -V
```

---

## Validation Checklist

- [ ] All 11 files removed
- [ ] parallel_tools.h updated
- [ ] CMakeLists.txt updated
- [ ] Build succeeds
- [ ] All tests pass
- [ ] No compiler warnings
- [ ] Code coverage ≥ 98%

---

## Rollback Plan

If issues arise:
```bash
git checkout HEAD -- Library/Core/parallel/
git checkout HEAD -- Docs/
```


