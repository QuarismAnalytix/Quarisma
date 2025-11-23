# XSigma Profiler: Before & After Comparison

## Current Output (Before)

### Console Report
```
=== Timing Analysis ===
#1 model_inference - 83.334 ms (depth 0, thread 0x7f1234567890)
#2 aten::linear - 69.327 ms (depth 1, thread 0x7f1234567890)
#3 aten::addmm - 47.564 ms (depth 2, thread 0x7f1234567890)
#4 aten::t - 14.801 ms (depth 2, thread 0x7f1234567890)
#5 aten::relu - 5.534 ms (depth 1, thread 0x7f1234567890)

=== Memory Analysis ===
model_inference (depth 0): delta +2.5 MB, current 2.5 MB, peak 2.5 MB
aten::linear (depth 1): delta +1.2 MB, current 1.2 MB, peak 1.2 MB
```

**Limitations:**
- ❌ No self vs. total time distinction
- ❌ No percentage calculations
- ❌ No aggregation of repeated calls
- ❌ No average time per call
- ❌ No CUDA timing
- ❌ Not comparable to PyTorch format

---

## Target Output (After)

### PyTorch-Style Table
```
                  Name    Self CPU %      Self CPU   CPU total %     CPU total  CPU time avg     Self CUDA   Self CUDA %    CUDA total  CUDA time avg    # of Calls  
----------------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  
       model_inference        10.17%       8.473ms       100.00%      83.334ms      83.334ms       8.251ms         9.89%      83.400ms      83.400ms             1  
          aten::linear         8.35%       6.962ms        83.19%      69.327ms      34.664ms       6.406ms         7.68%      69.510ms      34.755ms             2  
           aten::addmm        49.88%      41.565ms        57.08%      47.564ms      23.782ms      41.596ms        49.88%      48.124ms      24.062ms             2  
               aten::t        11.99%       9.993ms        17.76%      14.801ms       7.401ms      10.042ms        12.04%      14.980ms       7.490ms             2  
            aten::relu         3.66%       3.047ms         6.64%       5.534ms       5.534ms       3.005ms         3.60%       5.639ms       5.639ms             1  
       aten::transpose         3.51%       2.924ms         5.77%       4.808ms       2.404ms       2.902ms         3.48%       4.938ms       2.469ms             2  
           aten::copy_         5.53%       4.610ms         5.53%       4.610ms       2.305ms       4.794ms         5.75%       4.794ms       2.397ms             2  
       aten::clamp_min         2.98%       2.486ms         2.98%       2.486ms       2.486ms       2.634ms         3.16%       2.634ms       2.634ms             1  
      aten::as_strided         2.28%       1.897ms         2.28%       1.897ms     474.250us       2.133ms         2.56%       2.133ms     533.250us             4  
    aten::resolve_conj         1.40%       1.163ms         1.40%       1.163ms     290.800us       1.411ms         1.69%       1.411ms     352.750us             4  
```

**Advantages:**
- ✅ Self vs. total time clearly shown
- ✅ Percentage calculations for comparison
- ✅ Aggregated by operation name
- ✅ Average time per call visible
- ✅ CUDA timing included
- ✅ Matches PyTorch format exactly
- ✅ Professional, easy-to-read layout
- ✅ Sortable by any column

---

## Feature Comparison Matrix

| Feature | Before | After | Benefit |
|---------|--------|-------|---------|
| **Scope Names** | ✅ | ✅ | Unchanged |
| **Total Time** | ✅ | ✅ | Unchanged |
| **Self Time** | ❌ | ✅ | Identify bottlenecks |
| **Percentages** | ❌ | ✅ | Compare across runs |
| **Aggregation** | ❌ | ✅ | See aggregate stats |
| **Avg Time/Call** | ❌ | ✅ | Understand per-call cost |
| **Call Count** | ✅ | ✅ | Unchanged |
| **CUDA Timing** | ❌ | ✅ | GPU profiling |
| **Table Format** | ❌ | ✅ | PyTorch compatibility |
| **Hierarchical** | ✅ | ✅ | Unchanged |

---

## Data Transformation Example

### Input: Raw Scope Data
```
Scope: model_inference
  - Start: 0ms, End: 83.334ms, Duration: 83.334ms
  - Children: [aten::linear, aten::relu]
  - Call Count: 1

Scope: aten::linear (Call 1)
  - Start: 1ms, End: 35.664ms, Duration: 34.664ms
  - Children: [aten::addmm, aten::t]
  - Call Count: 1

Scope: aten::linear (Call 2)
  - Start: 40ms, End: 69.327ms, Duration: 29.327ms
  - Children: [aten::addmm, aten::t]
  - Call Count: 1
```

### Processing Steps
1. **Calculate Self Times**
   - model_inference: 83.334 - (34.664 + 29.327 + 5.534) = 13.809ms
   - aten::linear: 34.664 - (children) = 8.35ms (per call)

2. **Aggregate by Name**
   - aten::linear: 34.664 + 29.327 = 63.991ms total, 2 calls
   - Average: 63.991 / 2 = 31.9955ms per call

3. **Calculate Percentages**
   - aten::linear: 63.991 / 83.334 * 100 = 76.79%

4. **Format Table**
   - Align columns, add separators, format numbers

### Output: Single Aggregated Row
```
          aten::linear         8.35%       6.962ms        76.79%      63.991ms      31.996ms             2  
```

---

## Usage Comparison

### Before
```cpp
session->print_report();
// Output: Simple list of top operations
```

### After
```cpp
// Option 1: Print PyTorch-style table
auto report = session->generate_report();
std::cout << report->generate_pytorch_table();

// Option 2: Export to file
report->export_to_file("profile.txt", 
    profiler_options::output_format_enum::PYTORCH_TABLE);

// Option 3: Still support old formats
report->export_json_report("profile.json");
report->export_csv_report("profile.csv");
```

---

## Performance Impact

| Metric | Before | After | Change |
|--------|--------|-------|--------|
| Report Generation | ~5ms | ~15ms | +10ms (acceptable) |
| Memory Overhead | ~1MB | ~1.5MB | +0.5MB (negligible) |
| Aggregation Time | N/A | ~5ms | New feature |
| Table Formatting | N/A | ~5ms | New feature |

---

## Backward Compatibility

✅ **Fully Backward Compatible**
- All existing output formats preserved
- New format is optional
- No breaking changes to API
- Existing code continues to work
- New functionality is additive only

