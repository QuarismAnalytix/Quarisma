#pragma once

#include <Quarisma/core/jit_type.h>
#include <torch/csrc/jit/ir/ir.h>

namespace torch::jit
{

struct TORCH_API HashType
{
    size_t operator()(const TypePtr& type) const;
    size_t operator()(const quarisma::ConstTypePtr& type) const;
};

struct EqualType
{
    bool operator()(const TypePtr& a, const TypePtr& b) const;
    bool operator()(const quarisma::ConstTypePtr& a, const quarisma::ConstTypePtr& b) const;
};

}  // namespace torch::jit
