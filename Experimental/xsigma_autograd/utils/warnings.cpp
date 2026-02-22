#include <torch/csrc/autograd/utils/warnings.h>

namespace torch::autograd::utils
{

void DelayWarningHandler::process(const quarisma::Warning& warning)
{
    std::lock_guard<std::mutex> lock(mutex_);
    warnings_.push_back(warning);
}

void DelayWarningHandler::replay_warnings()
{
    std::lock_guard<std::mutex> lock(mutex_);
    TORCH_INTERNAL_ASSERT(
        quarisma::WarningUtils::get_warning_handler() != this,
        "DelayWarningHandler cannot replay warnings into itself, this will cause a deadlock");
    for (const auto& warning : warnings_)
    {
        quarisma::warn(warning);
    }
}

}  // namespace torch::autograd::utils
