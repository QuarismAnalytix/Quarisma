# PyTorch Profiler Table - Column Specifications

## Column Definitions

### 1. **Name** (String, Left-Aligned)
- **Description**: Operation or scope name
- **Format**: Hierarchical with indentation (2-3 spaces per level)
- **Example**: `model_inference`, `  aten::linear`, `    aten::addmm`
- **Width**: Variable (minimum 20 chars)
- **Source**: `profiler_scope_data::name_`

### 2. **Self CPU %** (Percentage, Right-Aligned)
- **Description**: Percentage of total session time spent in this operation (excluding children)
- **Formula**: `(self_cpu_time / session_total_time) * 100`
- **Format**: `XX.XX%` (2 decimal places)
- **Example**: `10.17%`, `8.35%`, `49.88%`
- **Width**: 12 chars
- **Calculation**: Requires self-time calculation

### 3. **Self CPU** (Time, Right-Aligned)
- **Description**: Absolute time spent in operation (excluding children)
- **Formula**: `total_time - sum(children_times)`
- **Format**: Auto-select unit (ms, us, ns)
- **Example**: `8.473ms`, `6.962ms`, `41.565ms`
- **Width**: 12 chars
- **Precision**: 3 decimal places

### 4. **CPU total %** (Percentage, Right-Aligned)
- **Description**: Percentage of total session time (including children)
- **Formula**: `(total_cpu_time / session_total_time) * 100`
- **Format**: `XX.XX%` (2 decimal places)
- **Example**: `100.00%`, `83.19%`, `57.08%`
- **Width**: 12 chars
- **Note**: Usually >= Self CPU %

### 5. **CPU total** (Time, Right-Aligned)
- **Description**: Total time including all nested operations
- **Formula**: `end_time - start_time`
- **Format**: Auto-select unit (ms, us, ns)
- **Example**: `83.334ms`, `69.327ms`, `47.564ms`
- **Width**: 12 chars
- **Precision**: 3 decimal places

### 6. **CPU time avg** (Time, Right-Aligned)
- **Description**: Average time per call
- **Formula**: `total_cpu_time / call_count`
- **Format**: Auto-select unit (ms, us, ns)
- **Example**: `83.334ms`, `34.664ms`, `23.782ms`
- **Width**: 12 chars
- **Precision**: 3 decimal places
- **Note**: For aggregated rows, this is meaningful

### 7. **Self CUDA** (Time, Right-Aligned)
- **Description**: GPU time spent in operation (excluding children)
- **Formula**: `cuda_end_time - cuda_start_time - sum(children_cuda_times)`
- **Format**: Auto-select unit (ms, us, ns)
- **Example**: `8.251ms`, `6.406ms`, `41.596ms`
- **Width**: 12 chars
- **Precision**: 3 decimal places
- **Note**: Optional, requires CUDA profiling enabled

### 8. **Self CUDA %** (Percentage, Right-Aligned)
- **Description**: Percentage of total session CUDA time
- **Formula**: `(self_cuda_time / session_total_cuda_time) * 100`
- **Format**: `XX.XX%` (2 decimal places)
- **Example**: `9.89%`, `7.68%`, `49.88%`
- **Width**: 12 chars
- **Note**: Optional, requires CUDA profiling enabled

### 9. **CUDA total** (Time, Right-Aligned)
- **Description**: Total GPU time including nested operations
- **Formula**: `cuda_end_time - cuda_start_time`
- **Format**: Auto-select unit (ms, us, ns)
- **Example**: `83.400ms`, `69.510ms`, `48.124ms`
- **Width**: 12 chars
- **Precision**: 3 decimal places
- **Note**: Optional, requires CUDA profiling enabled

### 10. **CUDA time avg** (Time, Right-Aligned)
- **Description**: Average GPU time per call
- **Formula**: `total_cuda_time / call_count`
- **Format**: Auto-select unit (ms, us, ns)
- **Example**: `83.400ms`, `34.755ms`, `24.062ms`
- **Width**: 12 chars
- **Precision**: 3 decimal places
- **Note**: Optional, requires CUDA profiling enabled

### 11. **# of Calls** (Integer, Right-Aligned)
- **Description**: Number of times this operation was called
- **Format**: Integer, no decimals
- **Example**: `1`, `2`, `4`
- **Width**: 12 chars
- **Source**: `profiler_scope_data::call_count_` (aggregated)

## Table Layout

```
                  Name    Self CPU %      Self CPU   CPU total %     CPU total  CPU time avg     Self CUDA   Self CUDA %    CUDA total  CUDA time avg    # of Calls  
----------------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  
       model_inference        10.17%       8.473ms       100.00%      83.334ms      83.334ms       8.251ms         9.89%      83.400ms      83.400ms             1  
          aten::linear         8.35%       6.962ms        83.19%      69.327ms      34.664ms       6.406ms         7.68%      69.510ms      34.755ms             2  
```

## Formatting Rules

1. **Column Separator**: 2 spaces between columns
2. **Header Separator**: Dashes (─) matching column width
3. **Alignment**: Right-aligned for all numeric columns, left-aligned for Name
4. **Padding**: Spaces to fill column width
5. **Decimal Places**: 
   - Percentages: 2 decimals
   - Times: 3 decimals
   - Integers: 0 decimals
6. **Time Unit Selection**:
   - If max time >= 1000ms: use ms
   - If max time >= 1000us: use us
   - Otherwise: use ns
7. **Hierarchical Indentation**: 2 spaces per nesting level

## Aggregation Rules

When aggregating multiple calls to same operation:
- **Name**: Use operation name (no indentation in aggregated view)
- **Self CPU**: Sum of all self times
- **CPU total**: Sum of all total times
- **CPU time avg**: Total / call_count
- **# of Calls**: Count of all invocations
- **Percentages**: Recalculate based on aggregated totals

## Edge Cases

1. **Zero-time operations**: Show `0.000ms` or `0.000us`
2. **Very small times**: Use nanoseconds if < 1 microsecond
3. **Single call**: CPU time avg = CPU total
4. **No CUDA data**: Show `0.000ms` or omit CUDA columns
5. **Root operation**: Usually 100% of CPU total %

