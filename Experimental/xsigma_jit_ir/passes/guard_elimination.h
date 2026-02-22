#pragma once

#include <Quarisma/Quarisma.h>
#include <Quarisma/core/ivalue.h>
#include <Quarisma/core/jit_type.h>
#include <Quarisma/core/stack.h>
#include <torch/csrc/Export.h>
#include <torch/csrc/jit/ir/ir.h>

#include <list>
#include <vector>

namespace torch::jit
{

TORCH_API void EliminateRedundantGuards(std::shared_ptr<Graph> graph);

}  // namespace torch::jit
