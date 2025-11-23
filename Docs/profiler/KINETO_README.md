# Kineto Profiler Documentation

## Overview

This documentation suite explains how the **Kineto Profiler** works in XSigma, including its architecture, entry points, classes, and functions required to run the profiler.

Kineto is XSigma's high-performance profiling system that captures detailed execution traces of CPU and GPU operations. XSigma integrates Kineto to provide comprehensive performance profiling capabilities.

---

## Documentation Files

### 1. **KINETO_ARCHITECTURE_SUMMARY.md** ⭐ START HERE
High-level overview of Kineto's architecture:
- What is Kineto?
- Three-layer design
- Main entry points (`enableProfiler()`, `disableProfiler()`)
- Core classes and their responsibilities
- Event recording flow
- Thread safety model
- Output formats

**Best for:** Understanding the big picture

---

### 2. **KINETO_PROFILER_GUIDE.md** 📖 DETAILED GUIDE
Comprehensive architecture guide:
- Detailed component descriptions
- Entry point functions with signatures
- Core state management classes
- Event recording callbacks
- Configuration classes and enums
- Result classes
- Execution flow diagrams
- libkineto integration
- Thread safety guarantees
- Key features and usage examples

**Best for:** Deep understanding of each component

---

### 3. **KINETO_QUICK_REFERENCE.md** 🚀 QUICK LOOKUP
Quick reference for developers:
- Entry point functions with examples
- Configuration classes
- Result classes
- State management
- Callback functions
- Thread support functions
- libkineto integration
- Common patterns
- File locations
- Output format examples

**Best for:** Quick API lookup while coding

---

### 4. **KINETO_IMPLEMENTATION_DETAILS.md** 🔧 TECHNICAL DEEP DIVE
Implementation-level details:
- Internal architecture
- State stack management
- Event collection mechanism
- Clock conversion
- libkineto integration points
- Callback registration
- Memory management
- Thread safety guarantees
- Post-processing callbacks
- Error handling
- Performance considerations
- Debugging and diagnostics
- Extension points
- Testing patterns

**Best for:** Implementing features or debugging issues

---

## Quick Start

### Basic Profiling Example

```cpp
#include <xsigma/csrc/autograd/profiler_kineto.h>

// 1. Configure profiler
xsigma::profiler::impl::ProfilerConfig config(
    xsigma::profiler::impl::ProfilerState::KINETO,
    true,   // report_input_shapes
    true,   // profile_memory
    true,   // with_stack
    false,  // with_flops
    false   // with_modules
);

// 2. Specify activities to profile
std::set<xsigma::profiler::impl::ActivityType> activities{
    xsigma::profiler::impl::ActivityType::CPU
};

// 3. Start profiling
xsigma::autograd::profiler::enableProfiler(config, activities);

// 4. Run code to profile
// ... your code here ...

// 5. Stop profiling and get results
auto result = xsigma::autograd::profiler::disableProfiler();

// 6. Save trace
result->save("profile_trace.json");

// 7. Access events
for (const auto& event : result->events()) {
    std::cout << "Event: " << event.name() << "\n"
              << "  Duration: " << event.durationNs() << " ns\n"
              << "  Device: " << static_cast<int>(event.deviceType()) << "\n";
}
```

---

## Key Concepts

### Entry Points (Main Functions)

| Function | Purpose | Location |
|----------|---------|----------|
| `enableProfiler()` | Start profiling | `profiler_kineto.cpp:834` |
| `disableProfiler()` | Stop profiling, return results | `profiler_kineto.cpp:915` |
| `enableProfilerInChildThread()` | Enable profiling in child thread | `profiler_kineto.cpp:896` |
| `disableProfilerInChildThread()` | Disable profiling in child thread | `profiler_kineto.cpp:908` |
| `isProfilerEnabledInMainThread()` | Check if main thread is profiling | `profiler_kineto.cpp:891` |

### Core Classes

| Class | Purpose | Location |
|-------|---------|----------|
| `KinetoThreadLocalState` | Thread-local profiler state | `profiler_kineto.cpp:390` |
| `ProfilerConfig` | Profiler configuration | `observer.h:140` |
| `KinetoEvent` | Individual profiled event | `profiler_kineto.h:28` |
| `ProfilerResult` | Final profiling output | `profiler_kineto.h:92` |
| `ActivityTraceWrapper` | libkineto trace wrapper | `kineto_shim.h:91` |
| `RecordQueue` | Event collection buffer | `collection.h` |

### Configuration Enums

| Enum | Purpose | Location |
|------|---------|----------|
| `ProfilerState` | Profiler mode (KINETO, NVTX, ITT, etc.) | `observer.h:32` |
| `ActivityType` | What to profile (CPU, CUDA, XPU, etc.) | `observer.h:14` |
| `RecordScope` | Which functions to capture | `record_function.h:31` |

---

## Architecture Layers

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

## Execution Flow

### Starting Profiler
```
enableProfiler(config, activities, scopes)
    ↓
Create KinetoThreadLocalState
    ↓
Initialize RecordQueue
    ↓
Register onFunctionEnter/Exit callbacks
    ↓
Initialize libkineto trace
    ↓
Profiling Active
```

### During Profiling
```
Function Execution
    ↓
onFunctionEnter() → recordQueue.begin_op()
    ↓
Function Body Executes
    ↓
onFunctionExit() → recordQueue.end_op()
    ↓
Event Stored in RecordQueue
```

### Stopping Profiler
```
disableProfiler()
    ↓
Stop libkineto trace collection
    ↓
Convert RecordQueue events to KinetoEvent objects
    ↓
Finalize ActivityTraceWrapper
    ↓
Return ProfilerResult
    ↓
User can save/analyze results
```

---

## File Organization

```
Library/Core/profiler/pytroch_profiler/
├── profiler_kineto.h          # Main API header
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
        "backend": "CPU"
      }
    }
  ]
}
```

**Viewable in:**
- Chrome DevTools (chrome://tracing)
- Perfetto (ui.perfetto.dev)
- XSigma TensorBoard plugin

---

## Key Features

✅ **CPU Profiling** - Captures all CPU operations  
✅ **GPU Profiling** - CUDA, XPU, HPU support  
✅ **Memory Tracking** - Allocation/deallocation events  
✅ **Stack Traces** - Optional call stack capture  
✅ **Tensor Metadata** - Shapes, dtypes, concrete inputs  
✅ **Module Hierarchy** - XSigma module structure  
✅ **Correlation IDs** - Link CPU and GPU events  
✅ **Thread-Safe** - Per-thread and global profiling modes  
✅ **Extensible** - Custom backend support (PrivateUse1)  
✅ **Post-Processing** - Optional event enrichment callbacks

---

## Common Use Cases

### 1. Profile CPU Operations
```cpp
ProfilerConfig config(ProfilerState::KINETO);
std::set<ActivityType> activities{ActivityType::CPU};
enableProfiler(config, activities);
// ... code ...
auto result = disableProfiler();
result->save("cpu_profile.json");
```

### 2. Profile with Memory Tracking
```cpp
ProfilerConfig config(ProfilerState::KINETO, false, true);  // profile_memory=true
std::set<ActivityType> activities{ActivityType::CPU};
enableProfiler(config, activities);
// ... code ...
auto result = disableProfiler();
```

### 3. Profile with Stack Traces
```cpp
ProfilerConfig config(ProfilerState::KINETO, false, false, true);  // with_stack=true
std::set<ActivityType> activities{ActivityType::CPU};
enableProfiler(config, activities);
// ... code ...
auto result = disableProfiler();
```

### 4. Multi-Thread Profiling
```cpp
// Main thread
enableProfiler(config, activities);

// Child thread
if (isProfilerEnabledInMainThread()) {
    enableProfilerInChildThread();
    // ... code ...
    disableProfilerInChildThread();
}

// Main thread
auto result = disableProfiler();
```

---

## Thread Safety

- **Thread-Local State:** Each thread maintains independent profiler state
- **Global vs. Per-Thread:** Profiler can run globally (all threads) or per-thread
- **No Synchronization Needed:** Event recording is lock-free
- **Child Thread Support:** Threads can join/leave profiling dynamically

---

## Performance Impact

- **CPU Profiling:** ~5-10% overhead
- **Memory Profiling:** +5-15% overhead
- **Stack Capture:** +10-20% overhead
- **GPU Profiling:** Minimal (GPU-side collection)

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| "Profiler already enabled" | Call `disableProfiler()` before `enableProfiler()` |
| No GPU events | Use `KINETO_GPU_FALLBACK` or check CUPTI availability |
| Missing events | Verify `RecordScope` includes desired function types |
| High overhead | Disable unnecessary features (shapes, memory, stacks) |
| Memory bloat | Profile shorter duration or increase buffer size |

---

## Next Steps

1. **Start with:** KINETO_ARCHITECTURE_SUMMARY.md
2. **Deep dive:** KINETO_PROFILER_GUIDE.md
3. **Quick lookup:** KINETO_QUICK_REFERENCE.md
4. **Implementation:** KINETO_IMPLEMENTATION_DETAILS.md

---

## Related Resources

- **XSigma Kineto:** https://github.com/pytorch/kineto
- **Chrome Tracing:** https://www.chromium.org/developers/how-tos/trace-event-profiling-tool
- **Perfetto:** https://ui.perfetto.dev
- **XSigma Profiler API:** `Library/Core/profiler/profiler_api.h`

