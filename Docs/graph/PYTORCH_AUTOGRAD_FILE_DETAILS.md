# PyTorch Autograd: Detailed File Reference

**In-depth information about key source files in the autograd system**

---

## CORE GRAPH STRUCTURE FILES

### torch/csrc/autograd/function.h (Lines 113-792)

**Purpose:** Defines the `Node` class, the fundamental building block of the computational graph.

**Key Components:**
- `class Node` - Base class for all autograd operations
  - `sequence_nr_` - Thread-local monotonically increasing ID for execution priority
  - `topological_nr_` - Ensures parent nodes have higher topo_nr than children
  - `next_edges_` - Vector of edges to next nodes in the graph
  - `input_metadata_` - Stores tensor shape, dtype, device, stream info
  - `saved_variables_` - Tensors saved for backward pass
  - `anomaly_metadata_` - Anomaly detection metadata

- `struct SavedVariable` - Wrapper for saving tensors in backward
- `class NodeGuard` - RAII guard for current evaluating node
- Key methods:
  - `apply()` - Execute backward pass for this node
  - `add_input_metadata()` - Store input tensor metadata
  - `add_next_edge()` - Connect to next node
  - `update_topological_nr()` - Update topological ordering
  - `release_variables()` - Release saved tensors

**Dependencies:** `edge.h`, `saved_variable.h`, `input_metadata.h`

---

### torch/csrc/autograd/edge.h (Lines 1-57)

**Purpose:** Defines the `Edge` structure representing data dependencies between nodes.

**Key Components:**
- `struct Edge` - Represents a connection between nodes
  - `std::shared_ptr<Node> function` - Target node
  - `uint32_t input_nr` - Which input of target node this edge connects to
  - Default constructor creates null edge
  - Comparison operators for edge equality

**Usage:** Every node has a `next_edges_` vector of Edge objects pointing to its inputs.

---

### torch/csrc/autograd/graph_task.h (Lines 17-230)

**Purpose:** Defines `GraphTask`, the metadata container for a single backward execution.

**Key Components:**
- `struct GraphTask` - Holds execution metadata
  - `not_ready_` - Map of nodes to InputBuffer (gradients not yet ready)
  - `dependencies_` - Map of nodes to dependency count
  - `nodes_in_graph_` - Set of all nodes in this backward pass
  - `graph_roots_` - Entry points for backward
  - `exec_info_` - Execution info for selective execution
  - `ready_queue_` - Priority queue of ready nodes
  - `captured_vars_` - Variables captured for selective execution
  - `leaf_streams_` - Stream info for multi-GPU

- `struct ExecInfo` - Controls selective execution
  - `needed_` - Whether this node is needed for output
  - `captures_` - Gradient capture info
  - `should_execute()` - Determines if node should run

- `class GraphTaskGuard` - RAII guard for current graph task

**Key Methods:**
- `get_current_graph_task()` - Get active GraphTask
- `get_current_graph_task_exec_info()` - Get execution info
- `add_node_to_current_graph_task_exec_info()` - Register node

---

### torch/csrc/autograd/input_buffer.h/cpp

**Purpose:** Implements `InputBuffer` for accumulating gradients from multiple edges.

**Key Components:**
- `class InputBuffer` - Accumulates gradients for a node's inputs
  - `buffer_` - Vector of optional tensors
  - `num_inputs_` - Number of inputs
  - `add()` - Add gradient to buffer
  - `get()` - Retrieve accumulated gradient
  - Handles implicit summation of multiple paths

**Algorithm:**
1. Initialize buffer with size = number of inputs
2. For each incoming gradient, add to corresponding buffer slot
3. When all inputs ready, sum all gradients
4. Return accumulated gradient

---

### torch/csrc/autograd/variable.h/cpp

**Purpose:** Manages tensor autograd metadata and grad_fn connections.

**Key Components:**
- `Variable` - Alias for `at::Tensor`
- Autograd metadata stored in tensor impl:
  - `grad_fn_` - Backward node for this tensor
  - `grad_` - Accumulated gradient
  - `requires_grad_` - Whether to compute gradients
  - `is_leaf_` - Whether this is a leaf tensor
  - `retains_grad_` - Whether to retain gradient for non-leaf

**Key Functions:**
- `grad_fn()` - Get grad_fn of tensor
- `set_grad_fn()` - Set grad_fn (called during forward)
- `grad()` - Get accumulated gradient
- `set_grad()` - Set gradient
- `rebase_history()` - Update grad_fn after in-place op
- `bump_version()` - Increment version counter
- `add_hook()` - Register backward hook

---

## GRAPH BUILDING FILES

### torch/csrc/autograd/functions/utils.h (Lines 66-91)

**Purpose:** Implements `set_history()`, the critical function connecting tensors to their grad_fn.

**Key Function:**
```cpp
void set_history(
    const Tensor& output,
    const std::shared_ptr<Node>& grad_fn,
    uint32_t output_nr = 0)
```

**Algorithm:**
1. Create Edge from grad_fn to its inputs
2. Set output.grad_fn = grad_fn
3. Store input metadata in grad_fn
4. Connect grad_fn to input nodes via next_edges

**Called During:** Every operation that creates a backward node

---

### torch/csrc/autograd/functions/accumulate_grad.h/cpp

**Purpose:** Implements `AccumulateGrad`, the special leaf node that accumulates gradients.

**Key Components:**
- `class AccumulateGrad : public Node` - Leaf node for parameter gradients
  - `variable_` - Reference to the leaf tensor
  - `apply()` - Accumulate gradient into variable.grad

**Algorithm:**
1. Receive gradient from backward pass
2. If variable.grad is None, set it to gradient
3. Otherwise, add gradient to variable.grad
4. Return empty (leaf node has no inputs)

---

### torch/csrc/autograd/functions/basic_ops.h/cpp

**Purpose:** Implements basic operation nodes used in graph execution.

**Key Classes:**
- `class GraphRoot : public Node` - Entry point for backward pass
  - `apply()` - Returns input gradients unchanged
  - Used to start backward execution

- `class Error : public Node` - Placeholder for unsupported operations
- `class DelayedError : public Node` - Deferred error reporting

---

## GRAPH EXECUTION FILES

### torch/csrc/autograd/engine.h (Lines 86-125)

**Purpose:** Defines the `Engine` class and `ReadyQueue` for backward execution.

**Key Components:**
- `class Engine` - Singleton backward execution engine
  - `execute()` - Main backward execution method
  - `get_default_engine()` - Get singleton instance
  - Thread pool for parallel execution

- `class ReadyQueue` - Priority queue for node scheduling
  - `push()` - Add node to queue
  - `pop()` - Get highest priority node
  - Priority: shutdown > lower reentrant_depth > higher sequence_nr

**Execution Priority:**
1. Shutdown tasks (highest)
2. Lower reentrant_depth (nested backward)
3. Higher sequence_nr (later operations first)
4. Regular tasks (lowest)

---

### torch/csrc/autograd/engine.cpp (Lines 1288-1380)

**Purpose:** Implements the backward execution algorithm.

**Key Method: Engine::execute()**

**Algorithm:**
1. Create GraphTask with execution metadata
2. Compute dependencies for all nodes
3. Initialize ready_queue with GraphRoot
4. While ready_queue not empty:
   - Pop highest priority node
   - Execute node.apply(gradients)
   - Accumulate outputs in InputBuffer
   - Decrement dependency count
   - Queue ready nodes
5. Return gradients

**Key Features:**
- Multi-threaded execution
- Stream synchronization for multi-GPU
- Selective execution via ExecInfo
- Anomaly detection support

---

### torch/csrc/autograd/python_engine.h/cpp

**Purpose:** Python bindings for the backward engine.

**Key Components:**
- `PyEngine` - Python wrapper for Engine
- Python methods:
  - `backward()` - Trigger backward pass
  - `grad()` - Compute gradients for specific outputs
  - `set_num_threads()` - Configure thread pool

---

## SUPPORTING INFRASTRUCTURE FILES

### torch/csrc/autograd/saved_variable.h/cpp

**Purpose:** Implements `SavedVariable` for storing tensors in backward.

**Key Components:**
- `class SavedVariable` - Wrapper for saved tensors
  - `tensor_` - The saved tensor
  - `hooks_` - Hooks for tensor packing/unpacking
  - `pack()` - Serialize tensor (e.g., to CPU)
  - `unpack()` - Deserialize tensor

**Features:**
- Supports saved tensor hooks for memory optimization
- Can move tensors to CPU to save GPU memory
- Automatic unpacking during backward

---

### torch/csrc/autograd/anomaly_mode.h/cpp

**Purpose:** Implements anomaly detection for debugging.

**Key Components:**
- `class AnomalyMode` - Anomaly detection context
- `class AnomalyMetadata` - Stores anomaly info per node
  - `parent_` - Parent node in anomaly trace
  - `traceback_` - Forward pass traceback

**Features:**
- Prints forward traceback when backward fails
- Detects NaN values in gradients
- Helps debug gradient computation issues

---

### torch/csrc/autograd/custom_function.h/cpp

**Purpose:** Implements custom autograd function support.

**Key Components:**
- `class CustomFunction` - Base for user-defined backward
- `class PythonFunction` - Python custom function wrapper
- Supports:
  - Custom forward implementation
  - Custom backward implementation
  - Saved tensors and context

---

### torch/csrc/autograd/forward_grad.h/cpp

**Purpose:** Implements forward-mode AD (tangent computation).

**Key Components:**
- `class ForwardGrad` - Forward gradient tracking
- `class DualTensor` - Tensor with tangent
- Supports:
  - Jacobian-vector products (JVP)
  - Forward-mode differentiation
  - Nested forward-backward

---

## PYTHON API FILES

### torch/autograd/function.py (Lines 472-566)

**Purpose:** Python API for custom autograd functions.

**Key Classes:**
- `class Function` - Base for custom autograd functions
  - `forward()` - User-defined forward pass
  - `backward()` - User-defined backward pass
  - `ctx` - Context object for saving tensors

- `class FunctionCtx` - Context for forward/backward
  - `save_for_backward()` - Save tensors
  - `mark_dirty()` - Mark in-place modifications
  - `mark_non_differentiable()` - Mark non-differentiable outputs

**Usage:**
```python
class MyFunc(Function):
    @staticmethod
    def forward(ctx, x):
        ctx.save_for_backward(x)
        return x * 2
    
    @staticmethod
    def backward(ctx, grad_output):
        x, = ctx.saved_tensors
        return grad_output * 2
```

---

### torch/autograd/graph.py

**Purpose:** Graph inspection and manipulation API.

**Key Classes:**
- `class Node` - Abstract base for graph nodes
  - `name()` - Get node name
  - `next_functions` - Get input nodes
  - `metadata()` - Get node metadata
  - `register_hook()` - Register backward hook

- `class GradientEdge` - Represents edge in graph
  - `node` - Target node
  - `input_nr` - Input index

**Key Functions:**
- `get_gradient_edge()` - Get edge for tensor
- `increment_version()` - Update version counter
- `saved_tensors_hooks()` - Hook saved tensors
- `register_multi_grad_hook()` - Multi-tensor hook

---

### torch/autograd/grad_mode.py

**Purpose:** Control gradient computation mode.

**Key Classes:**
- `class no_grad` - Disable gradient computation
- `class enable_grad` - Enable gradient computation
- `class inference_mode` - Inference mode (no view tracking)
- `class set_grad_enabled` - Set grad enabled state

**Usage:**
```python
with torch.no_grad():
    y = model(x)  # No gradients computed

with torch.enable_grad():
    y = model(x)  # Gradients computed
```

---

### torch/autograd/gradcheck.py

**Purpose:** Gradient checking utilities for testing.

**Key Functions:**
- `gradcheck()` - Check gradients via finite differences
- `gradgradcheck()` - Check second-order gradients
- Compares analytical gradients with numerical gradients

---

## GENERATED CODE

### tools/autograd/gen_autograd.py

**Purpose:** Orchestrates code generation for autograd.

**Generates:**
1. Backward function nodes (gen_autograd_functions.py)
2. VariableType methods (gen_variable_type.py)
3. Python bindings (gen_python_functions.py)

**Output:** `torch/csrc/autograd/generated/` directory

---

## QUICK LOOKUP TABLE

| Need | File | Key Symbol |
|------|------|-----------|
| Node structure | `function.h` | `class Node` |
| Edge structure | `edge.h` | `struct Edge` |
| Graph execution | `engine.h/cpp` | `class Engine` |
| Backward scheduling | `engine.h` | `class ReadyQueue` |
| Gradient accumulation | `input_buffer.h` | `class InputBuffer` |
| Leaf gradients | `accumulate_grad.h` | `class AccumulateGrad` |
| Tensor metadata | `variable.h` | `grad_fn_`, `grad_` |
| History setting | `functions/utils.h` | `set_history()` |
| Custom functions | `custom_function.h` | `class CustomFunction` |
| Anomaly detection | `anomaly_mode.h` | `class AnomalyMode` |
| Forward AD | `forward_grad.h` | `class ForwardGrad` |
| Python API | `torch/autograd/function.py` | `class Function` |
| Graph inspection | `torch/autograd/graph.py` | `class Node` |
| Grad modes | `torch/autograd/grad_mode.py` | `no_grad`, `enable_grad` |


