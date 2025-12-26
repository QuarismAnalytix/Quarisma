# PyTorch Autograd Source Inventory: Master Index

**Complete guide to PyTorch's computational graph system source code**

---

## 📚 DOCUMENTATION SUITE

This inventory consists of four comprehensive documents:

### 1. **PYTORCH_AUTOGRAD_SOURCE_INVENTORY.md** (Main Catalog)
   - **Purpose:** Complete catalog of all 80+ source files
   - **Organization:** By functional area (Graph Structure, Building, Execution, Infrastructure)
   - **Format:** Organized tables with file paths, purposes, key classes, and line counts
   - **Best For:** Finding which file implements a specific feature
   - **Read Time:** 30-45 minutes

### 2. **PYTORCH_AUTOGRAD_FILE_DETAILS.md** (Deep Dive)
   - **Purpose:** In-depth information about key files
   - **Content:** Detailed descriptions of core components
   - **Includes:** Algorithms, key methods, usage examples
   - **Best For:** Understanding how specific components work
   - **Read Time:** 45-60 minutes

### 3. **PYTORCH_AUTOGRAD_DEPENDENCY_MAP.md** (Architecture)
   - **Purpose:** Visual representation of file dependencies
   - **Content:** Dependency graphs, include hierarchies, compilation order
   - **Includes:** Circular dependency prevention, impact analysis
   - **Best For:** Understanding system architecture and dependencies
   - **Read Time:** 20-30 minutes

### 4. **PYTORCH_AUTOGRAD_INVENTORY_INDEX.md** (This Document)
   - **Purpose:** Navigation guide and quick reference
   - **Content:** How to use the inventory, quick lookups, common tasks
   - **Best For:** Finding what you need quickly
   - **Read Time:** 10-15 minutes

---

## 🎯 QUICK START GUIDE

### I want to understand...

**How nodes are represented**
→ Read: `PYTORCH_AUTOGRAD_FILE_DETAILS.md` → "torch/csrc/autograd/function.h"
→ File: `torch/csrc/autograd/function.h` (lines 113-792)

**How edges connect nodes**
→ Read: `PYTORCH_AUTOGRAD_FILE_DETAILS.md` → "torch/csrc/autograd/edge.h"
→ File: `torch/csrc/autograd/edge.h` (lines 1-57)

**How the backward pass executes**
→ Read: `PYTORCH_AUTOGRAD_FILE_DETAILS.md` → "torch/csrc/autograd/engine.cpp"
→ File: `torch/csrc/autograd/engine.cpp` (lines 1288-1380)

**How gradients are accumulated**
→ Read: `PYTORCH_AUTOGRAD_FILE_DETAILS.md` → "torch/csrc/autograd/input_buffer.h"
→ File: `torch/csrc/autograd/input_buffer.h/cpp`

**How tensors connect to grad_fn**
→ Read: `PYTORCH_AUTOGRAD_FILE_DETAILS.md` → "torch/csrc/autograd/functions/utils.h"
→ File: `torch/csrc/autograd/functions/utils.h` (lines 66-91)

**How custom functions work**
→ Read: `PYTORCH_AUTOGRAD_FILE_DETAILS.md` → "torch/csrc/autograd/custom_function.h"
→ File: `torch/csrc/autograd/custom_function.h/cpp`

**How to write custom autograd functions**
→ Read: `PYTORCH_AUTOGRAD_FILE_DETAILS.md` → "torch/autograd/function.py"
→ File: `torch/autograd/function.py` (lines 472-566)

**How to inspect the graph**
→ Read: `PYTORCH_AUTOGRAD_FILE_DETAILS.md` → "torch/autograd/graph.py"
→ File: `torch/autograd/graph.py`

**How to debug autograd issues**
→ Read: `PYTORCH_AUTOGRAD_FILE_DETAILS.md` → "torch/csrc/autograd/anomaly_mode.h"
→ File: `torch/csrc/autograd/anomaly_mode.h/cpp`

**How forward AD works**
→ Read: `PYTORCH_AUTOGRAD_FILE_DETAILS.md` → "torch/csrc/autograd/forward_grad.h"
→ File: `torch/csrc/autograd/forward_grad.h/cpp`

---

## 📋 FILE CATEGORIES

### Core Graph Structure (8 files)
```
torch/csrc/autograd/
├── function.h/cpp          # Node class
├── edge.h                  # Edge structure
├── graph_task.h            # Execution metadata
├── input_buffer.h/cpp      # Gradient accumulation
├── input_metadata.h/cpp    # Tensor metadata
├── saved_variable.h/cpp    # Saved tensors
├── variable.h/cpp          # Tensor autograd metadata
└── variable_info.h/cpp     # Variable information
```

### Graph Building (8 files)
```
torch/csrc/autograd/functions/
├── utils.h/cpp             # set_history()
├── basic_ops.h/cpp         # GraphRoot, Error
├── accumulate_grad.h/cpp   # AccumulateGrad
├── tensor.h/cpp            # Tensor operations
└── comm.h/cpp              # Communication ops
```

### Graph Execution (8 files)
```
torch/csrc/autograd/
├── engine.h/cpp            # Backward engine
├── python_engine.h/cpp     # Python engine
├── grad_mode.h             # Gradient mode
├── InferenceMode.h         # Inference mode
├── anomaly_mode.h/cpp      # Anomaly detection
├── python_anomaly_mode.h/cpp
└── (supporting files)
```

### Supporting Infrastructure (40+ files)
```
torch/csrc/autograd/
├── custom_function.h/cpp
├── python_function.h/cpp
├── python_cpp_function.h/cpp
├── forward_grad.h/cpp
├── saved_variable_hooks.h/cpp
├── cpp_hook.h/cpp
├── python_hook.h/cpp
├── profiler*.h/cpp
├── function_hook.h
├── autograd.h/cpp
├── init.cpp
├── python_variable.h/cpp
├── python_variable_indexing.h/cpp
├── utils/
└── (many more)
```

### Python API (8 files)
```
torch/autograd/
├── __init__.py             # Module initialization
├── function.py             # Custom functions
├── graph.py                # Graph inspection
├── grad_mode.py            # Gradient modes
├── anomaly_mode.py         # Anomaly detection
├── gradcheck.py            # Gradient checking
├── forward_ad.py           # Forward AD
└── variable.py             # Variable class
```

---

## 🔍 LOOKUP BY FEATURE

| Feature | Primary File | Secondary Files |
|---------|-------------|-----------------|
| Node structure | `function.h` | `edge.h`, `graph_task.h` |
| Edge structure | `edge.h` | `function.h` |
| Backward execution | `engine.h/cpp` | `graph_task.h`, `input_buffer.h` |
| Gradient accumulation | `input_buffer.h` | `engine.cpp` |
| Leaf gradients | `accumulate_grad.h` | `variable.h` |
| Tensor metadata | `variable.h` | `input_metadata.h` |
| History setting | `functions/utils.h` | `variable.h`, `function.h` |
| Custom functions | `custom_function.h` | `python_function.h` |
| Python API | `torch/autograd/function.py` | `torch/autograd/graph.py` |
| Graph inspection | `torch/autograd/graph.py` | `torch/autograd/__init__.py` |
| Anomaly detection | `anomaly_mode.h` | `python_anomaly_mode.h` |
| Forward AD | `forward_grad.h` | `torch/autograd/forward_ad.py` |
| Profiling | `profiler*.h/cpp` | `torch/autograd/profiler.py` |
| Hooks | `function_hook.h` | `cpp_hook.h`, `python_hook.h` |
| Saved tensors | `saved_variable.h` | `saved_variable_hooks.h` |

---

## 🏗️ ARCHITECTURE LAYERS

```
┌─────────────────────────────────────────────────────────────────┐
│                    PYTHON API LAYER                             │
│  torch/autograd/*.py - User-facing API                          │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                  PYTHON BINDING LAYER                           │
│  python_*.h/cpp - Python wrappers for C++ components            │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│              SUPPORTING INFRASTRUCTURE LAYER                    │
│  custom_function.h, forward_grad.h, profiler.h, etc.            │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                 GRAPH EXECUTION LAYER                           │
│  engine.h/cpp - Backward pass execution                         │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                 GRAPH BUILDING LAYER                            │
│  functions/utils.h - Tensor history and grad_fn connection      │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│              GRAPH STRUCTURE LAYER                              │
│  function.h, edge.h, graph_task.h - Core data structures        │
└─────────────────────────────────────────────────────────────────┘
```

---

## 📊 STATISTICS

| Metric | Count |
|--------|-------|
| Total source files | 80+ |
| C++ header files (.h) | 45+ |
| C++ implementation files (.cpp) | 35+ |
| Python files (.py) | 8 |
| Lines of code (C++) | 50,000+ |
| Lines of code (Python) | 10,000+ |
| Core graph structure files | 8 |
| Graph building files | 8 |
| Graph execution files | 8 |
| Supporting infrastructure files | 40+ |
| Python API files | 8 |

---

## 🔗 CROSS-REFERENCES

### By Functional Area

**Graph Structure:**
- Main: `PYTORCH_AUTOGRAD_SOURCE_INVENTORY.md` → Section 1
- Details: `PYTORCH_AUTOGRAD_FILE_DETAILS.md` → "CORE GRAPH STRUCTURE FILES"
- Dependencies: `PYTORCH_AUTOGRAD_DEPENDENCY_MAP.md` → Section 1

**Graph Building:**
- Main: `PYTORCH_AUTOGRAD_SOURCE_INVENTORY.md` → Section 2
- Details: `PYTORCH_AUTOGRAD_FILE_DETAILS.md` → "GRAPH BUILDING FILES"
- Dependencies: `PYTORCH_AUTOGRAD_DEPENDENCY_MAP.md` → Section 2

**Graph Execution:**
- Main: `PYTORCH_AUTOGRAD_SOURCE_INVENTORY.md` → Section 3
- Details: `PYTORCH_AUTOGRAD_FILE_DETAILS.md` → "GRAPH EXECUTION FILES"
- Dependencies: `PYTORCH_AUTOGRAD_DEPENDENCY_MAP.md` → Section 3

**Supporting Infrastructure:**
- Main: `PYTORCH_AUTOGRAD_SOURCE_INVENTORY.md` → Section 4
- Details: `PYTORCH_AUTOGRAD_FILE_DETAILS.md` → "SUPPORTING INFRASTRUCTURE FILES"
- Dependencies: `PYTORCH_AUTOGRAD_DEPENDENCY_MAP.md` → Section 4

**Python API:**
- Main: `PYTORCH_AUTOGRAD_SOURCE_INVENTORY.md` → Section 5
- Details: `PYTORCH_AUTOGRAD_FILE_DETAILS.md` → "PYTHON API FILES"
- Dependencies: `PYTORCH_AUTOGRAD_DEPENDENCY_MAP.md` → Section 5

---

## 💡 COMMON TASKS

### Task: Add a new operation's backward function
1. Read: `PYTORCH_AUTOGRAD_FILE_DETAILS.md` → "torch/csrc/autograd/functions/tensor.h"
2. File: `torch/csrc/autograd/functions/tensor.h/cpp`
3. Reference: `torch/csrc/autograd/functions/basic_ops.h` for examples

### Task: Implement a custom autograd function
1. Read: `PYTORCH_AUTOGRAD_FILE_DETAILS.md` → "torch/autograd/function.py"
2. File: `torch/autograd/function.py`
3. Reference: Examples in documentation

### Task: Debug a backward pass issue
1. Read: `PYTORCH_AUTOGRAD_FILE_DETAILS.md` → "torch/csrc/autograd/anomaly_mode.h"
2. File: `torch/csrc/autograd/anomaly_mode.h/cpp`
3. Reference: `torch/autograd/anomaly_mode.py` for Python API

### Task: Optimize backward execution
1. Read: `PYTORCH_AUTOGRAD_FILE_DETAILS.md` → "torch/csrc/autograd/engine.cpp"
2. File: `torch/csrc/autograd/engine.h/cpp`
3. Reference: `PYTORCH_AUTOGRAD_DEPENDENCY_MAP.md` for impact analysis

### Task: Add profiling support
1. Read: `PYTORCH_AUTOGRAD_SOURCE_INVENTORY.md` → Section 4.4
2. Files: `torch/csrc/autograd/profiler*.h/cpp`
3. Reference: `torch/autograd/profiler.py` for Python API

### Task: Implement forward AD
1. Read: `PYTORCH_AUTOGRAD_FILE_DETAILS.md` → "torch/csrc/autograd/forward_grad.h"
2. File: `torch/csrc/autograd/forward_grad.h/cpp`
3. Reference: `torch/autograd/forward_ad.py` for Python API

---

## 🚀 GETTING STARTED

### For Beginners
1. Start with: `PYTORCH_AUTOGRAD_SOURCE_INVENTORY.md` (overview)
2. Then read: `PYTORCH_AUTOGRAD_FILE_DETAILS.md` (core files)
3. Finally: `PYTORCH_AUTOGRAD_DEPENDENCY_MAP.md` (architecture)

### For Intermediate Users
1. Start with: `PYTORCH_AUTOGRAD_FILE_DETAILS.md` (specific files)
2. Reference: `PYTORCH_AUTOGRAD_SOURCE_INVENTORY.md` (quick lookup)
3. Check: `PYTORCH_AUTOGRAD_DEPENDENCY_MAP.md` (dependencies)

### For Advanced Users
1. Use: `PYTORCH_AUTOGRAD_DEPENDENCY_MAP.md` (architecture)
2. Reference: `PYTORCH_AUTOGRAD_FILE_DETAILS.md` (implementation details)
3. Consult: `PYTORCH_AUTOGRAD_SOURCE_INVENTORY.md` (complete catalog)

---

## 📞 QUICK REFERENCE

| Question | Answer | Document |
|----------|--------|----------|
| What files are in autograd? | See Section 1-7 | INVENTORY |
| How do nodes work? | See "function.h" | DETAILS |
| How does backward execute? | See "engine.cpp" | DETAILS |
| What depends on what? | See Section 1-7 | DEPENDENCY_MAP |
| How to write custom functions? | See "function.py" | DETAILS |
| How to inspect graphs? | See "graph.py" | DETAILS |
| How to debug autograd? | See "anomaly_mode.h" | DETAILS |
| What's the compilation order? | See Section 11 | DEPENDENCY_MAP |

---

## 📝 NOTES

- All file paths are relative to PyTorch repository root
- Line numbers are approximate and may vary between versions
- Some files are generated during build (in `torch/csrc/autograd/generated/`)
- Python files are in `torch/autograd/` directory
- C++ files are in `torch/csrc/autograd/` directory
- Tools are in `tools/autograd/` directory

---

## ✅ DOCUMENT CHECKLIST

- [x] Complete file catalog (80+ files)
- [x] Organized by functional area
- [x] Detailed file descriptions
- [x] Key classes and functions listed
- [x] Line number references
- [x] Dependency maps
- [x] Architecture diagrams
- [x] Quick reference tables
- [x] Common tasks guide
- [x] Getting started guide


