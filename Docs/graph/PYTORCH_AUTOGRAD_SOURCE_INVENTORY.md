# PyTorch Autograd Source Code Inventory

**Comprehensive catalog of all source files involved in PyTorch's computational graph system**

---

## 1. GRAPH STRUCTURE COMPONENTS

### 1.1 Node Representation

| File Path | Purpose | Key Classes/Functions | Lines | Category |
|-----------|---------|----------------------|-------|----------|
| `torch/csrc/autograd/function.h` | Core Node class definition for autograd graph nodes | `Node`, `NodeGuard`, `SavedVariable` | 113-792 | Core |
| `torch/csrc/autograd/function.cpp` | Node implementation, memory management, node deletion | `deleteNode()`, `gatherFunctions()`, `NodeGuard` | 1-200+ | Core |
| `torch/csrc/autograd/edge.h` | Edge structure connecting nodes in the graph | `Edge` struct with `function` and `input_nr` | 1-57 | Core |
| `torch/csrc/autograd/function_hook.h` | Hook interface for intercepting node execution | `FunctionPreHook`, `FunctionPostHook` | 1-100+ | Core |

### 1.2 Graph Data Structures

| File Path | Purpose | Key Classes/Functions | Lines | Category |
|-----------|---------|----------------------|-------|----------|
| `torch/csrc/autograd/graph_task.h` | GraphTask metadata container for backward execution | `GraphTask`, `ExecInfo`, `GraphTaskGuard` | 17-230 | Core |
| `torch/csrc/autograd/input_buffer.h` | InputBuffer for accumulating gradients | `InputBuffer` class | 1-100+ | Core |
| `torch/csrc/autograd/input_buffer.cpp` | InputBuffer implementation | Gradient accumulation logic | 1-100+ | Core |
| `torch/csrc/autograd/input_metadata.h` | Metadata storage for tensor inputs | `InputMetadata` struct | 1-100+ | Core |
| `torch/csrc/autograd/input_metadata.cpp` | InputMetadata implementation | Shape, dtype, device storage | 1-100+ | Core |

### 1.3 Tensor Autograd Metadata

| File Path | Purpose | Key Classes/Functions | Lines | Category |
|-----------|---------|----------------------|-------|----------|
| `torch/csrc/autograd/variable.h` | Tensor autograd metadata and grad_fn management | `Variable` (alias for Tensor), autograd metadata | 1-200+ | Core |
| `torch/csrc/autograd/variable.cpp` | Variable implementation, grad_fn access | `grad_fn()`, `set_grad_fn()`, `rebase_history()` | 1-600+ | Core |
| `torch/csrc/autograd/variable_info.h` | VariableInfo for storing tensor metadata | `VariableInfo` struct | 1-50 | Core |
| `torch/csrc/autograd/variable_info.cpp` | VariableInfo implementation | Metadata creation and storage | 1-100+ | Core |

---

## 2. GRAPH BUILDING (FORWARD PASS)

### 2.1 Backward Node Creation

| File Path | Purpose | Key Classes/Functions | Lines | Category |
|-----------|---------|----------------------|-------|----------|
| `torch/csrc/autograd/functions/utils.h` | Tensor history setting and grad_fn connection | `set_history()`, `set_history_and_grad_fn()` | 66-91 | Core |
| `torch/csrc/autograd/functions/utils.cpp` | Implementation of history setting | Backward node creation logic | 1-100+ | Core |
| `torch/csrc/autograd/functions/basic_ops.h` | Basic operation nodes (GraphRoot, Error) | `GraphRoot`, `Error`, `DelayedError` | 85-113 | Core |
| `torch/csrc/autograd/functions/basic_ops.cpp` | Implementation of basic operation nodes | Backward implementations | 1-200+ | Core |

### 2.2 Operation-Specific Backward Nodes

| File Path | Purpose | Key Classes/Functions | Lines | Category |
|-----------|---------|----------------------|-------|----------|
| `torch/csrc/autograd/functions/accumulate_grad.h` | AccumulateGrad node for leaf tensors | `AccumulateGrad` class | 1-100+ | Core |
| `torch/csrc/autograd/functions/accumulate_grad.cpp` | AccumulateGrad implementation | Gradient accumulation into parameter.grad | 1-100+ | Core |
| `torch/csrc/autograd/functions/tensor.h` | Tensor operation backward nodes | Various tensor operation nodes | 1-100+ | Core |
| `torch/csrc/autograd/functions/tensor.cpp` | Tensor operation implementations | Backward pass for tensor ops | 1-200+ | Core |

### 2.3 Distributed Operations

| File Path | Purpose | Key Classes/Functions | Lines | Category |
|-----------|---------|----------------------|-------|----------|
| `torch/csrc/autograd/functions/comm.h` | Communication operation nodes | Distributed backward nodes | 1-100+ | Core |
| `torch/csrc/autograd/functions/comm.cpp` | Communication operation implementations | Multi-GPU gradient communication | 1-100+ | Core |

---

## 3. GRAPH EXECUTION (BACKWARD PASS)

### 3.1 Execution Engine

| File Path | Purpose | Key Classes/Functions | Lines | Category |
|-----------|---------|----------------------|-------|----------|
| `torch/csrc/autograd/engine.h` | Backward execution engine and ready queue | `Engine`, `ReadyQueue`, `GraphTask` | 86-125 | Core |
| `torch/csrc/autograd/engine.cpp` | Engine implementation and backward execution | `Engine::execute()`, node scheduling | 1288-1380 | Core |
| `torch/csrc/autograd/python_engine.h` | Python-specific engine wrapper | Python bindings for engine | 1-100+ | Core |
| `torch/csrc/autograd/python_engine.cpp` | Python engine implementation | Python backward execution | 1-200+ | Core |

### 3.2 Execution Control

| File Path | Purpose | Key Classes/Functions | Lines | Category |
|-----------|---------|----------------------|-------|----------|
| `torch/csrc/autograd/grad_mode.h` | Gradient computation mode control | `GradMode`, `no_grad`, `enable_grad` | 1-100+ | Core |
| `torch/csrc/autograd/InferenceMode.h` | Inference mode for disabling autograd | `InferenceMode` context | 1-100+ | Core |

### 3.3 Anomaly Detection

| File Path | Purpose | Key Classes/Functions | Lines | Category |
|-----------|---------|----------------------|-------|----------|
| `torch/csrc/autograd/anomaly_mode.h` | Anomaly detection mode | `AnomalyMode`, `AnomalyMetadata` | 1-100+ | Core |
| `torch/csrc/autograd/anomaly_mode.cpp` | Anomaly detection implementation | Backward traceback and NaN detection | 1-200+ | Core |
| `torch/csrc/autograd/python_anomaly_mode.h` | Python anomaly mode wrapper | Python bindings | 1-50 | Core |
| `torch/csrc/autograd/python_anomaly_mode.cpp` | Python anomaly implementation | Python anomaly detection | 1-100+ | Core |

---

## 4. SUPPORTING INFRASTRUCTURE

### 4.1 Saved Variables and Hooks

| File Path | Purpose | Key Classes/Functions | Lines | Category |
|-----------|---------|----------------------|-------|----------|
| `torch/csrc/autograd/saved_variable.h` | SavedVariable for storing tensors in backward | `SavedVariable` class | 1-100+ | Core |
| `torch/csrc/autograd/saved_variable.cpp` | SavedVariable implementation | Tensor storage and retrieval | 1-200+ | Core |
| `torch/csrc/autograd/saved_variable_hooks.h` | Hooks for saved tensor management | `SavedTensorDefaultHooks` | 1-100+ | Core |
| `torch/csrc/autograd/python_saved_variable_hooks.h` | Python saved variable hooks | Python hook interface | 1-100+ | Core |
| `torch/csrc/autograd/python_saved_variable_hooks.cpp` | Python saved variable implementation | Python tensor hooks | 1-200+ | Core |

### 4.2 Custom Functions

| File Path | Purpose | Key Classes/Functions | Lines | Category |
|-----------|---------|----------------------|-------|----------|
| `torch/csrc/autograd/custom_function.h` | Custom autograd function support | `CustomFunction` base | 1-100+ | Core |
| `torch/csrc/autograd/custom_function.cpp` | Custom function implementation | User-defined backward | 1-200+ | Core |
| `torch/csrc/autograd/python_function.h` | Python custom function wrapper | `PyFunction` class | 1-100+ | Core |
| `torch/csrc/autograd/python_function.cpp` | Python function implementation | Python backward execution | 1-300+ | Core |
| `torch/csrc/autograd/python_cpp_function.h` | C++ function called from Python | `PythonCppFunction` | 1-100+ | Core |
| `torch/csrc/autograd/python_cpp_function.cpp` | C++ function implementation | C++ backward from Python | 1-200+ | Core |

### 4.3 Forward AD (Tangent Computation)

| File Path | Purpose | Key Classes/Functions | Lines | Category |
|-----------|---------|----------------------|-------|----------|
| `torch/csrc/autograd/forward_grad.h` | Forward AD tangent tracking | `ForwardGrad`, `DualTensor` | 1-100+ | Core |
| `torch/csrc/autograd/forward_grad.cpp` | Forward AD implementation | Tangent propagation | 1-200+ | Core |

### 4.4 Profiling and Debugging

| File Path | Purpose | Key Classes/Functions | Lines | Category |
|-----------|---------|----------------------|-------|----------|
| `torch/csrc/autograd/profiler.h` | Profiler interface | Profiling hooks | 1-100+ | Core |
| `torch/csrc/autograd/profiler_legacy.h` | Legacy profiler | Old profiling API | 1-100+ | Core |
| `torch/csrc/autograd/profiler_legacy.cpp` | Legacy profiler implementation | Legacy profiling | 1-200+ | Core |
| `torch/csrc/autograd/profiler_kineto.h` | Kineto profiler integration | Modern profiling | 1-100+ | Core |
| `torch/csrc/autograd/profiler_kineto.cpp` | Kineto profiler implementation | Kineto integration | 1-200+ | Core |
| `torch/csrc/autograd/profiler_python.h` | Python profiler wrapper | Python profiling API | 1-100+ | Core |
| `torch/csrc/autograd/profiler_python.cpp` | Python profiler implementation | Python profiling | 1-200+ | Core |

### 4.5 Hooks and Callbacks

| File Path | Purpose | Key Classes/Functions | Lines | Category |
|-----------|---------|----------------------|-------|----------|
| `torch/csrc/autograd/cpp_hook.h` | C++ hook interface | `CppFunctionPreHook` | 1-100+ | Core |
| `torch/csrc/autograd/cpp_hook.cpp` | C++ hook implementation | Hook execution | 1-100+ | Core |
| `torch/csrc/autograd/python_hook.h` | Python hook wrapper | `PyFunctionTensorPreHook` | 1-100+ | Core |
| `torch/csrc/autograd/python_hook.cpp` | Python hook implementation | Python hook execution | 1-200+ | Core |

### 4.6 Initialization and Bindings

| File Path | Purpose | Key Classes/Functions | Lines | Category |
|-----------|---------|----------------------|-------|----------|
| `torch/csrc/autograd/init.cpp` | Autograd module initialization | Module setup and bindings | 1-200+ | Core |
| `torch/csrc/autograd/python_autograd.h` | Python autograd module header | Module interface | 1-50 | Core |
| `torch/csrc/autograd/functions/init.cpp` | Functions module initialization | Function bindings | 1-100+ | Core |
| `torch/csrc/autograd/functions/pybind.h` | Pybind11 function bindings | Python bindings | 1-100+ | Core |

### 4.7 Utilities

| File Path | Purpose | Key Classes/Functions | Lines | Category |
|-----------|---------|----------------------|-------|----------|
| `torch/csrc/autograd/autograd.h` | Core autograd API | `backward()`, `grad()` | 1-100+ | Core |
| `torch/csrc/autograd/autograd.cpp` | Autograd implementation | Backward execution | 1-200+ | Core |
| `torch/csrc/autograd/autograd_meta.cpp` | Autograd metadata management | Metadata initialization | 1-100+ | Core |
| `torch/csrc/autograd/autograd_not_implemented_fallback.h` | Fallback for unimplemented ops | Error handling | 1-50 | Core |
| `torch/csrc/autograd/autograd_not_implemented_fallback.cpp` | Fallback implementation | Unimplemented op handling | 1-100+ | Core |
| `torch/csrc/autograd/jit_decomp_interface.h` | JIT decomposition interface | JIT integration | 1-50 | Core |
| `torch/csrc/autograd/jit_decomp_interface.cpp` | JIT decomposition implementation | JIT autograd | 1-100+ | Core |
| `torch/csrc/autograd/record_function_ops.h` | Record function operations | Profiling operations | 1-50 | Core |
| `torch/csrc/autograd/record_function_ops.cpp` | Record function implementation | Operation recording | 1-100+ | Core |

### 4.8 Python Variable Wrapper

| File Path | Purpose | Key Classes/Functions | Lines | Category |
|-----------|---------|----------------------|-------|----------|
| `torch/csrc/autograd/python_variable.h` | Python Tensor wrapper | `THPVariable` struct | 1-100+ | Core |
| `torch/csrc/autograd/python_variable.cpp` | Python variable implementation | Python tensor methods | 1-300+ | Core |
| `torch/csrc/autograd/python_variable_indexing.h` | Python indexing support | Indexing operations | 1-100+ | Core |
| `torch/csrc/autograd/python_variable_indexing.cpp` | Indexing implementation | Python indexing | 1-200+ | Core |
| `torch/csrc/autograd/python_legacy_variable.h` | Legacy variable support | Backward compatibility | 1-50 | Core |
| `torch/csrc/autograd/python_legacy_variable.cpp` | Legacy variable implementation | Legacy support | 1-100+ | Core |

### 4.9 Manual Functions

| File Path | Purpose | Key Classes/Functions | Lines | Category |
|-----------|---------|----------------------|-------|----------|
| `torch/csrc/autograd/FunctionsManual.h` | Manually defined backward functions | Custom backward implementations | 1-100+ | Core |
| `torch/csrc/autograd/FunctionsManual.cpp` | Manual function implementations | Special case backward | 1-200+ | Core |
| `torch/csrc/autograd/VariableTypeManual.cpp` | Manual variable type methods | Special tensor methods | 1-200+ | Core |
| `torch/csrc/autograd/TraceTypeManual.cpp` | Manual trace type methods | Tracing support | 1-100+ | Core |

### 4.10 Utilities and Helpers

| File Path | Purpose | Key Classes/Functions | Lines | Category |
|-----------|---------|----------------------|-------|----------|
| `torch/csrc/autograd/utils/error_messages.h` | Error message utilities | Error formatting | 1-50 | Utility |
| `torch/csrc/autograd/utils/grad_layout_contract.h` | Gradient layout contracts | Layout validation | 1-50 | Utility |
| `torch/csrc/autograd/utils/lambda_post_hook.h` | Lambda post-hook utilities | Hook helpers | 1-50 | Utility |
| `torch/csrc/autograd/utils/python_arg_parsing.h` | Python argument parsing | Argument parsing | 1-100+ | Utility |
| `torch/csrc/autograd/utils/warnings.h` | Warning utilities | Warning management | 1-50 | Utility |
| `torch/csrc/autograd/utils/warnings.cpp` | Warning implementation | Warning output | 1-100+ | Utility |
| `torch/csrc/autograd/utils/wrap_outputs.h` | Output wrapping utilities | Result wrapping | 1-100+ | Utility |
| `torch/csrc/autograd/VariableTypeUtils.h` | Variable type utilities | Type utilities | 1-100+ | Utility |

---

## 5. PYTHON API (torch/autograd/)

### 5.1 Core Python API

| File Path | Purpose | Key Classes/Functions | Lines | Category |
|-----------|---------|----------------------|-------|----------|
| `torch/autograd/__init__.py` | Autograd module initialization | Module exports, API | 1-600+ | Python API |
| `torch/autograd/function.py` | Custom autograd function API | `Function`, `FunctionCtx`, `once_differentiable` | 472-566 | Python API |
| `torch/autograd/variable.py` | Variable class (legacy) | `Variable`, `VariableMeta` | 1-50 | Python API |
| `torch/autograd/graph.py` | Graph inspection API | `Node`, `GradientEdge`, hooks | 1-600+ | Python API |

### 5.2 Gradient Computation

| File Path | Purpose | Key Classes/Functions | Lines | Category |
|-----------|---------|----------------------|-------|----------|
| `torch/autograd/grad_mode.py` | Gradient mode control | `no_grad`, `enable_grad`, `inference_mode` | 1-400+ | Python API |
| `torch/autograd/forward_ad.py` | Forward AD API | `make_dual`, `unpack_dual`, `dual_level` | 1-300+ | Python API |
| `torch/autograd/gradcheck.py` | Gradient checking utilities | `gradcheck`, `gradgradcheck` | 1-2200+ | Python API |

### 5.3 Debugging and Profiling

| File Path | Purpose | Key Classes/Functions | Lines | Category |
|-----------|---------|----------------------|-------|----------|
| `torch/autograd/anomaly_mode.py` | Anomaly detection API | `detect_anomaly`, `set_detect_anomaly` | 1-200+ | Python API |
| `torch/autograd/profiler.py` | Profiler API | `profile`, `record_function` | 1-400+ | Python API |
| `torch/autograd/profiler_util.py` | Profiler utilities | `FunctionEvent`, `EventList` | 1-300+ | Python API |

### 5.4 Functional API

| File Path | Purpose | Key Classes/Functions | Lines | Category |
|-----------|---------|----------------------|-------|----------|
| `torch/autograd/functional.py` | Functional autograd operations | `jacobian`, `hessian`, `vjp`, `jvp` | 1-400+ | Python API |

---

## 6. GENERATED CODE

### 6.1 Code Generation Tools

| File Path | Purpose | Key Classes/Functions | Lines | Category |
|-----------|---------|----------------------|-------|----------|
| `tools/autograd/gen_autograd.py` | Main autograd code generator | Orchestrates code generation | 1-100+ | Generator |
| `tools/autograd/gen_autograd_functions.py` | Generates backward function nodes | Creates Node subclasses | 1-500+ | Generator |
| `tools/autograd/gen_variable_type.py` | Generates VariableType methods | Creates tensor methods | 1-500+ | Generator |
| `tools/autograd/gen_python_functions.py` | Generates Python bindings | Creates Python API | 1-500+ | Generator |

### 6.2 Generated Output

| File Path | Purpose | Key Classes/Functions | Lines | Category |
|-----------|---------|----------------------|-------|----------|
| `torch/csrc/autograd/generated/` | Generated backward functions | Auto-generated Node classes | Variable | Generated |

---

## 7. INTEGRATION POINTS

### 7.1 Compiled Autograd

| File Path | Purpose | Key Classes/Functions | Lines | Category |
|-----------|---------|----------------------|-------|----------|
| `torch/_dynamo/compiled_autograd.py` | Compiled autograd system | `AutogradCompilerInstance` | 1-500+ | Integration |
| `torch/csrc/dynamo/compiled_autograd.h` | C++ compiled autograd | Compiled backward execution | 1-100+ | Integration |

### 7.2 Distributed Autograd

| File Path | Purpose | Key Classes/Functions | Lines | Category |
|-----------|---------|----------------------|-------|----------|
| `torch/csrc/distributed/autograd/` | Distributed autograd system | RPC-based backward | Variable | Integration |

---

## 8. SUMMARY STATISTICS

| Category | File Count | Purpose |
|----------|-----------|---------|
| **Graph Structure** | 8 | Node, Edge, GraphTask definitions |
| **Graph Building** | 8 | Backward node creation, history setting |
| **Graph Execution** | 8 | Engine, scheduling, execution control |
| **Supporting Infrastructure** | 40+ | Hooks, saved variables, profiling |
| **Python API** | 8 | User-facing autograd API |
| **Code Generation** | 4 | Autograd code generators |
| **Integration** | 2+ | Compiled autograd, distributed |
| **TOTAL** | **80+** | Complete autograd system |

---

## 9. KEY DEPENDENCIES

```
function.h (Node)
    ├── edge.h (Edge)
    ├── graph_task.h (GraphTask)
    ├── input_buffer.h (InputBuffer)
    ├── input_metadata.h (InputMetadata)
    └── saved_variable.h (SavedVariable)

engine.h/cpp (Engine)
    ├── function.h (Node)
    ├── graph_task.h (GraphTask)
    ├── input_buffer.h (InputBuffer)
    └── functions/basic_ops.h (GraphRoot)

variable.h (Tensor metadata)
    ├── edge.h (grad_fn edge)
    ├── function.h (grad_fn node)
    └── function_hook.h (hooks)

functions/utils.h (set_history)
    ├── function.h (Node)
    ├── edge.h (Edge)
    └── variable.h (Tensor)
```

---

## 10. QUICK REFERENCE BY FUNCTIONAL AREA

**Need to understand node structure?** → `function.h`, `edge.h`
**Need to understand graph building?** → `functions/utils.h`, `variable.h`
**Need to understand backward execution?** → `engine.h/cpp`, `graph_task.h`
**Need to understand gradient accumulation?** → `input_buffer.h`, `functions/accumulate_grad.h`
**Need to understand custom functions?** → `custom_function.h`, `python_function.h`
**Need to understand profiling?** → `profiler*.h/cpp`
**Need to understand anomaly detection?** → `anomaly_mode.h/cpp`
**Need Python API?** → `torch/autograd/*.py`


