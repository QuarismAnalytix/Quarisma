# Kineto Profiler Architecture Summary

## What is Kineto?

**Kineto** is Quarisma's high-performance profiling library that captures detailed execution traces of CPU and GPU operations. Quarisma integrates Kineto to provide comprehensive performance profiling capabilities.

---

## Core Architecture

### Three-Layer Design

```
┌─────────────────────────────────────────┐
│  User API Layer                         │
│  enableProfiler() / disableProfiler()   │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│  State Management Layer                 │
│  KinetoThreadLocalState                 │
│  RecordQueue                            │
│  ProfilerConfig                         │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│  libkineto Integration Layer            │
│  Kineto Shim (kineto_shim.h)            │
│  ActivityTraceWrapper                   │
│  libkineto::ActivityProfiler            │
└─────────────────────────────────────────┘
```

---

## Main Entry Points

### 1. **enableProfiler()** - Start Profiling
**Location:** `profiler_kineto.cpp:834`

**Signature:**
```cpp
void enableProfiler(
    const ProfilerConfig& config,
    const std::set<ActivityType>& activities,
    const std::unordered_set<RecordScope>& scopes = {});
```

**Responsibilities:**
1. Validate no profiler already running
2. Create `KinetoThreadLocalState` for current thread
3. Initialize `RecordQueue` for event collection
4. Register `onFunctionEnter()` and `onFunctionExit()` callbacks
5. Initialize libkineto trace collection
6. Store state for child thread access

**Key Parameters:**
- `config`: Profiler settings (state, memory tracking, stack capture, etc.)
- `activities`: What to profile (CPU, CUDA, XPU, etc.)
- `scopes`: Which function types to capture (FUNCTION, BACKWARD_FUNCTION, USER_SCOPE, etc.)

---

### 2. **disableProfiler()** - Stop Profiling
**Location:** `profiler_kineto.cpp:915`

**Signature:**
```cpp
std::unique_ptr<ProfilerResult> disableProfiler();
```

**Responsibilities:**
1. Remove registered callbacks
2. Stop libkineto trace collection
3. Finalize `ActivityTraceWrapper`
4. Convert `RecordQueue` events to `KinetoEvent` objects
5. Build event hierarchy
6. Return `ProfilerResult` with all collected data

**Returns:** `ProfilerResult` containing:
- All recorded events as `KinetoEvent` objects
- Event hierarchy tree
- Trace start timestamp
- libkineto trace wrapper

---

## Core Classes

### **KinetoThreadLocalState**
**Location:** `profiler_kineto.cpp:390`

Manages thread-local profiling state:
- Inherits from `ProfilerStateBase`
- Holds `RecordQueue` for event collection
- Stores `KinetoEvent` vector
- Maintains clock converter (approximate → Unix time)
- Optional post-processing callback

**Key Methods:**
- `get(bool global)` - Retrieve current thread's state
- `reportMemoryUsage()` - Record memory allocations
- `reportVulkanEventToProfiler()` - Record GPU events
- `finalizeTrace()` - Convert to `ProfilerResult`

---

### **ProfilerConfig**
**Location:** `observer.h:140`

Configuration structure:
```cpp
struct ProfilerConfig {
    ProfilerState state;              // KINETO, KINETO_GPU_FALLBACK, etc.
    bool report_input_shapes;         // Capture tensor shapes
    bool profile_memory;              // Track memory allocations
    bool with_stack;                  // Capture call stacks
    bool with_flops;                  // Calculate FLOPs
    bool with_modules;                // Track module hierarchy
    ExperimentalConfig experimental_config;
    std::string trace_id;
};
```

**ProfilerState Options:**
- `KINETO` - Full profiling with libkineto
- `KINETO_GPU_FALLBACK` - CPU + GPU (CUPTI fallback)
- `KINETO_ONDEMAND` - On-demand profiling
- `NVTX` - NVIDIA markers only
- `ITT` - Intel VTune markers
- `PRIVATEUSE1` - Custom backend markers

---

### **KinetoEvent**
**Location:** `profiler_kineto.h:28`

Represents a single profiled event:
```cpp
struct KinetoEvent {
    std::string name();                    // Operation name
    uint64_t startNs();                    // Start time (ns)
    uint64_t endNs();                      // End time (ns)
    uint64_t durationNs();                 // Duration (ns)
    quarisma::device_enum deviceType();      // CPU, CUDA, etc.
    int deviceIndex();                     // Device ID
    uint64_t correlationId();              // Links CPU↔GPU
    
    // Optional metadata
    bool hasShapes();
    const quarisma::array_ref<std::vector<int64_t>> shapes();
    bool hasStack();
    const quarisma::array_ref<std::string> stack();
    std::string backend();
};
```

---

### **ProfilerResult**
**Location:** `profiler_kineto.h:92`

Final output from profiling:
```cpp
struct ProfilerResult {
    uint64_t trace_start_ns() const;
    const std::vector<KinetoEvent>& events() const;
    const std::vector<experimental_event_t>& event_tree() const;
    void save(const std::string& path);
};
```

**Methods:**
- `trace_start_ns()` - Get trace start time
- `events()` - Access all recorded events
- `event_tree()` - Get hierarchical structure
- `save(path)` - Export to JSON/Chrome trace format

---

## Event Recording Flow

### During Profiling

```
Function Execution
    ↓
RecordFunction detects function call
    ↓
onFunctionEnter() callback triggered
    ↓
recordQueue.begin_op(function)
    ↓
Event start recorded with:
  - Function name
  - Thread ID
  - Timestamp
  - Input shapes (if enabled)
    ↓
Function body executes
    ↓
onFunctionExit() callback triggered
    ↓
recordQueue.end_op()
    ↓
Event end recorded with:
  - End timestamp
  - Duration
  - Output metadata
    ↓
Event stored in KinetoThreadLocalState
```

---

## libkineto Integration

### Kineto Shim Layer
**Location:** `kineto_shim.h`

Abstraction over libkineto:
- `prepareTrace()` - Initialize trace collection
- `startTrace()` - Begin recording
- `stopTrace()` - End recording, return `ActivityTraceWrapper`
- `pushCorrelationId()` - Link CPU and GPU events
- `recordThreadInfo()` - Register thread metadata

### Key libkineto Classes
- `libkineto::ActivityProfiler` - Main profiling interface
- `libkineto::CpuTraceBuffer` - Stores CPU events
- `libkineto::ActivityTraceInterface` - Unified trace interface
- `libkineto::GenericTraceActivity` - Individual activity record

---

## Thread Safety

### Thread-Local Storage
- Each thread maintains its own `KinetoThreadLocalState`
- Events are collected independently per thread
- No synchronization needed for event recording

### Global vs. Per-Thread Profiling
- **Global:** Profiler runs on all threads
- **Per-Thread:** Profiler runs only on current thread

### Child Thread Support
```cpp
// Main thread starts profiling
enableProfiler(config, activities);

// Child thread joins profiling
enableProfilerInChildThread();

// Child thread leaves profiling
disableProfilerInChildThread();

// Check if main thread is profiling
bool enabled = isProfilerEnabledInMainThread();
```

---

## Output Format

### Chrome Trace JSON
```json
{
  "traceEvents": [
    {
      "name": "aten::add",
      "ph": "X",
      "ts": 1234567890,
      "dur": 1000,
      "pid": 12345,
      "tid": 67890,
      "args": {
        "shapes": "[[1, 2, 3]]",
        "backend": "CPU",
        "correlation_id": "12345"
      }
    }
  ]
}
```

**Viewable in:**
- Chrome DevTools (chrome://tracing)
- Perfetto (ui.perfetto.dev)
- Quarisma TensorBoard plugin

---

## Callback System

### Function Entry/Exit Hooks
```cpp
// Called when function starts
std::unique_ptr<at::ObserverContext> onFunctionEnter(
    const at::RecordFunction& fn);

// Called when function ends
void onFunctionExit(at::ObserverContext* ctx);
```

### Callback Registration
- Registered via `pushProfilingCallbacks<global>(scopes)`
- Intercepts all `RecordFunction` calls
- Captures function metadata and timing
- Stores in `RecordQueue`

---

## Key Features

✅ **CPU Profiling** - All CPU operations  
✅ **GPU Profiling** - CUDA, XPU, HPU support  
✅ **Memory Tracking** - Allocation/deallocation events  
✅ **Stack Traces** - Optional call stack capture  
✅ **Tensor Metadata** - Shapes, dtypes, concrete inputs  
✅ **Module Hierarchy** - Quarisma module structure  
✅ **Correlation IDs** - Link CPU and GPU events  
✅ **Thread-Safe** - Per-thread and global modes  
✅ **Extensible** - Custom backend support (PrivateUse1)  
✅ **Post-Processing** - Optional event enrichment callbacks

---

## File Organization

```
Library/Core/profiler/pytroch_profiler/
├── profiler_kineto.h          # Main API
├── profiler_kineto.cpp        # Implementation
├── observer.h                 # Config classes
├── observer.cpp               # Config implementation
├── record_function.h          # RecordFunction interface
├── record_function.cpp        # RecordFunction implementation
├── kineto_shim.h              # libkineto wrapper
├── kineto_shim.cpp            # libkineto wrapper impl
├── collection.h               # Event collection
├── collection.cpp             # Event collection impl
├── events.h                   # Event structures
├── containers.h               # Container types
├── api.h                      # API aliases
└── kineto_client_interface.h  # Client interface
```

---

## Usage Pattern

```cpp
// 1. Configure
ProfilerConfig config(ProfilerState::KINETO, true, true, true);

// 2. Set activities
std::set<ActivityType> activities{ActivityType::CPU};

// 3. Start
enableProfiler(config, activities);

// 4. Run code
// ... your code to profile ...

// 5. Stop and save
auto result = disableProfiler();
result->save("trace.json");

// 6. Analyze
for (const auto& event : result->events()) {
    std::cout << event.name() << ": " 
              << event.durationNs() << " ns\n";
}
```

---

## Related Documentation

- **KINETO_PROFILER_GUIDE.md** - Detailed architecture guide
- **KINETO_QUICK_REFERENCE.md** - API quick reference
- **Quarisma Kineto Docs** - https://github.com/pytorch/kineto

