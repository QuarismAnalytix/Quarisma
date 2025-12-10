# Implementation Guide: Removing `smp_thread_local`

## Pre-Removal Verification Checklist

### 1. Verify No External Dependencies
```bash
# Check if smp_thread_local is used outside smp_tools.h
grep -r "smp_thread_local" --include="*.h" --include="*.cpp" \
  Library/Core Tests --exclude-dir=ThirdParty | \
  grep -v "smp_tools.h" | grep -v "smp_thread_local_"
```

**Expected Result**: Only matches in smp_thread_local_* files themselves

### 2. Check for Iteration Usage
```bash
# Look for .begin(), .end(), or iteration patterns
grep -r "\.begin()\|\.end()" Library/Core/smp/smp_tools.h
```

**Expected Result**: No matches (iteration not used)

### 3. Check for Exemplar Pattern
```bash
# Look for constructor calls with parameters
grep -r "smp_thread_local<.*>(" Library/Core/smp/smp_tools.h
```

**Expected Result**: Only `initialized_(0)` (single parameter)

---

## Step-by-Step Removal (Option 1: `thread_local`)

### Step 1: Update smp_tools.h
```cpp
// REMOVE: #include "smp_thread_local.h"

// CHANGE FROM:
struct smp_tools_functor_internal<Functor, true> {
    Functor& f_;
    smp_thread_local<unsigned char> initialized_;
    smp_tools_functor_internal(Functor& f) : f_(f), initialized_(0) {}
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
struct smp_tools_functor_internal<Functor, true> {
    Functor& f_;
    smp_tools_functor_internal(Functor& f) : f_(f) {}
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
rm Library/Core/smp/smp_thread_local.h
rm Library/Core/smp/common/smp_thread_local_api.h
rm Library/Core/smp/common/smp_thread_local_impl_abstract.h
rm Library/Core/smp/std_thread/smp_thread_local_impl.h
rm Library/Core/smp/std_thread/smp_thread_local_backend.h
rm Library/Core/smp/std_thread/smp_thread_local_backend.cpp
rm Library/Core/smp/openmp/smp_thread_local_impl.h
rm Library/Core/smp/openmp/smp_thread_local_backend.h
rm Library/Core/smp/openmp/smp_thread_local_backend.cpp
rm Library/Core/smp/tbb/smp_thread_local_impl.h
```

### Step 3: Update CMakeLists.txt
Remove smp_thread_local files from build configuration

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
- [ ] smp_tools.h updated
- [ ] CMakeLists.txt updated
- [ ] Build succeeds
- [ ] All tests pass
- [ ] No compiler warnings
- [ ] Code coverage ≥ 98%

---

## Rollback Plan

If issues arise:
```bash
git checkout HEAD -- Library/Core/smp/
git checkout HEAD -- Docs/
```


