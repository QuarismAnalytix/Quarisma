# PyTorch Graph Construction and Execution Mechanisms: Comprehensive Review

## Overview

PyTorch's autograd system builds a **dynamic computational graph** during the forward pass and executes it in reverse during the backward pass to compute gradients. This document provides a detailed analysis of the graph structure, construction process, and execution mechanisms.

---

## 1. GRAPH STRUCTURE

### 1.1 Nodes Representation

**Location:** `torch/csrc/autograd/function.h` (lines 113-792)

Nodes are represented by the `Node` class, which is the fundamental unit of computation in the autograd graph:

```cpp
struct TORCH_API Node : std::enable_shared_from_this<Node> {
  // Unique sequence number (thread-local, monotonically increasing)
  uint64_t sequence_nr_;
  
  // Outgoing edges to next nodes in the graph
  edge_list next_edges_;
  
  // Input metadata (shape, dtype, device, etc.)
  std::vector<InputMetadata> input_metadata_;
  
  // Topological number for execution ordering
  uint64_t topological_nr_;
  
  // Thread ID where this node was created
  uint64_t thread_id_;
};
```

**Key Node Characteristics:**
- **Sequence Number:** Monotonically increasing ID assigned at node creation, used for execution priority
- **Topological Number:** Ensures parent nodes have higher topo_nr than children
- **Input Metadata:** Stores tensor shape, dtype, device, and stream information
- **Thread Safety:** Each node is shared via `std::shared_ptr<Node>`

**Node Types:**
- `AccumulateGrad`: Leaf node that accumulates gradients into parameter.grad
- `GraphRoot`: Special node representing the backward pass entry point
- Operation-specific nodes (e.g., `AddBackward0`, `MulBackward0`)

### 1.2 Edges Representation

**Location:** `torch/csrc/autograd/edge.h` (lines 1-57)

Edges represent data dependencies between nodes:

```cpp
struct Edge {
  // Pointer to the target function/node
  std::shared_ptr<Node> function;
  
  // Which input of the target function this edge connects to
  uint32_t input_nr;
};
```

**Edge Semantics:**
- Each edge connects an output of one node to a specific input of another node
- Multiple edges pointing to the same input are **implicitly summed** during backward
- Edges are directional: from output node to input node in the backward graph

### 1.3 Graph Data Structures

**Location:** `torch/csrc/autograd/graph_task.h` (lines 17-230)

The `GraphTask` structure holds metadata for a single backward execution:

```cpp
struct GraphTask : std::enable_shared_from_this<GraphTask> {
  // Nodes waiting for inputs (not ready to execute)
  std::unordered_map<Node*, InputBuffer> not_ready_;
  
  // Dependency count for each node
  std::unordered_map<Node*, int> dependencies_;
  
  // Nodes in the graph
  std::unordered_set<Node*> nodes_in_graph_;
  
  // Root nodes of the backward graph
  c10::SmallVector<Node*, 4> graph_roots_;
  
  // Execution info for selective execution
  std::unordered_map<Node*, ExecInfo> exec_info_;
  
  // Captured gradients for .grad() calls
  std::vector<at::Tensor> captured_vars_;
};
```

---

## 2. GRAPH BUILDING PROCESS

### 2.1 Forward Pass: Graph Construction

**Location:** `torch/csrc/autograd/functions/utils.h` (lines 66-91)

During forward pass, operations record themselves via `set_history()`:

```cpp
inline void set_history(
    const at::Tensor& variable,
    const std::shared_ptr<Node>& grad_fn) {
  // Add input metadata to the grad_fn
  auto output_nr = grad_fn->add_input_metadata(variable);
  
  // Set the gradient edge on the output tensor
  impl::set_gradient_edge(variable, {grad_fn, output_nr});
}
```

**Process Flow:**
1. Operation creates a new `Node` (e.g., `AddBackward0`)
2. Node stores input metadata (shape, dtype, device)
3. Node's `next_edges` are set to point to input nodes' grad_fns
4. Output tensor's `grad_fn` is set to this new node
5. Output tensor's `output_nr` tracks which output this is

### 2.2 Role of Autograd in Graph Building

**Location:** `torch/csrc/autograd/autograd.cpp` (lines 90-203)

The autograd system:
- Intercepts tensor operations via dispatch mechanism
- Creates backward nodes for differentiable operations
- Chains nodes together via edges
- Maintains graph connectivity

### 2.3 Operation Recording

**Location:** `torch/csrc/autograd/python_function.cpp` (lines 1292-1350)

When a custom `Function.apply()` is called:

```python
class Exp(Function):
    @staticmethod
    def forward(ctx, i):
        result = i.exp()
        ctx.save_for_backward(result)
        return result
    
    @staticmethod
    def backward(ctx, grad_output):
        result, = ctx.saved_tensors
        return grad_output * result
```

The framework:
1. Creates an `ExpBackward` node
2. Saves input metadata via `add_input_metadata()`
3. Saves tensors via `ctx.save_for_backward()`
4. Sets output tensor's grad_fn to `ExpBackward`

### 2.4 Eager vs Graph Mode

**Eager Mode (Default):**
- Graph built dynamically during forward pass
- Each operation immediately creates nodes
- Flexible control flow (loops, conditionals)

**Graph Mode (TorchScript/FX):**
- Graph traced/scripted before execution
- Static graph structure
- Optimizations applied to entire graph

---

## 3. GRAPH EXECUTION

### 3.1 Backward Pass Initialization

**Location:** `torch/csrc/autograd/engine.cpp` (lines 1288-1380)

```cpp
auto Engine::execute(
    const edge_list& root_edges,
    const variable_list& inputs,
    bool keep_graph,
    bool create_graph,
    bool accumulate_grad,
    const edge_list& outputs) -> variable_list {
  
  // Create GraphTask with metadata
  auto graph_task = std::make_shared<GraphTask>(
      keep_graph, create_graph, reentrant_depth,
      local_ready_queue, graph_roots);
  
  // Create GraphRoot node
  auto graph_root = std::make_shared<GraphRoot>(root_edges, inputs);
  
  // Compute dependencies
  compute_dependencies(graph_root.get(), *graph_task, min_topo_nr);
  
  // Initialize execution info
  if (!outputs.empty()) {
    graph_task->init_to_execute(*graph_root, outputs, accumulate_grad, min_topo_nr);
  }
}
```

### 3.2 Execution Order and Scheduling

**Location:** `torch/csrc/autograd/engine.h` (lines 86-125)

The `ReadyQueue` uses a priority queue:

```cpp
struct ReadyQueue {
  struct CompareNodeTaskTime {
    bool operator()(NodeTask const& t1, NodeTask const& t2) {
      // Shutdown tasks first
      if (t2.isShutdownTask_) return true;
      
      // Then by reentrant depth
      if (t1.getReentrantDepth() == t2.getReentrantDepth()) {
        // Then by sequence number (higher = earlier)
        return t1.fn_->sequence_nr() < t2.fn_->sequence_nr();
      }
      return t1.getReentrantDepth() > t2.getReentrantDepth();
    }
  };
  
  std::priority_queue<NodeTask, std::vector<NodeTask>, CompareNodeTaskTime> heap_;
};
```

**Execution Priority:**
1. Nodes with higher sequence numbers execute first (reverse topological order)
2. Reentrant depth determines thread assignment
3. Ensures gradients flow backward correctly

### 3.3 Gradient Computation

**Location:** `torch/csrc/autograd/engine.cpp` (lines 1400-1500)

During backward execution:
1. **Gradient Accumulation:** Multiple edges to same input are summed
2. **Node Execution:** Each node's `apply()` method computes gradients
3. **Dependency Tracking:** `not_ready_` map tracks pending inputs
4. **Ready Queue:** Nodes become ready when all inputs arrive

### 3.4 Optimization Passes

**Key Optimizations:**
- **Topological Sorting:** Ensures correct execution order
- **Dependency Pruning:** Only executes nodes needed for outputs
- **Inplace Detection:** Tracks inplace operations for correctness
- **Stream Synchronization:** Handles multi-GPU execution

---

## 4. KEY SOURCE FILES

| File | Purpose |
|------|---------|
| `torch/csrc/autograd/function.h` | Node class definition |
| `torch/csrc/autograd/edge.h` | Edge structure |
| `torch/csrc/autograd/engine.h/cpp` | Backward execution engine |
| `torch/csrc/autograd/graph_task.h` | Graph execution metadata |
| `torch/csrc/autograd/functions/basic_ops.h/cpp` | Basic operation nodes |
| `torch/csrc/autograd/functions/utils.h` | Graph building utilities |
| `torch/autograd/function.py` | Python Function API |
| `torch/fx/graph.py` | FX graph representation |

---

## 5. EXAMPLE: Simple Forward and Backward

```python
import torch

# Forward pass: builds graph
x = torch.tensor([2.0], requires_grad=True)
y = x * 3  # Creates MulBackward0 node
z = y + 2  # Creates AddBackward0 node
loss = z.sum()  # Creates SumBackward0 node

# Graph structure:
# x (leaf) -> MulBackward0 -> AddBackward0 -> SumBackward0 -> AccumulateGrad(x)

# Backward pass: executes graph in reverse
loss.backward()  # Traverses: SumBackward0 -> AddBackward0 -> MulBackward0 -> AccumulateGrad
print(x.grad)  # tensor([3.])
```

**Graph Visualization:**
```
Forward:  x --[*3]--> y --[+2]--> z --[sum]--> loss
Backward: x <--[*3]-- y <--[+2]-- z <--[sum]-- loss
```

---

## 6. CONCLUSION

PyTorch's graph system elegantly combines:
- **Dynamic construction** during forward pass
- **Efficient execution** via topological ordering
- **Flexible control flow** through eager evaluation
- **Scalability** via multi-threaded execution

The design enables both research flexibility and production performance.

