# PyTorch-Style Profiler: Quick Reference Card

## 🎯 The Goal
Generate PyTorch-compatible profiler output with 11 columns showing CPU/CUDA timing, percentages, and call counts.

## 📊 The 11 Columns
```
Name | Self CPU % | Self CPU | CPU total % | CPU total | CPU time avg | 
Self CUDA | Self CUDA % | CUDA total | CUDA time avg | # of Calls
```

## 🔢 Key Formulas

| Metric | Formula | Example |
|--------|---------|---------|
| **Self CPU Time** | Total - Sum(Children) | 83.334 - 78.861 = 4.473ms |
| **Self CPU %** | (Self / Session Total) × 100 | (4.473 / 83.334) × 100 = 5.37% |
| **CPU Total %** | (Total / Session Total) × 100 | (83.334 / 83.334) × 100 = 100% |
| **CPU Time Avg** | Total / Call Count | 69.327 / 2 = 34.664ms |
| **CUDA Self Time** | CUDA Total - Sum(Children CUDA) | Similar to CPU |
| **CUDA Self %** | (CUDA Self / Session CUDA Total) × 100 | Similar to CPU % |

## 🛠️ Implementation Phases

| Phase | Tasks | Time | Files |
|-------|-------|------|-------|
| 1 | Self time + % calc | 3-5h | profiler.h/cpp, profiler_report.cpp |
| 2 | Aggregation logic | 3-4h | profiler_report.cpp |
| 3 | Table formatting | 2-3h | profiler_report.cpp |
| 4 | Integration | 1-2h | profiler_options.h |
| 5 | CUDA support | 4-6h | profiler.h/cpp, profiler_report.cpp |

**Total: 9-14 hours (core), 13-20 hours (with CUDA)**

## 📝 Data Structures

### Add to `profiler_scope_data`
```cpp
double self_cpu_time_ms_ = 0.0;
// Optional CUDA fields:
std::chrono::high_resolution_clock::time_point cuda_start_time_;
std::chrono::high_resolution_clock::time_point cuda_end_time_;
```

### New: `aggregated_operation_stats`
```cpp
struct aggregated_operation_stats {
    std::string name;
    double total_cpu_time_ms;
    double self_cpu_time_ms;
    size_t call_count;
    // ... helper methods for calculations
};
```

## 🔧 Key Methods to Implement

1. **`calculate_self_cpu_time(scope)`**
   - Returns: `total_time - sum(children_times)`

2. **`calculate_cpu_percentage(time, session_total)`**
   - Returns: `(time / session_total) * 100`

3. **`aggregate_by_operation(root)`**
   - Returns: `vector<aggregated_operation_stats>`
   - Groups scopes by name, sums times

4. **`format_pytorch_table(stats, session_total)`**
   - Returns: Formatted table string
   - Fixed-width columns, aligned values

5. **`generate_pytorch_table()`**
   - Public API method
   - Orchestrates all above methods

## 📂 Files to Modify

```
Library/Core/profiler/native/session/
├── profiler.h              ← Add fields to profiler_scope_data
├── profiler.cpp            ← Implement self-time calculation
├── profiler_report.h       ← Add method declarations
├── profiler_report.cpp     ← Implement all calculations & formatting
└── profiler_options.h      ← Add PYTORCH_TABLE format enum
```

## ✅ Success Checklist

- [ ] Self CPU time calculated correctly
- [ ] Percentages match PyTorch format
- [ ] Aggregation groups by operation name
- [ ] Table columns aligned properly
- [ ] Hierarchical indentation preserved
- [ ] All 11 columns populated
- [ ] Performance < 100ms for 1000 scopes
- [ ] 98%+ code coverage
- [ ] All tests passing
- [ ] Documentation complete

## 🧪 Test Coverage

```cpp
// Test self CPU time calculation
XSIGMATEST(profiler_report_test, self_cpu_time_calculation) { ... }

// Test percentage calculations
XSIGMATEST(profiler_report_test, cpu_percentage_calculation) { ... }

// Test aggregation
XSIGMATEST(profiler_report_test, operation_aggregation) { ... }

// Test table formatting
XSIGMATEST(profiler_report_test, pytorch_table_format) { ... }

// Test edge cases
XSIGMATEST(profiler_report_test, single_call_aggregation) { ... }
XSIGMATEST(profiler_report_test, nested_scopes) { ... }
XSIGMATEST(profiler_report_test, zero_time_operations) { ... }
```

## 📖 Documentation Files

| File | Purpose |
|------|---------|
| **PROFILER_PYTORCH_SUMMARY.md** | Executive overview |
| **PROFILER_PYTORCH_TABLE_FORMAT.md** | Detailed requirements |
| **PROFILER_TABLE_COLUMNS.md** | Column specifications |
| **PROFILER_CODE_EXAMPLES.md** | Implementation code |
| **PROFILER_IMPLEMENTATION_CHECKLIST.md** | Task breakdown |
| **PROFILER_BEFORE_AFTER.md** | Comparison |
| **PROFILER_QUICK_REFERENCE.md** | This file |

## 🚀 Getting Started

1. Read `PROFILER_PYTORCH_SUMMARY.md`
2. Review `PROFILER_TABLE_COLUMNS.md`
3. Study `PROFILER_CODE_EXAMPLES.md`
4. Follow `PROFILER_IMPLEMENTATION_CHECKLIST.md`
5. Reference `PROFILER_PYTORCH_TABLE_FORMAT.md` as needed

## 💡 Pro Tips

- Start with Phase 1 (calculations) - foundation for everything
- Write tests as you implement each method
- Use existing `call_count_` field - already tracked!
- Aggregation is the most complex part - plan carefully
- CUDA support is optional - can be added later
- Maintain backward compatibility - don't break existing code

## 🎓 Key Insights

- **Self time** = Time in operation minus time in children
- **Aggregation** = Group by name, sum times, count calls
- **Percentages** = Relative to session total, enables comparison
- **Table format** = Professional output, PyTorch-compatible
- **CUDA** = Optional enhancement for GPU profiling

---

**Status**: Analysis Complete ✅  
**Next**: Implementation Planning  
**Estimated Effort**: 9-14 hours (core)

