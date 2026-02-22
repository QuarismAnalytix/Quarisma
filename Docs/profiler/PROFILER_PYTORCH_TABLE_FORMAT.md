# PyTorch-Style Profiler Table Format for Quarisma Non-Naive Profiler

## Overview
This document outlines what's needed for the Quarisma Non-Naive Profiler to generate output similar to PyTorch's profiler table format shown in the reference example.

## Reference PyTorch Output Format
```
                  Name    Self CPU %      Self CPU   CPU total %     CPU total  CPU time avg     Self CUDA   Self CUDA %    CUDA total  CUDA time avg    # of Calls  
       model_inference        10.17%       8.473ms       100.00%      83.334ms      83.334ms       8.251ms         9.89%      83.400ms      83.400ms             1  
          aten::linear         8.35%       6.962ms        83.19%      69.327ms      34.664ms       6.406ms         7.68%      69.510ms      34.755ms             2  
```

## Current Quarisma Profiler Capabilities

### Available Data
- ✅ Scope names (hierarchical)
- ✅ CPU timing (start_time_, end_time_)
- ✅ Call counts (tracked per scope)
- ✅ Memory statistics (current, peak, delta)
- ✅ Thread information
- ✅ Hierarchical relationships (parent/child scopes)
- ✅ Timing statistics (min, max, mean, std_dev, percentiles)

### Missing/Incomplete Data
- ❌ CUDA timing (Self CUDA, CUDA total)
- ❌ Self CPU % calculation (time excluding children)
- ❌ CPU total % calculation (percentage of root)
- ❌ CPU time avg (average per call)
- ❌ CUDA time avg (average per call)
- ⚠️ Aggregation by operation name (currently per-scope, not aggregated)

## Implementation Requirements

### 1. **Self CPU Time Calculation**
- **Definition**: CPU time spent in scope excluding all child scopes
- **Formula**: `self_cpu_time = total_time - sum(child_times)`
- **Location**: `profiler_report.cpp` - new method `calculate_self_cpu_time()`
- **Impact**: Affects "Self CPU %" and "Self CPU" columns

### 2. **CPU Total % Calculation**
- **Definition**: Percentage of total session time
- **Formula**: `cpu_total_percent = (total_time / session_total_time) * 100`
- **Location**: `profiler_report.cpp` - new method `calculate_cpu_percentage()`
- **Requires**: Session total duration tracking

### 3. **CPU Time Average**
- **Definition**: Average time per call
- **Formula**: `cpu_time_avg = total_time / call_count`
- **Location**: Use existing `call_count_` field in `profiler_scope_data`
- **Note**: Already have call count tracking

### 4. **CUDA Timing Support**
- **Definition**: GPU execution time (separate from CPU)
- **Required Fields**:
  - `cuda_start_time_` (high_resolution_clock)
  - `cuda_end_time_` (high_resolution_clock)
  - `cuda_self_time_` (calculated)
  - `cuda_total_time_` (calculated)
- **Location**: Add to `profiler_scope_data` struct
- **Integration**: Requires Kineto/CUDA profiler backend integration

### 5. **Operation Aggregation**
- **Current**: Each scope instance tracked separately
- **Needed**: Aggregate by operation name across all calls
- **Example**: All `aten::linear` calls → single row with aggregated stats
- **Location**: New aggregation logic in `profiler_report.cpp`
- **Method**: Group scopes by name, sum times, aggregate statistics

### 6. **Table Formatting**
- **Columns** (in order):
  1. Name (left-aligned, hierarchical indent)
  2. Self CPU % (right-aligned, 2 decimals)
  3. Self CPU (right-aligned, time unit)
  4. CPU total % (right-aligned, 2 decimals)
  5. CPU total (right-aligned, time unit)
  6. CPU time avg (right-aligned, time unit)
  7. Self CUDA (right-aligned, time unit)
  8. Self CUDA % (right-aligned, 2 decimals)
  9. CUDA total (right-aligned, time unit)
  10. CUDA time avg (right-aligned, time unit)
  11. # of Calls (right-aligned, integer)

- **Formatting Details**:
  - Fixed-width columns with dashes separator
  - Time units: ms, us, ns (auto-select based on magnitude)
  - Percentage format: `XX.XX%`
  - Hierarchical indentation: 2-3 spaces per level

### 7. **Data Structure Changes**

#### `profiler_scope_data` additions:
```cpp
// CUDA timing (optional, if CUDA support enabled)
std::chrono::high_resolution_clock::time_point cuda_start_time_;
std::chrono::high_resolution_clock::time_point cuda_end_time_;

// Call count tracking
size_t call_count_ = 1;

// Aggregated statistics
double self_cpu_time_ms_ = 0.0;
double cuda_self_time_ms_ = 0.0;
```

#### New aggregation structure:
```cpp
struct aggregated_operation_stats {
    std::string name;
    double total_cpu_time_ms;
    double self_cpu_time_ms;
    double total_cuda_time_ms;
    double self_cuda_time_ms;
    size_t call_count;
    double cpu_time_avg_ms;
    double cuda_time_avg_ms;
};
```

## Implementation Steps

1. **Phase 1**: Self CPU time calculation
   - Add `calculate_self_cpu_time()` method
   - Update `generate_timing_section()` to show self vs total

2. **Phase 2**: Percentage calculations
   - Add session total duration tracking
   - Implement percentage calculation methods

3. **Phase 3**: Operation aggregation
   - Create aggregation logic
   - Group scopes by name
   - Aggregate statistics

4. **Phase 4**: Table formatting
   - Implement PyTorch-style table generator
   - Add column alignment and formatting
   - Create new output format option

5. **Phase 5**: CUDA support (optional)
   - Integrate with Kineto/CUDA backend
   - Track CUDA timing separately
   - Add CUDA columns to table

## Files to Modify

- `Library/Core/profiler/native/session/profiler.h` - Add fields to `profiler_scope_data`
- `Library/Core/profiler/native/session/profiler.cpp` - Implement calculations
- `Library/Core/profiler/native/session/profiler_report.h` - Add new methods
- `Library/Core/profiler/native/session/profiler_report.cpp` - Implement table generation
- `Library/Core/profiler/native/session/profiler_options.h` - Add new output format enum

## Testing Considerations

- Unit tests for self CPU time calculation
- Unit tests for percentage calculations
- Integration tests with sample profiling data
- Comparison tests with PyTorch profiler output format
- Edge cases: single call, nested scopes, zero-time operations

