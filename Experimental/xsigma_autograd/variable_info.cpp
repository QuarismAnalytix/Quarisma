#ifndef AT_PER_OPERATOR_HEADERS
#include <Quarisma/Functions.h>
#else
#include <Quarisma/ops/zeros.h>
#include <Quarisma/ops/zeros_like.h>
#endif

#include <torch/csrc/autograd/variable.h>
#include <torch/csrc/autograd/variable_info.h>

namespace torch::autograd
{

VariableInfo::VariableInfo(const Variable& var, bool use_zeros_like)
    : layout(var.layout()),
      device(var.device()),
      scalar_type(var.scalar_type()),
      size(var.sym_sizes().vec()),
      requires_grad(var.requires_grad()),
      is_empty(false),
      the_var(use_zeros_like ? std::optional<Variable>(var.detach()) : std::nullopt)
{
}

VariableInfo::VariableInfo() : requires_grad(false), is_empty(true) {}

Variable VariableInfo::zeros(quarisma::OptionalDeviceGuard& device_guard) const
{
    if (is_empty)
    {
        // Return undefined tensor.
        return quarisma::Tensor();
    }
    else if (the_var.has_value())
    {
        return quarisma::zeros_like(*the_var);
    }
    else
    {
        return quarisma::zeros_symint(
            size, quarisma::TensorOptions(scalar_type).device(device).layout(layout));
    }
}

}  // namespace torch::autograd
