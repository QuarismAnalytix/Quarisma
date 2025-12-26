#include "smp/Advanced/thread_name.h"

#include <string>

#include "logging/logger.h"

namespace xsigma::detail::smp::Advanced
{

// Consolidated implementation: delegate to the central logger API
void set_thread_name(const std::string& name)
{
    xsigma::logger::SetThreadName(name);
}

std::string get_thread_name()
{
    auto name = xsigma::logger::GetThreadName();
    // Preserve previous semantics: return empty when unset/not supported
    if (name == "N/A")
    {
        return {};
    }
    return name;
}

}  // namespace xsigma::detail::smp::Advanced
