#include <Quarisma/TensorOperators.h>
#include <torch/csrc/jit/passes/fold_linear_bn.h>

#ifndef AT_PER_OPERATOR_HEADERS
#include <Quarisma/Functions.h>
#else
#include <Quarisma/ops/rsqrt.h>
#endif

namespace torch::jit
{

std::tuple<quarisma::Tensor, quarisma::Tensor> computeUpdatedLinearWeightAndBias(
    const LinearBNParameters& p)
{
    quarisma::Tensor bn_scale = p.bn_w * quarisma::rsqrt(p.bn_rv + p.bn_eps);
    quarisma::Tensor fused_w  = p.linear_w * bn_scale.unsqueeze(-1);
    quarisma::Tensor fused_b  = (p.linear_b - p.bn_rm) * bn_scale + p.bn_b;

    auto linear_w_dtype = p.linear_w.dtype();
    auto linear_b_dtype = p.linear_b.dtype();

    return std::make_tuple(fused_w.to(linear_w_dtype), fused_b.to(linear_b_dtype));
}

}  // namespace torch::jit
