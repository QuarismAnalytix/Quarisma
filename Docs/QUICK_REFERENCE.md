# Quick Reference Guide

## Files to Remove (11 total)

```
Library/Core/smp/smp_thread_local.h
Library/Core/smp/common/smp_thread_local_api.h
Library/Core/smp/common/smp_thread_local_impl_abstract.h
Library/Core/smp/std_thread/smp_thread_local_impl.h
Library/Core/smp/std_thread/smp_thread_local_backend.h
Library/Core/smp/std_thread/smp_thread_local_backend.cpp
Library/Core/smp/openmp/smp_thread_local_impl.h
Library/Core/smp/openmp/smp_thread_local_backend.h
Library/Core/smp/openmp/smp_thread_local_backend.cpp
Library/Core/smp/tbb/smp_thread_local_impl.h
```

---

## File to Modify (1 total)

### Library/Core/smp/smp_tools.h

**Line 26 - REMOVE:**
```cpp
#include "smp_thread_local.h"
```

**Lines 92-94 - CHANGE FROM:**
```cpp
Functor&                        f_;
smp_thread_local<unsigned char> initialized_;
smp_tools_functor_internal(Functor& f) : f_(f), initialized_(0) {}
```

**CHANGE TO:**
```cpp
Functor& f_;
smp_tools_functor_internal(Functor& f) : f_(f) {}
```

**Lines 97-98 - CHANGE FROM:**
```cpp
unsigned char& inited = this->initialized_.local();
```

**CHANGE TO:**
```cpp
thread_local unsigned char initialized = 0;
unsigned char& inited = initialized;
```

---

## Option Comparison (Quick)

| Aspect | Option 1 | Option 2 | Option 3 | Option 4 |
|--------|----------|----------|----------|----------|
| **Approach** | `thread_local` | `std::vector<T>` | Backend-specific | Refactor |
| **Complexity** | ⭐ | ⭐⭐ | ⭐⭐⭐ | ⭐⭐ |
| **Risk** | ⭐ | ⭐⭐ | ⭐⭐⭐ | ⭐⭐ |
| **Time** | 1-2h | 3-4h | 6-8h | 2-3h |
| **Performance** | BEST | POOR | BEST | BEST |
| **Recommended** | ✅ | ⚠️ | ❌ | ⚠️ |

---

## Implementation Steps (Option 1)

### 1. Verify (15 min)
```bash
grep -r "smp_thread_local" Library/Core Tests --include="*.h" --include="*.cpp" | \
  grep -v "smp_thread_local_" | grep -v "smp_tools.h"
# Expected: No matches
```

### 2. Update smp_tools.h (30 min)
- Remove include
- Remove member variable
- Remove constructor initialization
- Update Execute() method

### 3. Remove Files (10 min)
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

### 4. Update CMakeLists.txt (10 min)
Remove all smp_thread_local* files from build configuration

### 5. Build & Test (30 min)
```bash
cd Scripts
python setup.py config.build.test.ninja.clang.python
cd ../build_ninja_python
ctest -V
```

---

## Success Checklist

- [ ] All 11 files removed
- [ ] smp_tools.h updated
- [ ] CMakeLists.txt updated
- [ ] Build succeeds
- [ ] All tests pass
- [ ] No compiler warnings
- [ ] Code coverage ≥ 98%

---

## Key Facts

- **Active Usage**: 1 location (smp_tools.h line 93)
- **Files Affected**: 1 (smp_tools.h)
- **API Changes**: 0
- **Test Changes**: 0
- **External Dependencies**: 0
- **Code Removed**: ~1000 lines
- **Estimated Time**: 1.5-2 hours
- **Risk Level**: LOW

---

## Documents to Read

1. **REMOVAL_SUMMARY.md** - Start here
2. **REMOVAL_IMPLEMENTATION_GUIDE.md** - How to do it
3. **REMOVAL_CHECKLIST.md** - Step-by-step checklist
4. **OPTION_COMPARISON_DETAILED.md** - If considering other options

---

## Rollback Command

If anything goes wrong:
```bash
git checkout HEAD -- Library/Core/smp/
```


