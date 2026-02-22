#pragma once

#include <torch/csrc/Export.h>
#include <quarisma/core/InferenceMode.h>

namespace torch::autograd
{

using InferenceMode = quarisma::InferenceMode;

}
