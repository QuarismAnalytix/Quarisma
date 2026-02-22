#pragma once

#include <Quarisma/core/grad_mode.h>
#include <torch/csrc/Export.h>

namespace torch::autograd
{

using GradMode     = quarisma::GradMode;
using AutoGradMode = quarisma::AutoGradMode;

}  // namespace torch::autograd
