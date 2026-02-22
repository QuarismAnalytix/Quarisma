# Quarisma Graph Architecture: Complete Documentation Index

This directory contains a comprehensive review of Quarisma's graph construction and execution mechanisms. The documentation is organized into five complementary documents, each providing different perspectives and levels of detail.

---

## 📚 Documentation Files

### 1. **PYQUARISMA_GRAPH_SUMMARY.md** ⭐ START HERE
**Best for:** Quick overview and executive summary

Contains:
- High-level overview of the graph system
- Graph structure (nodes, edges, graph tasks)
- Forward pass (graph building)
- Backward pass (graph execution)
- Key mechanisms (gradient accumulation, selective execution)
- Complete examples with multi-path scenarios
- Performance characteristics
- Debugging tools

**Read this first** to understand the big picture.

---

### 2. **PYQUARISMA_GRAPH_ARCHITECTURE.md**
**Best for:** Detailed technical understanding

Contains:
- Comprehensive graph structure explanation
- Node representation and attributes
- Edge semantics and data dependencies
- Graph data structures (GraphTask, InputBuffer)
- Graph building process during forward pass
- Role of autograd in graph construction
- Operation recording mechanisms
- Eager vs graph mode comparison
- Backward pass initialization
- Execution order and scheduling
- Gradient computation details
- Optimization passes

**Read this** for deep technical understanding of each component.

---

### 3. **PYQUARISMA_GRAPH_DETAILED_EXAMPLES.md**
**Best for:** Code-level implementation details

Contains:
- Node constructor implementation
- Edge setup and management
- Adding next edges with topological updates
- Tensor history setting during forward pass
- Input metadata management
- Engine execute method walkthrough
- Dependency computation algorithm
- Ready queue and scheduling implementation
- Basic operation nodes (GraphRoot, Error)
- Python custom function examples

**Read this** to understand the actual C++ and Python code.

---

### 4. **PYQUARISMA_GRAPH_DATA_FLOW.md**
**Best for:** Practical examples and data flow

Contains:
- Complete forward-backward examples
- Simple computation walkthrough
- Multi-path gradient accumulation
- Edge structure and data flow
- Gradient accumulation at edges
- Graph task execution flow (3 phases)
- Ready queue priority explanation
- InputBuffer structure and gradient flow
- Selective execution (.grad() vs .backward())
- ExecInfo structure and filtering
- Anomaly mode and debugging
- Key takeaways

**Read this** to see practical examples and understand data flow.

---

### 5. **PYQUARISMA_GRAPH_QUICK_REFERENCE.md**
**Best for:** Quick lookup and reference

Contains:
- Key components at a glance (table)
- Node structure summary
- Edge structure summary
- Forward pass process flow
- Backward pass execution flow
- Execution priority rules
- Gradient accumulation example
- Selective execution comparison
- GraphTask structure
- ExecInfo structure
- Complete example with execution order
- Multi-path example
- Key insights
- Debugging tips
- Performance considerations
- Source file reference
- Quick lookup table

**Use this** as a quick reference while reading code.

---

## 🎯 Reading Paths

### Path 1: Quick Understanding (30 minutes)
1. Read **PYQUARISMA_GRAPH_SUMMARY.md** (sections 1-7)
2. Skim **PYQUARISMA_GRAPH_QUICK_REFERENCE.md** (sections 1-8)
3. Look at examples in **PYQUARISMA_GRAPH_DATA_FLOW.md** (sections 1-2)

### Path 2: Deep Technical Understanding (2-3 hours)
1. Read **PYQUARISMA_GRAPH_SUMMARY.md** (all sections)
2. Read **PYQUARISMA_GRAPH_ARCHITECTURE.md** (all sections)
3. Study **PYQUARISMA_GRAPH_DETAILED_EXAMPLES.md** (all sections)
4. Reference **PYQUARISMA_GRAPH_QUICK_REFERENCE.md** as needed

### Path 3: Implementation Study (4-5 hours)
1. Start with **PYQUARISMA_GRAPH_SUMMARY.md**
2. Read **PYQUARISMA_GRAPH_ARCHITECTURE.md** carefully
3. Study **PYQUARISMA_GRAPH_DETAILED_EXAMPLES.md** with source code open
4. Work through **PYQUARISMA_GRAPH_DATA_FLOW.md** examples
5. Use **PYQUARISMA_GRAPH_QUICK_REFERENCE.md** for lookups

### Path 4: Practical Examples (1-2 hours)
1. Read **PYQUARISMA_GRAPH_DATA_FLOW.md** (all sections)
2. Reference **PYQUARISMA_GRAPH_QUICK_REFERENCE.md** (sections 11-17)
3. Try examples from **PYQUARISMA_GRAPH_SUMMARY.md** (sections 6-7)

---

## 🔑 Key Concepts

### Graph Structure
- **Nodes:** Represent operations (MulBackward0, AddBackward0, etc.)
- **Edges:** Represent data dependencies (target_node, input_index)
- **GraphTask:** Metadata container for backward execution

### Graph Building (Forward Pass)
1. Operation creates backward node
2. Node stores input metadata
3. Node's next_edges point to input nodes
4. Output tensor's grad_fn set to this node

### Graph Execution (Backward Pass)
1. Create GraphTask and GraphRoot
2. Compute dependencies (count incoming edges)
3. Execute nodes in reverse topological order
4. Accumulate gradients at each node

### Key Mechanisms
- **Implicit Summation:** Multiple edges to same input are summed
- **Selective Execution:** Only execute nodes needed for outputs
- **Gradient Accumulation:** InputBuffer handles multiple incoming gradients
- **Topological Ordering:** Ensures correct gradient flow

---

## 📍 Source Code Locations

| Component | File | Key Lines |
|-----------|------|-----------|
| Node class | `quarisma/csrc/autograd/function.h` | 113-792 |
| Edge struct | `quarisma/csrc/autograd/edge.h` | 1-57 |
| Engine | `quarisma/csrc/autograd/engine.cpp` | 1288-1380 |
| GraphTask | `quarisma/csrc/autograd/graph_task.h` | 17-230 |
| set_history | `quarisma/csrc/autograd/functions/utils.h` | 66-91 |
| GraphRoot | `quarisma/csrc/autograd/functions/basic_ops.h` | 85-113 |
| ReadyQueue | `quarisma/csrc/autograd/engine.h` | 86-125 |
| Python API | `quarisma/autograd/function.py` | 472-566 |
| FX Graph | `quarisma/fx/graph.py` | 1105-1350 |

---

## 💡 Quick Examples

### Simple Forward-Backward
```python
import quarisma

x = quarisma.tensor([2.0], requires_grad=True)
y = x * 3           # Creates MulBackward0
z = y + 2           # Creates AddBackward0
loss = z.sum()      # Creates SumBackward0

loss.backward()     # Executes graph in reverse
print(x.grad)       # tensor([3.])
```

### Multi-Path Gradient Accumulation
```python
x = quarisma.tensor([1.0], requires_grad=True)

y = x * 2
z = x * 3
loss = y + z

loss.backward()
print(x.grad)       # tensor([5.]) - gradients summed!
```

### Selective Execution
```python
x = quarisma.tensor([1.0], requires_grad=True)
y = x * 2
z = y * 3

# .backward(): Execute all nodes, accumulate into x.grad
z.backward()

# .grad(): Execute only nodes on path to x, return gradients
grad_x = quarisma.autograd.grad(z, x)
```

---

## 🔍 Debugging Tips

```python
# Print grad_fn chain
print(loss.grad_fn)

# Enable anomaly detection
quarisma.autograd.set_detect_anomaly(True)

# Inspect next functions
print(loss.grad_fn.next_functions)

# Check requires_grad
print(x.requires_grad)

# Retain graph for multiple backward passes
loss.backward(retain_graph=True)
```

---

## 📊 Performance Characteristics

| Aspect | Complexity |
|--------|-----------|
| Graph Construction | O(n) - n = operations |
| Dependency Computation | O(n + e) - e = edges |
| Backward Execution | O(n) with parallelism |
| Memory | O(n) + saved tensors |

---

## 🎓 Learning Objectives

After reading this documentation, you should understand:

1. ✅ How Quarisma builds computational graphs during forward pass
2. ✅ How nodes and edges represent operations and dependencies
3. ✅ How the backward pass executes the graph
4. ✅ How gradients are accumulated and propagated
5. ✅ How topological ordering ensures correct execution
6. ✅ How selective execution optimizes for specific outputs
7. ✅ How implicit summation handles multiple paths
8. ✅ How the system scales to multi-GPU execution
9. ✅ How to debug autograd issues
10. ✅ Where to find relevant source code

---

## 🔗 Related Resources

- **Official Autograd Documentation:** https://pytorch.org/docs/stable/autograd.html
- **Custom Functions Guide:** https://pytorch.org/docs/stable/autograd.html#extending-quarisma-autograd
- **Profiling Guide:** https://pytorch.org/docs/stable/profiler.html
- **Anomaly Detection:** https://pytorch.org/docs/stable/autograd.html#anomaly-detection
- **Quarisma GitHub:** https://github.com/pytorch/pytorch

---

## 📝 Document Statistics

| Document | Sections | Focus |
|----------|----------|-------|
| PYQUARISMA_GRAPH_SUMMARY.md | 13 | Executive summary |
| PYQUARISMA_GRAPH_ARCHITECTURE.md | 6 | Technical details |
| PYQUARISMA_GRAPH_DETAILED_EXAMPLES.md | 7 | Code examples |
| PYQUARISMA_GRAPH_DATA_FLOW.md | 8 | Practical examples |
| PYQUARISMA_GRAPH_QUICK_REFERENCE.md | 17 | Quick lookup |

---

## ✨ Key Takeaways

1. **Lazy Construction:** Graph built during forward, not executed
2. **Eager Execution:** Backward pass executes immediately
3. **Topological Ordering:** Ensures correct gradient flow
4. **Implicit Summation:** Multiple paths are automatically summed
5. **Selective Execution:** Only execute nodes needed for outputs
6. **Thread Safety:** Shared pointers and mutex protection
7. **Scalability:** Multi-threaded execution with stream synchronization
8. **Flexibility:** Supports dynamic control flow and shapes

---

## 🚀 Next Steps

1. **Start with:** PYQUARISMA_GRAPH_SUMMARY.md
2. **Deep dive:** PYQUARISMA_GRAPH_ARCHITECTURE.md
3. **Study code:** PYQUARISMA_GRAPH_DETAILED_EXAMPLES.md
4. **Practice:** PYQUARISMA_GRAPH_DATA_FLOW.md
5. **Reference:** PYQUARISMA_GRAPH_QUICK_REFERENCE.md

Happy learning! 🎉

