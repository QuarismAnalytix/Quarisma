# Detailed Option Comparison

## Option 1: Standard C++ `thread_local`

### Code Example
```cpp
// parallel_tools.h - Line 93
struct parallel_tools_functor_internal<Functor, true> {
    Functor& f_;
    // REMOVE: parallel_thread_local<unsigned char> initialized_;
    
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

### Pros & Cons
| Aspect | Details |
|--------|---------|
| **Simplicity** | ✅ Minimal code changes |
| **Performance** | ✅ Zero overhead (compiler optimized) |
| **Maintenance** | ✅ Standard C++ feature |
| **Iteration** | ❌ Cannot iterate over all copies |
| **Aggregation** | ❌ Cannot collect results from threads |
| **Flexibility** | ❌ Static storage only |

### Files Removed: 11
### Lines Removed: ~1000
### Time: 1-2 hours
### Risk: **LOW**

---

## Option 2: `std::vector<T>` + Thread ID Mapping

### Code Example
```cpp
template <typename T>
class thread_local_storage {
    mutable std::mutex mutex_;
    std::map<std::thread::id, T> storage_;
    
public:
    T& get() {
        auto id = std::this_thread::get_id();
        std::lock_guard lock(mutex_);
        if (storage_.find(id) == storage_.end()) {
            storage_[id] = T();
        }
        return storage_[id];
    }
    
    std::vector<T> collect() {
        std::lock_guard lock(mutex_);
        std::vector<T> result;
        for (auto& [id, value] : storage_) {
            result.push_back(value);
        }
        return result;
    }
};

// Usage
thread_local_storage<unsigned char> initialized;
unsigned char& inited = initialized.get();
```

### Pros & Cons
| Aspect | Details |
|--------|---------|
| **Simplicity** | ⚠️ More complex than Option 1 |
| **Performance** | ❌ Mutex overhead on every access |
| **Maintenance** | ✅ Single implementation |
| **Iteration** | ✅ Can iterate/collect results |
| **Aggregation** | ✅ Can aggregate from all threads |
| **Flexibility** | ✅ Works with any type T |

### Files Removed: 11
### Files Added: 1 (new helper class)
### Lines Added: ~100
### Time: 3-4 hours
### Risk: **MEDIUM**

---

## Option 3: Backend-Specific Replacement

### Code Example (TBB)
```cpp
#if QUARISMA_HAS_TBB
    #include <tbb/enumerable_thread_specific.h>
    tbb::enumerable_thread_specific<unsigned char> initialized;
    unsigned char& inited = initialized.local();
#elif QUARISMA_HAS_OPENMP
    #pragma omp threadprivate(initialized)
    unsigned char initialized = 0;
#else
    thread_local unsigned char initialized = 0;
#endif
```

### Pros & Cons
| Aspect | Details |
|--------|---------|
| **Simplicity** | ❌ Requires conditional compilation |
| **Performance** | ✅ Optimal per backend |
| **Maintenance** | ❌ 3 different implementations |
| **Iteration** | ✅ Possible with TBB/OpenMP |
| **Aggregation** | ✅ Possible with TBB/OpenMP |
| **Flexibility** | ✅ Leverages native features |

### Files Removed: 11
### Conditional Blocks: 3
### Time: 6-8 hours
### Risk: **HIGH**

---

## Option 4: Refactor Design

### Code Example
```cpp
struct parallel_tools_functor_internal<Functor, true> {
    Functor& f_;
    bool initialized = false;
    
    parallel_tools_functor_internal(Functor& f) : f_(f) {}
    
    void parallel_for(size_t first, size_t last, size_t grain) {
        if (!initialized) {
            f_.Initialize();  // Called ONCE, not per-thread
            initialized = true;
        }
        // ... parallel execution
    }
};
```

### Pros & Cons
| Aspect | Details |
|--------|---------|
| **Simplicity** | ✅ Simplest design |
| **Performance** | ✅ Best performance |
| **Maintenance** | ✅ Minimal code |
| **Iteration** | ✅ N/A (no thread-local storage) |
| **API Change** | ❌ Changes semantics |
| **Compatibility** | ❌ May break existing code |

### Files Removed: 11
### API Changes: 1 (Initialize semantics)
### Time: 2-3 hours
### Risk: **MEDIUM-HIGH**

---

## Summary Table

| Criterion | Option 1 | Option 2 | Option 3 | Option 4 |
|-----------|----------|----------|----------|----------|
| **Complexity** | ⭐ | ⭐⭐ | ⭐⭐⭐ | ⭐⭐ |
| **Risk** | ⭐ | ⭐⭐ | ⭐⭐⭐ | ⭐⭐ |
| **Performance** | ⭐⭐⭐ | ⭐ | ⭐⭐⭐ | ⭐⭐⭐ |
| **Maintains Features** | ❌ | ✅ | ✅ | ❌ |
| **Time (hours)** | 1-2 | 3-4 | 6-8 | 2-3 |
| **Recommended** | ✅ | ⚠️ | ❌ | ⚠️ |


