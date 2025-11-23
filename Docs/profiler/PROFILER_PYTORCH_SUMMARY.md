# XSigma Non-Naive Profiler → PyTorch-Style Output: Executive Summary

## Goal
Enable XSigma's Non-Naive Profiler to generate output matching PyTorch's profiler table format with columns for CPU/CUDA timing, percentages, and call counts.

## Current State
✅ **What We Have:**
- Hierarchical scope tracking with names
- CPU timing (start/end timestamps)
- Call count tracking
- Memory statistics
- Thread information
- Multiple output formats (JSON, CSV, XML, Console)

❌ **What We're Missing:**
- Self CPU time (time excluding children)
- Percentage calculations (% of total session)
- Operation aggregation (group by name)
- PyTorch-style table formatting
- CUDA timing integration (optional)

## The Gap: 6 Key Enhancements

### 1. **Self CPU Time** (HIGH Priority)
- **What**: Time spent in operation excluding all child operations
- **Why**: Essential for understanding where time is actually spent
- **How**: Subtract sum of children times from total time
- **Effort**: 2-3 hours

### 2. **Percentage Calculations** (HIGH Priority)
- **What**: CPU time as percentage of total session time
- **Why**: Enables comparison across different profiling runs
- **How**: `(operation_time / session_total_time) * 100`
- **Effort**: 1-2 hours

### 3. **Operation Aggregation** (HIGH Priority)
- **What**: Group multiple calls to same operation into single row
- **Why**: PyTorch shows aggregated stats, not individual calls
- **How**: Group by name, sum times, aggregate statistics
- **Effort**: 3-4 hours

### 4. **Table Formatting** (MEDIUM Priority)
- **What**: Generate fixed-width, aligned table matching PyTorch format
- **Why**: Professional output, easy to read and compare
- **How**: Implement column formatting with proper alignment
- **Effort**: 2-3 hours

### 5. **CUDA Timing** (LOW Priority, Optional)
- **What**: Track GPU execution time separately from CPU
- **Why**: Essential for GPU-accelerated workloads
- **How**: Integrate with Kineto/CUDA backend
- **Effort**: 4-6 hours

### 6. **CPU Time Average** (ALREADY DONE ✅)
- **What**: Average time per call
- **How**: `total_time / call_count`
- **Status**: Call count already tracked

## Implementation Timeline

| Phase | Tasks | Duration | Cumulative |
|-------|-------|----------|-----------|
| 1 | Self CPU + Percentages | 3-5h | 3-5h |
| 2 | Aggregation | 3-4h | 6-9h |
| 3 | Table Formatting | 2-3h | 8-12h |
| 4 | Integration | 1-2h | 9-14h |
| 5 | CUDA Support (opt) | 4-6h | 13-20h |

**Estimated Total (Core): 9-14 hours**
**With CUDA: 13-20 hours**

## Example Output

### Before (Current)
```
=== Timing Analysis ===
#1 model_inference - 83.334 ms (depth 0, thread 0x1234)
#2 aten::linear - 69.327 ms (depth 1, thread 0x1234)
#3 aten::addmm - 47.564 ms (depth 2, thread 0x1234)
```

### After (PyTorch-Style)
```
                  Name    Self CPU %      Self CPU   CPU total %     CPU total  CPU time avg    # of Calls  
       model_inference        10.17%       8.473ms       100.00%      83.334ms      83.334ms             1  
          aten::linear         8.35%       6.962ms        83.19%      69.327ms      34.664ms             2  
           aten::addmm        49.88%      41.565ms        57.08%      47.564ms      23.782ms             2  
```

## Key Files to Modify

1. `Library/Core/profiler/native/session/profiler.h` - Add fields
2. `Library/Core/profiler/native/session/profiler.cpp` - Implement calculations
3. `Library/Core/profiler/native/session/profiler_report.h` - Add methods
4. `Library/Core/profiler/native/session/profiler_report.cpp` - Implement table generation
5. `Library/Core/profiler/native/session/profiler_options.h` - Add format enum

## Success Metrics

- ✅ Table format matches PyTorch exactly
- ✅ All 11 columns populated correctly
- ✅ Hierarchical indentation preserved
- ✅ Aggregation accurate across calls
- ✅ Performance: < 100ms for 1000 scopes
- ✅ Code coverage: 98%+
- ✅ All tests passing

## Next Steps

1. Review this analysis with team
2. Prioritize: Core (Phases 1-4) vs Optional (Phase 5)
3. Create detailed design for aggregation logic
4. Begin Phase 1 implementation
5. Write comprehensive unit tests
6. Validate against PyTorch output

## Related Documentation

- `PROFILER_PYTORCH_TABLE_FORMAT.md` - Detailed requirements
- `PROFILER_TABLE_COLUMNS.md` - Column specifications
- `PROFILER_IMPLEMENTATION_CHECKLIST.md` - Task breakdown

