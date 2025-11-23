# PyTorch-Style Profiler Table Implementation Checklist

## Quick Summary
To generate PyTorch-style profiler output, the XSigma Non-Naive Profiler needs **6 key enhancements**:

| # | Feature | Status | Priority | Effort | Dependencies |
|---|---------|--------|----------|--------|--------------|
| 1 | Self CPU Time Calculation | ❌ | HIGH | 2-3h | None |
| 2 | CPU Total % Calculation | ❌ | HIGH | 1-2h | #1 |
| 3 | CPU Time Average | ✅ | MEDIUM | 0.5h | Existing call_count |
| 4 | Operation Aggregation | ❌ | HIGH | 3-4h | #1, #2 |
| 5 | Table Formatting | ❌ | MEDIUM | 2-3h | #1-4 |
| 6 | CUDA Timing Support | ❌ | LOW | 4-6h | Kineto integration |

## Implementation Roadmap

### Phase 1: Core Calculations (Est. 3-5 hours)
- [ ] Add `calculate_self_cpu_time()` method to `profiler_report`
- [ ] Add `calculate_cpu_percentage()` method
- [ ] Update `profiler_scope_data` with `self_cpu_time_ms_` field
- [ ] Write unit tests for calculations
- [ ] Verify with sample data

### Phase 2: Operation Aggregation (Est. 3-4 hours)
- [ ] Create `aggregated_operation_stats` struct
- [ ] Implement aggregation logic in `profiler_report`
- [ ] Group scopes by operation name
- [ ] Aggregate timing and call count statistics
- [ ] Write aggregation tests

### Phase 3: Table Generation (Est. 2-3 hours)
- [ ] Create `generate_pytorch_table()` method
- [ ] Implement column formatting and alignment
- [ ] Add hierarchical indentation
- [ ] Add separator lines
- [ ] Test with various data sizes

### Phase 4: Integration (Est. 1-2 hours)
- [ ] Add new output format enum: `PYTORCH_TABLE`
- [ ] Wire into `export_to_file()` method
- [ ] Add to console report generation
- [ ] Update documentation

### Phase 5: CUDA Support (Optional, Est. 4-6 hours)
- [ ] Add CUDA timing fields to `profiler_scope_data`
- [ ] Integrate with Kineto backend
- [ ] Track CUDA start/end times
- [ ] Calculate CUDA self time
- [ ] Add CUDA columns to table

## Key Data Structures

### Current Available
```cpp
profiler_scope_data {
    std::string name_;
    std::chrono::high_resolution_clock::time_point start_time_;
    std::chrono::high_resolution_clock::time_point end_time_;
    size_t call_count_;  // ✅ Already tracked
    std::vector<unique_ptr<profiler_scope_data>> children_;
}
```

### Needed Additions
```cpp
profiler_scope_data {
    double self_cpu_time_ms_;  // NEW
    // Optional CUDA fields:
    std::chrono::high_resolution_clock::time_point cuda_start_time_;
    std::chrono::high_resolution_clock::time_point cuda_end_time_;
}

aggregated_operation_stats {  // NEW
    std::string name;
    double total_cpu_time_ms;
    double self_cpu_time_ms;
    size_t call_count;
    double cpu_time_avg_ms;
    // Optional CUDA fields
}
```

## Files to Modify

1. **profiler.h** - Add fields to `profiler_scope_data`
2. **profiler.cpp** - Implement self-time calculation
3. **profiler_report.h** - Add new method declarations
4. **profiler_report.cpp** - Implement all calculations and table generation
5. **profiler_options.h** - Add `PYTORCH_TABLE` format enum

## Testing Strategy

- Unit tests for each calculation method
- Integration tests with sample profiling data
- Comparison tests with PyTorch output format
- Edge case tests (single call, nested scopes, zero-time ops)
- Performance tests (large scope hierarchies)

## Success Criteria

- [ ] Table matches PyTorch format exactly
- [ ] All columns populated correctly
- [ ] Hierarchical indentation preserved
- [ ] Aggregation accurate across multiple calls
- [ ] Performance acceptable (< 100ms for 1000 scopes)
- [ ] 98%+ code coverage on new code
- [ ] All tests passing

