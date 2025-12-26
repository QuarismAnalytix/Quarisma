# PyTorch Graph Construction and Execution: Detailed Code Examples

## 1. NODE CREATION AND EDGE SETUP

### 1.1 Node Constructor

**File:** `torch/csrc/autograd/function.h` (lines 115-140)

```cpp
// Node constructor with sequence number
explicit Node(uint64_t sequence_nr, edge_list&& next_edges = edge_list())
    : sequence_nr_(sequence_nr), next_edges_(std::move(next_edges)) {
  // Update topological numbers based on next_edges
  for (const Edge& edge : next_edges_) {
    update_topological_nr(edge);
  }
  
  // Store anomaly metadata if enabled
  if (AnomalyMode::is_enabled()) {
    metadata()->store_stack();
    assign_parent();
  }
  
  // Record thread ID for profiling
  thread_id_ = at::RecordFunction::currentThreadId();
}

// Default constructor (auto-increments sequence number)
explicit Node(edge_list&& next_edges = edge_list())
    : Node(at::sequence_number::get_and_increment(), std::move(next_edges)) {}
```

### 1.2 Adding Next Edges

**File:** `torch/csrc/autograd/function.h` (lines 298-325)

```cpp
// Update topological number when adding edge
void update_topological_nr(const Edge& edge) {
  TORCH_INTERNAL_ASSERT(!has_parent_,
      "Cannot update topological_nr after node has a parent");
  Node* node = edge.function.get();
  if (node) {
    auto topo_nr = node->topological_nr();
    if (topological_nr_ <= topo_nr) {
      topological_nr_ = topo_nr + 1;  // Parent > all children
    }
  }
}

// Set edge at specific index
void set_next_edge(size_t index, Edge edge) {
  update_topological_nr(edge);
  next_edges_[index] = std::move(edge);
}

// Add new edge
void add_next_edge(Edge edge) {
  update_topological_nr(edge);
  next_edges_.emplace_back(std::move(edge));
}

// Set all edges at once
void set_next_edges(edge_list&& next_edges) {
  next_edges_ = std::move(next_edges);
  for (const auto& next_edge : next_edges_) {
    update_topological_nr(next_edge);
  }
}
```

---

## 2. GRAPH BUILDING DURING FORWARD PASS

### 2.1 Setting Tensor History

**File:** `torch/csrc/autograd/functions/utils.h` (lines 66-91)

```cpp
// Single tensor history
inline void set_history(
    const at::Tensor& variable,
    const std::shared_ptr<Node>& grad_fn) {
  TORCH_CHECK(grad_fn != nullptr);
  if (variable.defined()) {
    // Add input metadata to grad_fn
    auto output_nr = grad_fn->add_input_metadata(variable);
    
    // Set gradient edge on output tensor
    impl::set_gradient_edge(variable, {grad_fn, output_nr});
  } else {
    grad_fn->add_input_metadata(Node::undefined_input());
  }
}

// Multiple tensors history
inline void set_history(
    const std::vector<Variable>& variables,
    const std::shared_ptr<Node>& grad_fn) {
  for (auto& variable : variables) {
    set_history(variable, grad_fn);
  }
}
```

### 2.2 Input Metadata Management

**File:** `torch/csrc/autograd/function.h` (lines 197-231)

```cpp
// Add metadata for a tensor input
uint32_t add_input_metadata(const at::Tensor& t) noexcept {
  uint32_t input_nr = input_metadata_.size();
  input_metadata_.emplace_back(t);  // Stores shape, dtype, device, stream
  return input_nr;
}

// Add metadata with explicit parameters
uint32_t add_input_metadata(
    const at::TensorOptions& options,
    c10::SymIntArrayRef shape,
    bool is_tensor_subclass,
    bool is_nested,
    std::optional<at::ScalarType> grad_dtype) noexcept {
  uint32_t input_nr = input_metadata_.size();
  auto meta_shape = MetadataShape{std::in_place_type<SymIntSmallVec>, shape};
  input_metadata_.emplace_back(
      options, meta_shape, is_tensor_subclass, is_nested, grad_dtype);
  return input_nr;
}

// Add placeholder for unused input
uint32_t add_input_metadata(undefined_input u) noexcept {
  uint32_t input_nr = input_metadata_.size();
  input_metadata_.emplace_back();
  return input_nr;
}
```

---

## 3. BACKWARD PASS EXECUTION

### 3.1 Engine Execute Method

**File:** `torch/csrc/autograd/engine.cpp` (lines 1288-1352)

```cpp
auto Engine::execute(
    const edge_list& root_edges,
    const variable_list& inputs,
    bool keep_graph,
    bool create_graph,
    bool accumulate_grad,
    const edge_list& outputs) -> variable_list {
  
  // Validate outputs
  validate_outputs(root_edges, const_cast<variable_list&>(inputs),
      [](const std::string& msg) { return msg; });
  
  // Initialize ready queue
  init_local_ready_queue();
  bool not_reentrant_backward_call = worker_device == NO_DEVICE;
  
  // Extract root nodes
  c10::SmallVector<Node*, 4> temp_roots{root_edges.size()};
  for (const auto i : c10::irange(root_edges.size())) {
    temp_roots[i] = root_edges[i].function.get();
  }
  
  // Create graph task
  auto graph_task = std::make_shared<GraphTask>(
      keep_graph, create_graph,
      not_reentrant_backward_call ? 0 : total_depth + 1,
      local_ready_queue, std::move(temp_roots));
  
  // Create graph root (entry point)
  bool skip_dummy_node = root_edges.size() == 1 && compiled_autograd == nullptr;
  auto graph_root = skip_dummy_node
      ? root_edges.at(0).function
      : std::make_shared<GraphRoot>(root_edges, inputs);
  
  // Compute dependencies
  auto min_topo_nr = compute_min_topological_nr(outputs);
  compute_dependencies(graph_root.get(), *graph_task, min_topo_nr);
  
  // Initialize execution info for selective execution
  if (!outputs.empty()) {
    graph_task->init_to_execute(*graph_root, outputs, accumulate_grad, min_topo_nr);
  }
}
```

### 3.2 Dependency Computation

**File:** `torch/csrc/autograd/engine.cpp` (lines 1666-1760)

```cpp
void GraphTask::init_to_execute(
    Node& graph_root,
    const edge_list& outputs,
    bool accumulate_grad,
    uint64_t min_topo_nr) {
  
  // Mark output nodes as needed
  int output_idx = 0;
  for (auto& output_edge : outputs) {
    Node* output = output_edge.function.get();
    auto& info = exec_info_[output];
    if (accumulate_grad) {
      info.needed_ = true;
    } else {
      // For .grad(), capture gradients
      if (!info.captures_) {
        info.captures_ = std::make_unique<std::vector<ExecInfo::Capture>>();
      }
      info.captures_->emplace_back(output_edge.input_nr, output_idx++);
    }
  }
  
  // Traverse graph to mark needed nodes
  struct Frame {
    Frame(Node* fn) : fn_(fn) {}
    Node* fn_{};
    size_t next_next_fn_{};
    
    Node* get_next_fn() {
      const auto& next = fn_->next_edges();
      auto num_next = next.size();
      while (next_next_fn_ < num_next) {
        auto fn = next[next_next_fn_++].function.get();
        if (fn) return fn;
      }
      return nullptr;
    }
  };
  
  std::vector<Frame> stack;
  std::unordered_set<Node*> seen;
  stack.emplace_back(&graph_root);
  exec_info_.emplace(stack.back().fn_, ExecInfo());
  
  // Iterative DFS to mark needed nodes
  while (!stack.empty()) {
    auto& frame = stack.back();
    const auto fn = frame.fn_;
    
    Node* child_fn = nullptr;
    while ((child_fn = frame.get_next_fn()) && !seen.emplace(child_fn).second) {
      if (nodeShouldExecute(child_fn)) {
        exec_info_[fn].needed_ = true;
      }
    }
    
    if (child_fn) {
      stack.emplace_back(child_fn);
      exec_info_.emplace(child_fn, ExecInfo());
    } else {
      stack.pop_back();
    }
  }
}
```

---

## 4. READY QUEUE AND SCHEDULING

**File:** `torch/csrc/autograd/engine.h` (lines 86-125)

```cpp
struct ReadyQueue {
  struct CompareNodeTaskTime {
    bool operator()(NodeTask const& t1, NodeTask const& t2) {
      // Shutdown tasks have highest priority
      if (t2.isShutdownTask_) {
        return true;
      } else if (!t1.fn_ || t1.isShutdownTask_) {
        return false;
      } else if (!t2.fn_) {
        return true;
      }
      
      // Sort by reentrant depth (lower depth = higher priority)
      else if (t1.getReentrantDepth() == t2.getReentrantDepth()) {
        // Within same depth, higher sequence number = higher priority
        return t1.fn_->sequence_nr() < t2.fn_->sequence_nr();
      }
      return t1.getReentrantDepth() > t2.getReentrantDepth();
    }
  };
  
  std::priority_queue<NodeTask, std::vector<NodeTask>, CompareNodeTaskTime> heap_;
  
  void push(NodeTask item, bool incrementOutstandingTasks = true);
  NodeTask pop();
  bool empty() const;
  size_t size() const;
};
```

---

## 5. BASIC OPERATION NODES

**File:** `torch/csrc/autograd/functions/basic_ops.h` (lines 85-113)

```cpp
// GraphRoot: Entry point for backward pass
struct TORCH_API GraphRoot : public Node {
  GraphRoot(edge_list functions, variable_list inputs)
      : Node(std::move(functions)), outputs(std::move(inputs)) {
    // Store metadata for all root gradients
    for (const auto& t : outputs) {
      add_input_metadata(t);
    }
  }
  
  variable_list apply(variable_list&& inputs) override {
    return outputs;  // Return root gradients
  }
  
  variable_list outputs;
};

// Error: Represents unsupported backward operation
struct TORCH_API Error : public Node {
  Error(std::string msg, edge_list&& next_edges)
      : Node(std::move(next_edges)), msg(std::move(msg)) {}
  
  variable_list apply(variable_list&& inputs) override;
  
  std::string msg;
};
```

---

## 6. PYTHON CUSTOM FUNCTION EXAMPLE

**File:** `torch/autograd/function.py` (lines 472-566)

```python
class Function(_SingleLevelFunction):
    @staticmethod
    def forward(ctx, *args, **kwargs):
        """Compute forward pass and save tensors for backward"""
        raise NotImplementedError
    
    @staticmethod
    def backward(ctx, *grad_outputs):
        """Compute gradients given output gradients"""
        raise NotImplementedError
    
    @classmethod
    def apply(cls, *args, **kwargs):
        """Apply the function (creates backward node)"""
        # Unpacks inputs and creates backward node
        # Sets grad_fn on outputs
        # Returns outputs
        return super().apply(*args, **kwargs)

# Example usage
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

output = Exp.apply(input)  # Creates ExpBackward node
```

---

## 7. KEY INSIGHTS

1. **Lazy Evaluation:** Nodes created during forward, executed during backward
2. **Topological Ordering:** Ensures correct gradient flow
3. **Sequence Numbers:** Enable reverse execution order
4. **Metadata Storage:** Enables gradient shape inference
5. **Thread Safety:** Shared pointers and mutex protection
6. **Selective Execution:** Only execute nodes needed for outputs

