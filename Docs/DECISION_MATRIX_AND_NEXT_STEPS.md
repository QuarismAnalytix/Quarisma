# Decision Matrix & Next Steps

## Quick Decision Guide

### Choose Option 1 (`thread_local`) if:
- ✅ You want **minimal changes** to codebase
- ✅ You want **fastest implementation** (1-2 hours)
- ✅ You want **lowest risk** (no API changes)
- ✅ You don't need to **iterate over thread-local copies**
- ✅ You don't need to **aggregate results from threads**

### Choose Option 2 (`std::vector<T>`) if:
- ✅ You need to **iterate/aggregate** thread-local data
- ✅ You can tolerate **mutex overhead**
- ✅ You want a **single implementation** for all backends
- ✅ You have **moderate time budget** (3-4 hours)

### Choose Option 3 (Backend-specific) if:
- ✅ You need **optimal performance** per backend
- ✅ You want to **leverage native features** (TBB, OpenMP)
- ✅ You have **significant time budget** (6-8 hours)
- ✅ You're willing to **maintain conditional compilation**

### Choose Option 4 (Refactor design) if:
- ✅ You want to **eliminate thread-local storage entirely**
- ✅ You can **change API semantics** (Initialize called once)
- ✅ You're willing to **review user code** for compatibility
- ✅ You want **simplest long-term design**

---

## Recommended Path: Option 1

### Why Option 1?
1. **Lowest risk** - Only 1 active usage
2. **Fastest** - 1-2 hours implementation
3. **Simplest** - Standard C++ feature
4. **No API changes** - Backward compatible
5. **No performance regression** - Zero overhead

### Implementation Steps

#### Phase 1: Verification (15 minutes)
```bash
# Confirm no other usages
grep -r "parallel_thread_local" Library/Core Tests --include="*.h" --include="*.cpp" | \
  grep -v "parallel_thread_local_" | grep -v "parallel_tools.h"

# Confirm no iteration usage
grep -r "\.begin()\|\.end()" Library/Core/parallel/parallel_tools.h
```

#### Phase 2: Code Changes (30 minutes)
1. Update `Library/Core/parallel/parallel_tools.h` (line 93)
2. Remove `#include "parallel_thread_local.h"`
3. Replace `parallel_thread_local<unsigned char>` with `thread_local unsigned char`
4. Update CMakeLists.txt to remove parallel_thread_local files

#### Phase 3: File Removal (10 minutes)
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

#### Phase 4: Build & Test (30 minutes)
```bash
cd Scripts
python setup.py config.build.test.ninja.clang.python
cd ../build_ninja_python
ctest -V
```

#### Phase 5: Verification (15 minutes)
- [ ] Build succeeds
- [ ] All tests pass
- [ ] No compiler warnings
- [ ] Code coverage ≥ 98%
- [ ] No performance regression

**Total Time: 1.5-2 hours**

---

## Alternative: Option 2 (If Iteration Needed)

If you discover that iteration/aggregation is needed:

1. Create `Library/Core/parallel/common/thread_local_storage.h`
2. Implement `thread_local_storage<T>` class
3. Update `parallel_tools.h` to use new class
4. Add aggregation logic where needed
5. Test thoroughly

**Additional Time: 2-3 hours**

---

## What NOT to Do

❌ **Don't choose Option 3** unless you have specific performance requirements
❌ **Don't choose Option 4** without reviewing all user code first
❌ **Don't remove files** without updating CMakeLists.txt
❌ **Don't skip testing** - run full test suite after changes

---

## Next Steps

1. **Review this analysis** with team
2. **Choose preferred option** (recommend Option 1)
3. **Get approval** to proceed
4. **Execute implementation** following guide
5. **Run full test suite** and verify
6. **Commit changes** with clear message

---

## Questions to Ask Before Proceeding

1. Is iteration over thread-local copies needed anywhere?
2. Is aggregation of thread-local results needed?
3. Are there performance-critical sections?
4. Is the codebase using per-thread initialization patterns?
5. Are there external users of parallel_thread_local?

**If all answers are "No" → Proceed with Option 1**


