# Kineto Profiler Documentation Index

## 📚 Documentation Suite

This comprehensive documentation explains the Kineto Profiler architecture in XSigma.

### Main Documents

1. **KINETO_README.md** - Start here for overview and quick start
2. **KINETO_ARCHITECTURE_SUMMARY.md** - High-level architecture overview
3. **KINETO_PROFILER_GUIDE.md** - Detailed component guide
4. **KINETO_QUICK_REFERENCE.md** - API quick reference
5. **KINETO_IMPLEMENTATION_DETAILS.md** - Technical deep dive

---

## 🎯 Quick Navigation

### By Use Case

**I want to...**

- **Understand what Kineto is** → KINETO_ARCHITECTURE_SUMMARY.md
- **Learn the architecture** → KINETO_PROFILER_GUIDE.md
- **Write profiling code** → KINETO_QUICK_REFERENCE.md
- **Debug profiler issues** → KINETO_IMPLEMENTATION_DETAILS.md
- **Get started quickly** → KINETO_README.md (Quick Start section)
- **Find a specific API** → KINETO_QUICK_REFERENCE.md (use Ctrl+F)
- **Understand internals** → KINETO_IMPLEMENTATION_DETAILS.md

---

## 📖 Entry Points Reference

### Main Functions

| Function | Purpose | File | Line |
|----------|---------|------|------|
| `enableProfiler()` | Start profiling | profiler_kineto.cpp | 834 |
| `disableProfiler()` | Stop profiling | profiler_kineto.cpp | 915 |
| `enableProfilerInChildThread()` | Enable in child thread | profiler_kineto.cpp | 896 |
| `disableProfilerInChildThread()` | Disable in child thread | profiler_kineto.cpp | 908 |
| `isProfilerEnabledInMainThread()` | Check main thread status | profiler_kineto.cpp | 891 |
| `reportBackendEventToActiveKinetoProfiler()` | Report backend event | profiler_kineto.h | 138 |
| `enableProfilerWithEventPostProcess()` | Enable with post-processing | profiler_kineto.h | 173 |
| `prepareProfiler()` | Prepare profiler | profiler_kineto.h | 181 |
| `toggleCollectionDynamic()` | Toggle collection | profiler_kineto.h | 185 |
| `startMemoryProfile()` | Start memory profiling | profiler_kineto.h | 188 |
| `stopMemoryProfile()` | Stop memory profiling | profiler_kineto.h | 189 |
| `exportMemoryProfile()` | Export memory profile | profiler_kineto.h | 190 |

---

## 🏗️ Core Classes Reference

### State Management

| Class | Purpose | File | Line |
|-------|---------|------|------|
| `KinetoThreadLocalState` | Thread-local state | profiler_kineto.cpp | 390 |
| `ProfilerStateBase` | Base profiler state | observer.h | 173 |
| `ProfilerConfig` | Configuration | observer.h | 140 |
| `ExperimentalConfig` | Experimental options | observer.h | 57 |

### Events

| Class | Purpose | File | Line |
|-------|---------|------|------|
| `KinetoEvent` | Individual event | profiler_kineto.h | 28 |
| `ProfilerResult` | Profiling results | profiler_kineto.h | 92 |
| `ActivityTraceWrapper` | libkineto trace wrapper | kineto_shim.h | 91 |

### Collection

| Class | Purpose | File |
|-------|---------|------|
| `RecordQueue` | Event collection buffer | collection.h |
| `TraceWrapper` | CPU trace wrapper | kineto_shim.h |

---

## 📋 Enums Reference

### ProfilerState
```cpp
enum class ProfilerState {
    Disabled = 0,
    CPU,                          // CPU-only
    CUDA,                         // CPU + CUDA
    NVTX,                         // NVIDIA markers
    ITT,                          // Intel VTune markers
    PRIVATEUSE1,                  // Custom backend
    KINETO,                       // Full libkineto
    KINETO_GPU_FALLBACK,          // Fallback mode
    KINETO_PRIVATEUSE1_FALLBACK,
    KINETO_ONDEMAND               // On-demand mode
};
```
**Location:** `observer.h:32`

### ActivityType
```cpp
enum class ActivityType {
    CPU = 0,
    XPU,
    CUDA,
    HPU,
    MTIA,
    PrivateUse1,
    NUM_KINETO_ACTIVITIES
};
```
**Location:** `observer.h:14`

### RecordScope
```cpp
enum class RecordScope : uint8_t {
    FUNCTION = 0,
    BACKWARD_FUNCTION,
    TORCHSCRIPT_FUNCTION,
    KERNEL_FUNCTION_DTYPE,
    CUSTOM_CLASS,
    BUILD_FEATURE,
    LITE_INTERPRETER,
    USER_SCOPE,
    STATIC_RUNTIME_OP,
    STATIC_RUNTIME_MODEL,
    NUM_SCOPES
};
```
**Location:** `record_function.h:31`

### ActiveProfilerType
```cpp
enum class ActiveProfilerType {
    NONE = 0,
    LEGACY,
    KINETO,
    NVTX,
    ITT,
    PRIVATEUSE1
};
```
**Location:** `observer.h:47`

---

## 🔧 Callback Functions

| Function | Purpose | File | Line |
|----------|---------|------|------|
| `onFunctionEnter()` | Function entry hook | profiler_kineto.cpp | 534 |
| `onFunctionExit()` | Function exit hook | profiler_kineto.cpp | ~600 |
| `pushProfilingCallbacks()` | Register callbacks | profiler_kineto.cpp | ~700 |

---

## 📁 File Organization

### Main Headers
- `profiler_kineto.h` - Main API
- `observer.h` - Configuration classes
- `record_function.h` - RecordFunction interface
- `kineto_shim.h` - libkineto wrapper
- `api.h` - API aliases

### Implementation Files
- `profiler_kineto.cpp` - Main implementation
- `observer.cpp` - Configuration implementation
- `record_function.cpp` - RecordFunction implementation
- `kineto_shim.cpp` - libkineto wrapper implementation
- `collection.cpp` - Event collection implementation

### Supporting Files
- `events.h` - Event structures
- `containers.h` - Container types
- `collection.h` - Event collection interface
- `kineto_client_interface.h` - Client interface
- `util.h` - Utility functions

**Base Path:** `Library/Core/profiler/pytroch_profiler/`

---

## 🔄 Data Flow

```
Input Configuration
    ↓
enableProfiler(config, activities, scopes)
    ↓
Create KinetoThreadLocalState
    ↓
Register Callbacks
    ↓
Initialize libkineto
    ↓
[Profiling Active]
    ↓
Function Execution
    ↓
onFunctionEnter() → recordQueue.begin_op()
    ↓
onFunctionExit() → recordQueue.end_op()
    ↓
[More Functions...]
    ↓
disableProfiler()
    ↓
Stop libkineto
    ↓
Convert Events to KinetoEvent[]
    ↓
Build Event Tree
    ↓
Create ProfilerResult
    ↓
Return to User
    ↓
result->save("trace.json")
    ↓
Chrome Trace JSON Output
```

---

## 🎓 Learning Path

### Beginner
1. Read KINETO_README.md (Overview section)
2. Review Quick Start example
3. Look at basic profiling pattern

### Intermediate
1. Read KINETO_ARCHITECTURE_SUMMARY.md
2. Study core classes
3. Understand event recording flow
4. Review thread safety model

### Advanced
1. Read KINETO_PROFILER_GUIDE.md (detailed)
2. Study KINETO_IMPLEMENTATION_DETAILS.md
3. Review source code in profiler_kineto.cpp
4. Understand libkineto integration

### Expert
1. Study all implementation details
2. Review callback registration mechanism
3. Understand clock conversion
4. Study memory management
5. Review error handling patterns

---

## 🔍 Common Searches

**Find by topic:**

- **Thread safety** → KINETO_PROFILER_GUIDE.md (Thread Safety section)
- **Memory management** → KINETO_IMPLEMENTATION_DETAILS.md (Memory Management)
- **Error handling** → KINETO_IMPLEMENTATION_DETAILS.md (Error Handling)
- **Performance** → KINETO_IMPLEMENTATION_DETAILS.md (Performance Considerations)
- **Debugging** → KINETO_IMPLEMENTATION_DETAILS.md (Debugging and Diagnostics)
- **Testing** → KINETO_IMPLEMENTATION_DETAILS.md (Testing Considerations)
- **Output format** → KINETO_QUICK_REFERENCE.md (Output Format)
- **Configuration** → KINETO_QUICK_REFERENCE.md (Configuration Classes)

---

## 📊 Architecture Diagrams

The documentation includes several Mermaid diagrams:

1. **Kineto Profiler Architecture Flow** - High-level data flow
2. **Kineto Profiler Class Hierarchy** - Class relationships
3. **Kineto Profiler Execution Sequence** - Sequence diagram
4. **Kineto Data Flow: Input to Output** - Data transformation
5. **Kineto Profiler Complete Component Map** - All components

---

## 🚀 Quick Examples

### Basic Profiling
```cpp
ProfilerConfig config(ProfilerState::KINETO);
std::set<ActivityType> activities{ActivityType::CPU};
enableProfiler(config, activities);
// ... code ...
auto result = disableProfiler();
result->save("trace.json");
```

### With Memory Profiling
```cpp
ProfilerConfig config(ProfilerState::KINETO, false, true);
std::set<ActivityType> activities{ActivityType::CPU};
enableProfiler(config, activities);
// ... code ...
auto result = disableProfiler();
```

### Multi-Thread
```cpp
enableProfiler(config, activities);
if (isProfilerEnabledInMainThread()) {
    enableProfilerInChildThread();
    // ... code ...
    disableProfilerInChildThread();
}
auto result = disableProfiler();
```

---

## 📞 Related Resources

- **XSigma Kineto:** https://github.com/pytorch/kineto
- **Chrome Tracing:** https://www.chromium.org/developers/how-tos/trace-event-profiling-tool
- **Perfetto:** https://ui.perfetto.dev
- **XSigma Profiler API:** `Library/Core/profiler/profiler_api.h`

---

## 📝 Document Versions

| Document | Purpose | Audience |
|----------|---------|----------|
| KINETO_README.md | Overview & quick start | Everyone |
| KINETO_ARCHITECTURE_SUMMARY.md | High-level architecture | Developers |
| KINETO_PROFILER_GUIDE.md | Detailed guide | Developers |
| KINETO_QUICK_REFERENCE.md | API reference | Developers |
| KINETO_IMPLEMENTATION_DETAILS.md | Technical deep dive | Advanced developers |
| KINETO_INDEX.md | Navigation & index | Everyone |

---

## 🔗 Cross-References

**From KINETO_README.md:**
- See KINETO_ARCHITECTURE_SUMMARY.md for architecture details
- See KINETO_PROFILER_GUIDE.md for component details
- See KINETO_QUICK_REFERENCE.md for API reference

**From KINETO_ARCHITECTURE_SUMMARY.md:**
- See KINETO_PROFILER_GUIDE.md for detailed descriptions
- See KINETO_QUICK_REFERENCE.md for API examples
- See KINETO_IMPLEMENTATION_DETAILS.md for internals

**From KINETO_PROFILER_GUIDE.md:**
- See KINETO_QUICK_REFERENCE.md for quick lookup
- See KINETO_IMPLEMENTATION_DETAILS.md for implementation details
- See source code in profiler_kineto.cpp for actual implementation

**From KINETO_QUICK_REFERENCE.md:**
- See KINETO_PROFILER_GUIDE.md for detailed explanations
- See KINETO_IMPLEMENTATION_DETAILS.md for implementation details

**From KINETO_IMPLEMENTATION_DETAILS.md:**
- See KINETO_PROFILER_GUIDE.md for architecture overview
- See KINETO_QUICK_REFERENCE.md for API reference
- See source code for actual implementation

---

## ✅ Checklist for Understanding Kineto

- [ ] Read KINETO_README.md overview
- [ ] Review Quick Start example
- [ ] Understand enableProfiler() and disableProfiler()
- [ ] Know the main classes (KinetoThreadLocalState, ProfilerConfig, KinetoEvent, ProfilerResult)
- [ ] Understand event recording flow
- [ ] Know thread safety model
- [ ] Understand libkineto integration
- [ ] Know output format (Chrome Trace JSON)
- [ ] Can write basic profiling code
- [ ] Understand error handling patterns
- [ ] Know performance considerations
- [ ] Can debug profiler issues

---

**Last Updated:** 2025-11-07  
**Documentation Version:** 1.0  
**Kineto Version:** XSigma Kineto (integrated in XSigma)

