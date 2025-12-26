# PyTorch Autograd: File Dependency Map

**Visual representation of dependencies between autograd source files**

---

## 1. CORE DEPENDENCY HIERARCHY

```
┌─────────────────────────────────────────────────────────────────┐
│                    GRAPH STRUCTURE LAYER                        │
└─────────────────────────────────────────────────────────────────┘

edge.h (Edge structure)
    ↓
function.h (Node class)
    ├── Depends on: edge.h, saved_variable.h, input_metadata.h
    ├── Used by: engine.h, graph_task.h, variable.h
    └── Key: Defines the fundamental Node class

input_metadata.h (Tensor metadata)
    ├── Depends on: ATen/Tensor.h
    └── Used by: function.h, input_buffer.h

saved_variable.h (Saved tensor wrapper)
    ├── Depends on: ATen/Tensor.h
    └── Used by: function.h, custom_function.h

input_buffer.h (Gradient accumulation)
    ├── Depends on: input_metadata.h, ATen/Tensor.h
    └── Used by: engine.h, graph_task.h

graph_task.h (Execution metadata)
    ├── Depends on: function.h, input_buffer.h, edge.h
    └── Used by: engine.h, python_engine.h

variable.h (Tensor autograd metadata)
    ├── Depends on: edge.h, function.h, function_hook.h
    └── Used by: engine.h, functions/utils.h, python_variable.h
```

---

## 2. GRAPH BUILDING LAYER

```
┌─────────────────────────────────────────────────────────────────┐
│                  GRAPH BUILDING (Forward Pass)                  │
└─────────────────────────────────────────────────────────────────┘

functions/utils.h (set_history)
    ├── Depends on: function.h, edge.h, variable.h
    └── Used by: All operation backward nodes

functions/basic_ops.h (GraphRoot, Error)
    ├── Depends on: function.h, edge.h
    └── Used by: engine.h, python_function.h

functions/accumulate_grad.h (AccumulateGrad)
    ├── Depends on: function.h, variable.h
    └── Used by: engine.h, graph building

functions/tensor.h (Tensor operation nodes)
    ├── Depends on: function.h, saved_variable.h
    └── Used by: Generated backward functions

functions/comm.h (Communication nodes)
    ├── Depends on: function.h
    └── Used by: Distributed operations
```

---

## 3. GRAPH EXECUTION LAYER

```
┌─────────────────────────────────────────────────────────────────┐
│                 GRAPH EXECUTION (Backward Pass)                 │
└─────────────────────────────────────────────────────────────────┘

engine.h (Engine & ReadyQueue)
    ├── Depends on: function.h, graph_task.h, input_buffer.h
    │              functions/basic_ops.h, anomaly_mode.h
    ├── Used by: engine.cpp, python_engine.h, autograd.h
    └── Key: Defines backward execution algorithm

engine.cpp (Engine implementation)
    ├── Depends on: engine.h, function.h, variable.h
    │              anomaly_mode.h, grad_mode.h
    └── Key: Implements backward execution

python_engine.h/cpp (Python engine wrapper)
    ├── Depends on: engine.h, python_function.h
    └── Used by: Python autograd API

grad_mode.h (Gradient mode control)
    ├── Depends on: ATen/core/TensorBase.h
    └── Used by: engine.cpp, variable.h

anomaly_mode.h/cpp (Anomaly detection)
    ├── Depends on: function.h
    └── Used by: engine.cpp, python_anomaly_mode.h
```

---

## 4. SUPPORTING INFRASTRUCTURE LAYER

```
┌─────────────────────────────────────────────────────────────────┐
│              SUPPORTING INFRASTRUCTURE                          │
└─────────────────────────────────────────────────────────────────┘

custom_function.h/cpp (Custom autograd functions)
    ├── Depends on: function.h, saved_variable.h
    └── Used by: python_cpp_function.h, python_function.h

python_function.h/cpp (Python custom functions)
    ├── Depends on: custom_function.h, python_hook.h
    │              saved_variable.h, graph_task.h
    └── Used by: Python Function API

python_cpp_function.h/cpp (C++ functions from Python)
    ├── Depends on: custom_function.h, python_function.h
    └── Used by: Python Function.apply()

forward_grad.h/cpp (Forward AD)
    ├── Depends on: function.h, variable.h
    └── Used by: Forward mode differentiation

saved_variable_hooks.h/cpp (Saved tensor hooks)
    ├── Depends on: saved_variable.h
    └── Used by: Memory optimization

cpp_hook.h/cpp (C++ hooks)
    ├── Depends on: function_hook.h
    └── Used by: Hook execution

python_hook.h/cpp (Python hooks)
    ├── Depends on: function_hook.h, python_headers.h
    └── Used by: Python hook execution

profiler*.h/cpp (Profiling)
    ├── Depends on: function.h, engine.h
    └── Used by: Performance profiling
```

---

## 5. PYTHON BINDING LAYER

```
┌─────────────────────────────────────────────────────────────────┐
│                    PYTHON BINDING LAYER                         │
└─────────────────────────────────────────────────────────────────┘

python_variable.h/cpp (Python Tensor wrapper)
    ├── Depends on: variable.h, python_hook.h
    └── Used by: Python tensor operations

python_variable_indexing.h/cpp (Python indexing)
    ├── Depends on: python_variable.h, variable.h
    └── Used by: Python tensor indexing

init.cpp (Module initialization)
    ├── Depends on: All autograd headers
    └── Used by: PyTorch initialization

python_autograd.h (Python autograd module)
    ├── Depends on: python_engine.h, python_function.h
    └── Used by: Python autograd module

functions/init.cpp (Functions module init)
    ├── Depends on: All function headers
    └── Used by: PyTorch initialization
```

---

## 6. PYTHON API LAYER

```
┌─────────────────────────────────────────────────────────────────┐
│                      PYTHON API LAYER                           │
└─────────────────────────────────────────────────────────────────┘

torch/autograd/__init__.py
    ├── Imports: function.py, grad_mode.py, graph.py
    │           anomaly_mode.py, gradcheck.py, forward_ad.py
    └── Exports: Public autograd API

torch/autograd/function.py (Custom functions)
    ├── Depends on: torch._C._functions (C++ backend)
    └── Used by: User custom autograd functions

torch/autograd/graph.py (Graph inspection)
    ├── Depends on: torch._C._autograd (C++ backend)
    └── Used by: Graph inspection and hooks

torch/autograd/grad_mode.py (Gradient modes)
    ├── Depends on: torch._C (C++ backend)
    └── Used by: Controlling gradient computation

torch/autograd/anomaly_mode.py (Anomaly detection)
    ├── Depends on: torch._C (C++ backend)
    └── Used by: Debugging autograd

torch/autograd/gradcheck.py (Gradient checking)
    ├── Depends on: torch.autograd.grad
    └── Used by: Testing gradients

torch/autograd/forward_ad.py (Forward AD)
    ├── Depends on: torch._C (C++ backend)
    └── Used by: Forward mode differentiation
```

---

## 7. COMPLETE DEPENDENCY GRAPH

```
┌──────────────────────────────────────────────────────────────────┐
│                    COMPLETE DEPENDENCY GRAPH                     │
└──────────────────────────────────────────────────────────────────┘

                        ATen/Tensor.h
                              ↓
        ┌─────────────────────┼─────────────────────┐
        ↓                     ↓                     ↓
    edge.h          input_metadata.h        saved_variable.h
        ↓                     ↓                     ↓
        └─────────────────────┼─────────────────────┘
                              ↓
                        function.h
                              ↓
        ┌─────────────────────┼─────────────────────┐
        ↓                     ↓                     ↓
    variable.h         graph_task.h          input_buffer.h
        ↓                     ↓                     ↓
        └─────────────────────┼─────────────────────┘
                              ↓
                        engine.h/cpp
                              ↓
        ┌─────────────────────┼─────────────────────┐
        ↓                     ↓                     ↓
  python_engine.h    custom_function.h    anomaly_mode.h
        ↓                     ↓                     ↓
        └─────────────────────┼─────────────────────┘
                              ↓
                    python_function.h
                              ↓
                        init.cpp
                              ↓
                    Python API Layer
```

---

## 8. DEPENDENCY BY FUNCTIONAL AREA

### Graph Structure Dependencies
```
edge.h
    ↓
function.h ← input_metadata.h, saved_variable.h
    ↓
graph_task.h ← input_buffer.h
    ↓
variable.h ← function_hook.h
```

### Graph Building Dependencies
```
variable.h
    ↓
functions/utils.h (set_history)
    ↓
functions/accumulate_grad.h
functions/basic_ops.h
functions/tensor.h
```

### Graph Execution Dependencies
```
function.h
    ↓
graph_task.h
    ↓
engine.h ← anomaly_mode.h, grad_mode.h
    ↓
engine.cpp
    ↓
python_engine.h
```

### Custom Functions Dependencies
```
function.h
    ↓
custom_function.h ← saved_variable.h
    ↓
python_cpp_function.h
    ↓
python_function.h ← python_hook.h
```

---

## 9. CIRCULAR DEPENDENCY PREVENTION

**Key Design Patterns:**

1. **Forward Declarations**
   - `function.h` forward declares types used in `edge.h`
   - Prevents circular includes

2. **Separate Headers**
   - `edge.h` is separate from `function.h`
   - Allows `edge.h` to be included without full `function.h`

3. **Implementation Files**
   - Circular dependencies resolved in `.cpp` files
   - Headers remain acyclic

4. **Namespace Isolation**
   - All autograd code in `torch::autograd` namespace
   - Prevents naming conflicts

---

## 10. INCLUDE DEPENDENCY SUMMARY

| File | Includes | Included By |
|------|----------|------------|
| `edge.h` | ATen | function.h, graph_task.h, variable.h |
| `function.h` | edge.h, saved_variable.h | engine.h, variable.h, all ops |
| `graph_task.h` | function.h, input_buffer.h | engine.h, python_engine.h |
| `variable.h` | edge.h, function.h | engine.h, functions/utils.h |
| `engine.h` | function.h, graph_task.h | engine.cpp, python_engine.h |
| `functions/utils.h` | function.h, variable.h | All backward nodes |
| `custom_function.h` | function.h | python_function.h |
| `python_function.h` | custom_function.h | init.cpp |

---

## 11. COMPILATION ORDER

**Recommended compilation order (respecting dependencies):**

1. `edge.h` (no dependencies)
2. `input_metadata.h` (no dependencies)
3. `saved_variable.h` (no dependencies)
4. `function.h` (depends on 1-3)
5. `input_buffer.h` (depends on 2)
6. `graph_task.h` (depends on 4-5)
7. `variable.h` (depends on 1, 4)
8. `engine.h` (depends on 4-6)
9. `functions/utils.h` (depends on 4, 7)
10. `functions/accumulate_grad.h` (depends on 4, 7)
11. `functions/basic_ops.h` (depends on 4)
12. `custom_function.h` (depends on 4)
13. `python_function.h` (depends on 12)
14. `engine.cpp` (depends on 8)
15. `python_engine.h/cpp` (depends on 8, 13)
16. `init.cpp` (depends on all)

---

## 12. QUICK REFERENCE: WHAT DEPENDS ON WHAT

**If you modify `function.h`, you must recompile:**
- `edge.h` users
- `graph_task.h` users
- `variable.h` users
- `engine.h` users
- All operation backward nodes
- All Python bindings

**If you modify `engine.h`, you must recompile:**
- `engine.cpp`
- `python_engine.h/cpp`
- All code calling backward

**If you modify `variable.h`, you must recompile:**
- `functions/utils.h` users
- All operation backward nodes
- All Python variable code

**If you modify `functions/utils.h`, you must recompile:**
- All operation backward nodes
- All code using `set_history()`


