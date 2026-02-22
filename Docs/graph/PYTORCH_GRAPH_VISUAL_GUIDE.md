# Quarisma Graph Architecture: Visual Guide

## 1. COMPLETE GRAPH LIFECYCLE

```
┌─────────────────────────────────────────────────────────────────┐
│                    FORWARD PASS (Graph Building)                │
└─────────────────────────────────────────────────────────────────┘

  x = quarisma.tensor([2.0], requires_grad=True)
  │
  ├─ Create leaf tensor
  │  └─ No grad_fn (leaf node)
  │
  y = x * 3
  │
  ├─ Execute MulBackward0 node creation
  │  ├─ Store input metadata (shape, dtype, device)
  │  ├─ Set next_edges to input nodes
  │  └─ Set y.grad_fn = MulBackward0
  │
  z = y + 2
  │
  ├─ Execute AddBackward0 node creation
  │  ├─ Store input metadata
  │  ├─ Set next_edges to [MulBackward0, ...]
  │  └─ Set z.grad_fn = AddBackward0
  │
  loss = z.sum()
  │
  ├─ Execute SumBackward0 node creation
  │  ├─ Store input metadata
  │  ├─ Set next_edges to [AddBackward0]
  │  └─ Set loss.grad_fn = SumBackward0
  │
  ✓ Graph is now complete but NOT executed

┌─────────────────────────────────────────────────────────────────┐
│                   GRAPH STRUCTURE (In Memory)                   │
└─────────────────────────────────────────────────────────────────┘

  x (leaf)
  │
  ├─ grad_fn: None
  ├─ requires_grad: True
  └─ grad: None (will be filled)
       ↑
       │ (next_edges[0])
       │
  MulBackward0 (sequence_nr: 1, topo_nr: 3)
  │
  ├─ next_edges: [(AccumulateGrad(x), 0)]
  ├─ input_metadata: [shape, dtype, device of x]
  └─ saved_tensors: [3.0]
       ↑
       │ (next_edges[0])
       │
  AddBackward0 (sequence_nr: 2, topo_nr: 2)
  │
  ├─ next_edges: [(MulBackward0, 0), (None, 0)]
  ├─ input_metadata: [shape, dtype, device of y, 2.0]
  └─ saved_tensors: []
       ↑
       │ (next_edges[0])
       │
  SumBackward0 (sequence_nr: 3, topo_nr: 1)
  │
  ├─ next_edges: [(AddBackward0, 0)]
  ├─ input_metadata: [shape, dtype, device of z]
  └─ saved_tensors: []

┌─────────────────────────────────────────────────────────────────┐
│                  BACKWARD PASS (Graph Execution)                │
└─────────────────────────────────────────────────────────────────┘

  loss.backward()
  │
  ├─ Engine::execute()
  │  ├─ Create GraphTask
  │  ├─ Create GraphRoot (entry point)
  │  ├─ Compute dependencies
  │  │  ├─ SumBackward0: 1 incoming edge
  │  │  ├─ AddBackward0: 1 incoming edge
  │  │  ├─ MulBackward0: 1 incoming edge
  │  │  └─ AccumulateGrad(x): 1 incoming edge
  │  │
  │  └─ Initialize ready_queue
  │     └─ Add GraphRoot (highest priority)
  │
  ├─ Execute nodes in priority order:
  │
  │  1. GraphRoot.apply([1.0])
  │     └─ Output: [1.0] → SumBackward0
  │
  │  2. SumBackward0.apply([1.0])
  │     └─ Output: [1.0] → AddBackward0
  │
  │  3. AddBackward0.apply([1.0])
  │     ├─ Input 0: [1.0] → MulBackward0
  │     └─ Input 1: [1.0] (constant, no grad)
  │
  │  4. MulBackward0.apply([1.0])
  │     ├─ Input 0: [3.0] → AccumulateGrad(x)
  │     └─ Input 1: [2.0] (constant, no grad)
  │
  │  5. AccumulateGrad(x).apply([3.0])
  │     └─ x.grad = [3.0]
  │
  ✓ Backward pass complete

┌─────────────────────────────────────────────────────────────────┐
│                      RESULT: x.grad = [3.0]                    │
└─────────────────────────────────────────────────────────────────┘
```

---

## 2. MULTI-PATH GRADIENT ACCUMULATION

```
┌─────────────────────────────────────────────────────────────────┐
│                    FORWARD PASS (Graph Building)                │
└─────────────────────────────────────────────────────────────────┘

  x = quarisma.tensor([1.0], requires_grad=True)
  
  y = x * 2  ──→ MulBackward0(y)
  z = x * 3  ──→ MulBackward0(z)
  loss = y + z ──→ AddBackward0

┌─────────────────────────────────────────────────────────────────┐
│                   GRAPH STRUCTURE (In Memory)                   │
└─────────────────────────────────────────────────────────────────┘

                    x (leaf)
                    │
         ┌──────────┴──────────┐
         │                     │
    MulBackward0(y)      MulBackward0(z)
    (seq_nr: 1)          (seq_nr: 2)
         │                     │
         └──────────┬──────────┘
                    │
              AddBackward0
              (seq_nr: 3)
                    │
              AccumulateGrad(x)

┌─────────────────────────────────────────────────────────────────┐
│                  BACKWARD PASS (Graph Execution)                │
└─────────────────────────────────────────────────────────────────┘

  loss.backward()
  │
  ├─ AddBackward0.apply([1.0, 1.0])
  │  ├─ Input 0: [1.0] → MulBackward0(y)
  │  └─ Input 1: [1.0] → MulBackward0(z)
  │
  ├─ MulBackward0(y).apply([1.0])
  │  └─ Output: [2.0] → AccumulateGrad(x)
  │
  ├─ MulBackward0(z).apply([1.0])
  │  └─ Output: [3.0] → AccumulateGrad(x)
  │
  ├─ AccumulateGrad(x).apply([2.0, 3.0])
  │  └─ InputBuffer accumulates: [2.0] + [3.0] = [5.0]
  │     x.grad = [5.0]
  │
  ✓ Gradients from both paths are summed!

┌─────────────────────────────────────────────────────────────────┐
│                      RESULT: x.grad = [5.0]                    │
└─────────────────────────────────────────────────────────────────┘
```

---

## 3. EXECUTION PRIORITY QUEUE

```
┌─────────────────────────────────────────────────────────────────┐
│                    READY QUEUE ORDERING                         │
└─────────────────────────────────────────────────────────────────┘

Priority Comparator:
  1. Shutdown tasks (highest)
  2. Lower reentrant_depth
  3. Higher sequence_nr
  4. Regular tasks (lowest)

Example: loss = (x * 3 + 2).sum()

Node Creation Order (sequence_nr):
  MulBackward0: sequence_nr = 1
  AddBackward0: sequence_nr = 2
  SumBackward0: sequence_nr = 3

Execution Order (reverse):
  1. SumBackward0 (seq_nr: 3) ← Highest priority
  2. AddBackward0 (seq_nr: 2)
  3. MulBackward0 (seq_nr: 1)
  4. AccumulateGrad(x) (seq_nr: 0)

Why reverse? Because later operations depend on earlier ones!
```

---

## 4. DEPENDENCY TRACKING

```
┌─────────────────────────────────────────────────────────────────┐
│              DEPENDENCY COMPUTATION ALGORITHM                   │
└─────────────────────────────────────────────────────────────────┘

Graph: x → MulBackward0 → y → AddBackward0 → z → SumBackward0 → loss

Step 1: Traverse from root (SumBackward0)
  ├─ Visit SumBackward0
  │  └─ dependencies[SumBackward0] = 1 (from GraphRoot)
  │
  ├─ Visit AddBackward0
  │  └─ dependencies[AddBackward0] = 1 (from SumBackward0)
  │
  ├─ Visit MulBackward0
  │  └─ dependencies[MulBackward0] = 1 (from AddBackward0)
  │
  └─ Visit AccumulateGrad(x)
     └─ dependencies[AccumulateGrad(x)] = 1 (from MulBackward0)

Step 2: Initialize not_ready map
  ├─ not_ready[SumBackward0] = InputBuffer()
  ├─ not_ready[AddBackward0] = InputBuffer()
  ├─ not_ready[MulBackward0] = InputBuffer()
  └─ not_ready[AccumulateGrad(x)] = InputBuffer()

Step 3: Queue ready nodes
  └─ ready_queue.push(GraphRoot)

Step 4: Execute
  While ready_queue not empty:
    node = ready_queue.pop()
    gradients = node.apply(not_ready[node])
    
    For each output_edge in node.next_edges:
      target_node = output_edge.function
      input_idx = output_edge.input_nr
      
      not_ready[target_node].add(input_idx, gradients)
      dependencies[target_node] -= 1
      
      If dependencies[target_node] == 0:
        ready_queue.push(target_node)
```

---

## 5. INPUT BUFFER ACCUMULATION

```
┌─────────────────────────────────────────────────────────────────┐
│              INPUT BUFFER GRADIENT ACCUMULATION                 │
└─────────────────────────────────────────────────────────────────┘

Example: y = x + x (x appears twice)

Forward Pass:
  x (leaf)
  │
  └─ AddBackward0 (two inputs, both x)

Backward Pass:
  SumBackward0.apply([1.0])
  │
  └─ AddBackward0.apply([1.0])
     │
     ├─ Input 0: [1.0]
     ├─ Input 1: [1.0]
     │
     └─ InputBuffer accumulation:
        ├─ buffer[0] = [1.0]
        ├─ buffer[1] = [1.0]
        │
        └─ When both inputs ready:
           ├─ gradient = buffer[0] + buffer[1]
           ├─ gradient = [1.0] + [1.0]
           └─ gradient = [2.0]
     │
     └─ MulBackward0.apply([2.0])
        │
        └─ AccumulateGrad(x).apply([2.0])
           └─ x.grad = [2.0]

Result: x.grad = [2.0] (both paths summed!)
```

---

## 6. SELECTIVE EXECUTION (.grad() vs .backward())

```
┌─────────────────────────────────────────────────────────────────┐
│                  SELECTIVE EXECUTION COMPARISON                 │
└─────────────────────────────────────────────────────────────────┘

Graph: x → MulBackward0 → y → AddBackward0 → z
       w → MulBackward0 → w_out ─┘

.backward() - Full Accumulation:
  ├─ Execute all nodes
  ├─ Accumulate into parameter.grad
  └─ Result: x.grad and w.grad both set

.grad(loss, x) - Selective Execution:
  ├─ Mark only nodes on path to x as needed
  ├─ Skip nodes not on path (e.g., w's MulBackward0)
  ├─ Return gradients without accumulating
  └─ Result: x.grad remains None, returns (grad_x,)

ExecInfo filtering:
  ├─ For each node:
  │  ├─ needed_ = True if on path to output
  │  ├─ captures_ = gradient capture info
  │  └─ should_execute() = needed_ || captures_
  │
  └─ Only execute nodes where should_execute() == True
```

---

## 7. SEQUENCE NUMBER ASSIGNMENT

```
┌─────────────────────────────────────────────────────────────────┐
│              SEQUENCE NUMBER ASSIGNMENT (Thread-Local)          │
└─────────────────────────────────────────────────────────────────┘

Thread-local counter: next_sequence_nr = 0

Forward Pass:
  x = quarisma.tensor([2.0], requires_grad=True)
  │
  y = x * 3
  │
  ├─ Create MulBackward0
  ├─ Assign sequence_nr = next_sequence_nr++
  ├─ sequence_nr = 1
  └─ next_sequence_nr = 2
  │
  z = y + 2
  │
  ├─ Create AddBackward0
  ├─ Assign sequence_nr = next_sequence_nr++
  ├─ sequence_nr = 2
  └─ next_sequence_nr = 3
  │
  loss = z.sum()
  │
  ├─ Create SumBackward0
  ├─ Assign sequence_nr = next_sequence_nr++
  ├─ sequence_nr = 3
  └─ next_sequence_nr = 4

Backward Pass:
  Execute in order: 3 → 2 → 1 → 0
  (Reverse topological order!)
```

---

## 8. TOPOLOGICAL NUMBER ASSIGNMENT

```
┌─────────────────────────────────────────────────────────────────┐
│              TOPOLOGICAL NUMBER ASSIGNMENT                      │
└─────────────────────────────────────────────────────────────────┘

Invariant: parent.topo_nr > all children.topo_nr

Forward Pass:
  x = quarisma.tensor([2.0], requires_grad=True)
  │
  y = x * 3
  │
  ├─ Create MulBackward0
  ├─ topo_nr = 0 (initial)
  │
  z = y + 2
  │
  ├─ Create AddBackward0
  ├─ topo_nr = 0 (initial)
  ├─ Update: topo_nr = max(children.topo_nr) + 1
  ├─ topo_nr = max(MulBackward0.topo_nr) + 1
  ├─ topo_nr = 0 + 1 = 1
  │
  loss = z.sum()
  │
  ├─ Create SumBackward0
  ├─ topo_nr = 0 (initial)
  ├─ Update: topo_nr = max(children.topo_nr) + 1
  ├─ topo_nr = max(AddBackward0.topo_nr) + 1
  ├─ topo_nr = 1 + 1 = 2

Final Topological Numbers:
  SumBackward0: topo_nr = 2 (highest)
  AddBackward0: topo_nr = 1
  MulBackward0: topo_nr = 0 (lowest)

Invariant satisfied: 2 > 1 > 0 ✓
```

---

## 9. KEY INSIGHTS SUMMARY

```
┌─────────────────────────────────────────────────────────────────┐
│                      KEY INSIGHTS                               │
└─────────────────────────────────────────────────────────────────┘

1. LAZY CONSTRUCTION
   ├─ Graph built during forward pass
   ├─ Nodes created on-demand
   └─ No execution until backward()

2. EAGER EXECUTION
   ├─ Backward pass executes immediately
   ├─ No compilation or optimization
   └─ Supports dynamic control flow

3. TOPOLOGICAL ORDERING
   ├─ Ensures correct gradient flow
   ├─ Parent > children in topo_nr
   └─ Reverse execution order

4. IMPLICIT SUMMATION
   ├─ Multiple paths automatically summed
   ├─ InputBuffer accumulates gradients
   └─ No manual gradient handling needed

5. SELECTIVE EXECUTION
   ├─ Only execute nodes on path to output
   ├─ Optimizes for specific gradients
   └─ Supports .grad() and .backward()

6. THREAD SAFETY
   ├─ Shared pointers for memory management
   ├─ Mutex protection for shared state
   └─ Thread-local sequence numbers

7. SCALABILITY
   ├─ Multi-threaded execution
   ├─ Stream synchronization for multi-GPU
   └─ Efficient memory usage

8. FLEXIBILITY
   ├─ Supports dynamic shapes
   ├─ Handles loops and conditionals
   └─ Enables research innovation
```

---

## 10. QUICK REFERENCE DIAGRAM

```
┌─────────────────────────────────────────────────────────────────┐
│                    COMPLETE SYSTEM OVERVIEW                     │
└─────────────────────────────────────────────────────────────────┘

FORWARD PASS (Graph Building)
  ↓
  Operation Execution
  ↓
  Create Backward Node
  ↓
  Store Input Metadata
  ↓
  Set next_edges
  ↓
  Set output.grad_fn
  ↓
  Graph Complete (Not Executed)

BACKWARD PASS (Graph Execution)
  ↓
  loss.backward()
  ↓
  Engine::execute()
  ↓
  Create GraphTask
  ↓
  Compute Dependencies
  ↓
  Initialize Ready Queue
  ↓
  While ready_queue not empty:
    ├─ Pop highest priority node
    ├─ Execute node.apply(gradients)
    ├─ Accumulate gradients in InputBuffer
    ├─ Decrement dependency count
    └─ Queue ready nodes
  ↓
  Return Gradients

RESULT
  ↓
  parameter.grad = accumulated_gradients
```

