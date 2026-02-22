# Quarisma Graph Data Flow and Practical Examples

## 1. COMPLETE FORWARD-BACKWARD EXAMPLE

### 1.1 Simple Computation

```python
import quarisma

# Step 1: Create leaf tensors
x = quarisma.tensor([2.0], requires_grad=True)
w = quarisma.tensor([3.0], requires_grad=True)

# Step 2: Forward pass (graph construction)
y = x * w        # Creates MulBackward0 node
z = y + 1        # Creates AddBackward0 node
loss = z.sum()   # Creates SumBackward0 node

# Graph structure after forward:
# x (leaf) ──[MulBackward0]──> y
# w (leaf) ──[MulBackward0]──> y
#                               │
#                          [AddBackward0]
#                               │
#                               z
#                               │
#                          [SumBackward0]
#                               │
#                             loss

# Step 3: Backward pass (graph execution)
loss.backward()

# Gradient computation:
# d(loss)/d(z) = 1.0 (from SumBackward0)
# d(loss)/d(y) = 1.0 (from AddBackward0)
# d(loss)/d(x) = d(loss)/d(y) * d(y)/d(x) = 1.0 * w = 3.0
# d(loss)/d(w) = d(loss)/d(y) * d(y)/d(w) = 1.0 * x = 2.0

print(x.grad)  # tensor([3.])
print(w.grad)  # tensor([2.])
```

### 1.2 Graph with Multiple Paths

```python
import quarisma

x = quarisma.tensor([2.0], requires_grad=True)

# Multiple paths to same node
y = x * 3
z = x * 2
loss = y + z  # Both y and z depend on x

# Graph:
#        ┌─[MulBackward0]─> y ─┐
# x ─────┤                      [AddBackward0]─> loss
#        └─[MulBackward0]─> z ─┘

loss.backward()

# Gradients are summed:
# d(loss)/d(x) = d(loss)/d(y) * 3 + d(loss)/d(z) * 2
#              = 1.0 * 3 + 1.0 * 2 = 5.0

print(x.grad)  # tensor([5.])
```

---

## 2. EDGE STRUCTURE AND DATA FLOW

### 2.1 Edge Creation During Forward

**File:** `quarisma/csrc/autograd/edge.h`

```cpp
// Edge represents: (target_node, input_index)
struct Edge {
  std::shared_ptr<Node> function;  // Target node
  uint32_t input_nr;               // Which input of target
};

// Example: For operation y = x + w
// Two edges are created:
// Edge 1: (AddBackward0, input_nr=0) <- from x's grad_fn
// Edge 2: (AddBackward0, input_nr=1) <- from w's grad_fn
```

### 2.2 Gradient Accumulation at Edges

When multiple edges point to the same input:

```python
import quarisma

x = quarisma.tensor([1.0], requires_grad=True)

# Create multiple paths
y = x * 2
z = x * 3
loss = y + z

# During backward, gradients are accumulated:
# At AddBackward0 input 0: receives grad from y (value: 1.0)
# At AddBackward0 input 1: receives grad from z (value: 1.0)
# Both flow back to x via MulBackward0 nodes
# Final gradient at x: 2.0 + 3.0 = 5.0

loss.backward()
print(x.grad)  # tensor([5.])
```

---

## 3. GRAPH TASK EXECUTION FLOW

### 3.1 Execution Phases

**Phase 1: Initialization**
```
Engine::execute()
  ├─ Create GraphTask
  ├─ Create GraphRoot (entry point)
  ├─ Compute dependencies
  └─ Initialize execution info
```

**Phase 2: Dependency Computation**
```
compute_dependencies()
  ├─ Traverse graph from root
  ├─ Count incoming edges for each node
  ├─ Store in dependencies_ map
  └─ Mark nodes as "not_ready"
```

**Phase 3: Execution**
```
execute_graph_task()
  ├─ Queue root node
  ├─ While ready_queue not empty:
  │  ├─ Pop highest priority node
  │  ├─ Execute node.apply(gradients)
  │  ├─ For each output edge:
  │  │  ├─ Add gradient to target node's input buffer
  │  │  ├─ Decrement target's dependency count
  │  │  └─ If count == 0, queue target node
  │  └─ Continue
  └─ Return gradients
```

### 3.2 Ready Queue Priority

```cpp
// Execution order determined by:
// 1. Reentrant depth (lower = higher priority)
// 2. Sequence number (higher = higher priority)

// Example execution order for: loss = (x * 3 + 2).sum()
// 1. SumBackward0 (highest sequence_nr)
// 2. AddBackward0
// 3. MulBackward0
// 4. AccumulateGrad(x)
```

---

## 4. INPUT BUFFER AND GRADIENT ACCUMULATION

### 4.1 InputBuffer Structure

**File:** `quarisma/csrc/autograd/input_buffer.h`

```cpp
// Accumulates gradients for a node's inputs
class InputBuffer {
  std::vector<at::Tensor> buffer_;  // One slot per input

  void add(size_t pos, at::Tensor&& grad) {
    if (buffer_[pos].defined()) {
      buffer_[pos] = buffer_[pos] + grad;  // Accumulate
    } else {
      buffer_[pos] = std::move(grad);
    }
  }
};
```

### 4.2 Gradient Flow Example

```python
import quarisma

x = quarisma.tensor([1.0, 2.0], requires_grad=True)

# Multiple uses of x in same operation
y = x + x  # x appears twice

# During backward:
# SumBackward0 produces grad: [1.0, 1.0]
# AddBackward0 receives: [1.0, 1.0]
# AddBackward0 has two inputs (both x):
#   - Input 0: receives [1.0, 1.0]
#   - Input 1: receives [1.0, 1.0]
# Both accumulated in InputBuffer
# MulBackward0 (or AccumulateGrad) receives sum: [2.0, 2.0]

y.sum().backward()
print(x.grad)  # tensor([2., 2.])
```

---

## 5. SELECTIVE EXECUTION (.grad() vs .backward())

### 5.1 .backward() - Full Accumulation

```python
import quarisma

x = quarisma.tensor([1.0], requires_grad=True)
y = x * 2
z = y * 3

z.backward()  # Accumulates into x.grad
print(x.grad)  # tensor([6.])

x.grad.zero_()
z.backward()  # Accumulates again
print(x.grad)  # tensor([6.])
```

### 5.2 .grad() - Selective Capture

```python
import quarisma

x = quarisma.tensor([1.0], requires_grad=True)
y = x * 2
z = y * 3

# Only compute gradient w.r.t. x, don't accumulate
grad_x = quarisma.autograd.grad(z, x)
print(grad_x)  # (tensor([6.]),)

# x.grad is still None
print(x.grad)  # None
```

**Implementation:**
- `.backward()`: Sets `accumulate_grad=True`, uses `AccumulateGrad` nodes
- `.grad()`: Sets `accumulate_grad=False`, captures gradients in `captured_vars_`

---

## 6. EXECUTION INFO AND FILTERING

### 6.1 ExecInfo Structure

**File:** `quarisma/csrc/autograd/graph_task.h` (lines 47-130)

```cpp
struct ExecInfo {
  bool needed_ = false;  // Should this node execute?

  // For .grad() calls, capture gradients
  std::unique_ptr<std::vector<Capture>> captures_;

  struct Capture {
    int input_idx_;   // Which input of this node
    int output_idx_;  // Where to store in captured_vars_
  };

  bool should_execute() const {
    return needed_ || captures_;
  }
};
```

### 6.2 Selective Execution Example

```python
import quarisma

x = quarisma.tensor([1.0], requires_grad=True)
y = quarisma.tensor([2.0], requires_grad=True)

z = x * 2
w = y * 3
loss = z + w

# Only compute gradient w.r.t. x
grad_x = quarisma.autograd.grad(loss, x)

# Execution graph:
# - AddBackward0: EXECUTED (needed for output)
# - MulBackward0(z): EXECUTED (path to x)
# - MulBackward0(w): SKIPPED (not on path to x)
# - AccumulateGrad(y): SKIPPED (not needed)

print(grad_x)  # (tensor([2.]),)
```

---

## 7. ANOMALY MODE AND DEBUGGING

### 7.1 Anomaly Detection

```python
import quarisma

with quarisma.autograd.anomaly_mode.set_detect_anomaly(True):
    x = quarisma.tensor([1.0], requires_grad=True)
    y = x ** 2
    z = y.sum()
    z.backward()

# If NaN/Inf detected, prints:
# - Which operation produced it
# - Stack trace of forward pass
# - Parent node information
```

---

## 8. KEY TAKEAWAYS

1. **Graph is built lazily** during forward pass
2. **Execution is eager** during backward pass
3. **Gradients accumulate** at nodes with multiple inputs
4. **Topological order** ensures correct computation
5. **Selective execution** optimizes for specific outputs
6. **Thread-safe** via shared pointers and mutexes


