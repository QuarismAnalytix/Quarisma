# Quarisma Graph Architecture: Quick Reference Guide

## 1. KEY COMPONENTS AT A GLANCE

| Component | Location | Purpose |
|-----------|----------|---------|
| **Node** | `quarisma/csrc/autograd/function.h` | Represents an operation in the graph |
| **Edge** | `quarisma/csrc/autograd/edge.h` | Represents data dependency between nodes |
| **GraphTask** | `quarisma/csrc/autograd/graph_task.h` | Metadata for a single backward execution |
| **Engine** | `quarisma/csrc/autograd/engine.h/cpp` | Executes the backward graph |
| **ReadyQueue** | `quarisma/csrc/autograd/engine.h` | Priority queue for node execution |
| **InputBuffer** | `quarisma/csrc/autograd/input_buffer.h` | Accumulates gradients for node inputs |

---

## 2. NODE STRUCTURE

```cpp
struct Node {
  uint64_t sequence_nr_;              // Execution priority (higher = earlier)
  uint64_t topological_nr_;           // Ensures parent > children
  edge_list next_edges_;              // Outgoing edges to next nodes
  std::vector<InputMetadata> input_metadata_;  // Shape, dtype, device info
  uint64_t thread_id_;                // Thread where node was created
};
```

**Node Types:**
- `AccumulateGrad`: Leaf node, accumulates gradients into parameter.grad
- `GraphRoot`: Entry point for backward pass
- Operation nodes: `AddBackward0`, `MulBackward0`, etc.

---

## 3. EDGE STRUCTURE

```cpp
struct Edge {
  std::shared_ptr<Node> function;  // Target node
  uint32_t input_nr;               // Which input of target (0, 1, 2, ...)
};
```

**Key Property:** Multiple edges to same input are **implicitly summed**

---

## 4. FORWARD PASS: GRAPH CONSTRUCTION

```
Operation Execution
    ↓
Create Backward Node (e.g., MulBackward0)
    ↓
Add Input Metadata (shape, dtype, device)
    ↓
Set next_edges to input nodes' grad_fns
    ↓
Set output tensor's grad_fn to this node
    ↓
Set output tensor's output_nr
```

**Key Function:** `set_history()` in `quarisma/csrc/autograd/functions/utils.h`

---

## 5. BACKWARD PASS: EXECUTION FLOW

```
loss.backward()
    ↓
Engine::execute()
    ├─ Create GraphTask
    ├─ Create GraphRoot
    ├─ Compute dependencies
    └─ Initialize execution info
    ↓
compute_dependencies()
    ├─ Traverse graph from root
    ├─ Count incoming edges per node
    └─ Store in dependencies_ map
    ↓
execute_graph_task()
    ├─ Queue root node
    ├─ While ready_queue not empty:
    │  ├─ Pop highest priority node
    │  ├─ Execute node.apply(gradients)
    │  ├─ For each output edge:
    │  │  ├─ Add gradient to target's InputBuffer
    │  │  ├─ Decrement target's dependency count
    │  │  └─ If count == 0, queue target
    │  └─ Continue
    └─ Return gradients
```

---

## 6. EXECUTION PRIORITY

**ReadyQueue Comparator:**
```cpp
// 1. Shutdown tasks first
// 2. By reentrant depth (lower = higher priority)
// 3. By sequence number (higher = higher priority)
```

**Result:** Nodes execute in **reverse topological order**

---

## 7. GRADIENT ACCUMULATION

```python
# Example: x appears twice
y = x + x

# During backward:
# AddBackward0 has two inputs (both x)
# Input 0: receives gradient [1.0, 1.0]
# Input 1: receives gradient [1.0, 1.0]
# InputBuffer accumulates: [1.0, 1.0] + [1.0, 1.0] = [2.0, 2.0]
# Result: x.grad = [2.0, 2.0]
```

---

## 8. SELECTIVE EXECUTION

### .backward() - Full Accumulation
```python
loss.backward()
# Executes all nodes in graph
# Accumulates into parameter.grad
```

### .grad() - Selective Capture
```python
grad_x = quarisma.autograd.grad(loss, x)
# Only executes nodes on path to x
# Returns gradients without accumulating
```

**Implementation:**
- `.backward()`: `accumulate_grad=True`, uses `AccumulateGrad` nodes
- `.grad()`: `accumulate_grad=False`, captures in `captured_vars_`

---

## 9. GRAPH TASK STRUCTURE

```cpp
struct GraphTask {
  std::unordered_map<Node*, InputBuffer> not_ready_;
  std::unordered_map<Node*, int> dependencies_;
  std::unordered_set<Node*> nodes_in_graph_;
  quarisma::small_vector<Node*, 4> graph_roots_;
  std::unordered_map<Node*, ExecInfo> exec_info_;
  std::vector<at::Tensor> captured_vars_;
};
```

---

## 10. EXECUTION INFO

```cpp
struct ExecInfo {
  bool needed_ = false;  // Should this node execute?
  std::unique_ptr<std::vector<Capture>> captures_;  // For .grad()

  struct Capture {
    int input_idx_;   // Which input of this node
    int output_idx_;  // Where to store in captured_vars_
  };
};
```

---

## 11. COMPLETE EXAMPLE

```python
import quarisma

# Forward pass: builds graph
x = quarisma.tensor([2.0], requires_grad=True)
y = x * 3           # Creates MulBackward0
z = y + 2           # Creates AddBackward0
loss = z.sum()      # Creates SumBackward0

# Graph structure:
# x ──[MulBackward0]──> y ──[AddBackward0]──> z ──[SumBackward0]──> loss

# Backward pass: executes graph
loss.backward()

# Execution order (reverse topological):
# 1. SumBackward0.apply([1.0]) → [1.0]
# 2. AddBackward0.apply([1.0]) → [1.0, 1.0]
# 3. MulBackward0.apply([1.0]) → [3.0]
# 4. AccumulateGrad(x).apply([3.0]) → x.grad = [3.0]

print(x.grad)  # tensor([3.])
```

---

## 12. MULTI-PATH EXAMPLE

```python
import quarisma

x = quarisma.tensor([1.0], requires_grad=True)

# Multiple paths to x
y = x * 2
z = x * 3
loss = y + z

# Graph:
#        ┌─[MulBackward0]─> y ─┐
# x ─────┤                      [AddBackward0]─> loss
#        └─[MulBackward0]─> z ─┘

# Backward execution:
# 1. AddBackward0: produces [1.0, 1.0]
# 2. MulBackward0(y): receives 1.0, produces 2.0
# 3. MulBackward0(z): receives 1.0, produces 3.0
# 4. AccumulateGrad(x): receives 2.0 + 3.0 = 5.0

loss.backward()
print(x.grad)  # tensor([5.])
```

---

## 13. KEY INSIGHTS

1. **Lazy Graph Construction:** Built during forward, executed during backward
2. **Topological Ordering:** Ensures correct gradient flow
3. **Sequence Numbers:** Enable reverse execution order
4. **Implicit Summation:** Multiple edges to same input are summed
5. **Metadata Storage:** Enables gradient shape inference
6. **Thread Safety:** Shared pointers and mutex protection
7. **Selective Execution:** Only execute nodes needed for outputs
8. **Gradient Accumulation:** InputBuffer handles multiple incoming gradients

---

## 14. DEBUGGING TIPS

```python
# Print graph structure
print(loss.grad_fn)  # Shows grad_fn chain

# Enable anomaly detection
quarisma.autograd.set_detect_anomaly(True)
loss.backward()  # Prints stack trace if NaN/Inf

# Inspect node information
print(loss.grad_fn.next_functions)  # Shows next nodes

# Check requires_grad
print(x.requires_grad)  # True if in graph

# Retain graph for multiple backward passes
loss.backward(retain_graph=True)
```

---

## 15. PERFORMANCE CONSIDERATIONS

1. **Sequence Numbers:** Higher = executed first (reverse topo order)
2. **Topological Numbers:** Ensures parent > all children
3. **Dependency Pruning:** Only execute nodes needed for outputs
4. **Stream Synchronization:** Handles multi-GPU execution
5. **Inplace Detection:** Tracks inplace operations for correctness

---

## 16. SOURCE FILE REFERENCE

```
quarisma/csrc/autograd/
├── function.h/cpp          # Node class
├── edge.h                  # Edge structure
├── engine.h/cpp            # Backward execution
├── graph_task.h            # Execution metadata
├── input_buffer.h/cpp      # Gradient accumulation
├── functions/
│   ├── basic_ops.h/cpp     # GraphRoot, Error, etc.
│   └── utils.h             # set_history()
└── variable.h              # Tensor autograd metadata
```

---

## 17. QUICK LOOKUP TABLE

| Question | Answer | File |
|----------|--------|------|
| How are nodes created? | `Node()` constructor | `function.h:115-140` |
| How are edges created? | `add_next_edge()` | `function.h:303-305` |
| How is history set? | `set_history()` | `functions/utils.h:66-91` |
| How is backward executed? | `Engine::execute()` | `engine.cpp:1288-1352` |
| How are gradients accumulated? | `InputBuffer::add()` | `input_buffer.h` |
| How is execution ordered? | `ReadyQueue` comparator | `engine.h:90-100` |
| How are dependencies computed? | `compute_dependencies()` | `engine.cpp` |
| How is selective execution done? | `ExecInfo` filtering | `graph_task.h:47-130` |
