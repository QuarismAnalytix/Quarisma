#include "profiler/common/unwind/cpp_stacktraces.h"

#include <cstdlib>
#include <cstring>

#include "util/env.h"
#include "util/exception.h"

namespace xsigma
{
namespace
{
bool compute_cpp_stack_traces_enabled()
{
    return xsigma::utils::check_env("XSIGMA_SHOW_CPP_STACKTRACES") == true;
}

bool compute_disable_addr2line()
{
    return xsigma::utils::check_env("XSIGMA_DISABLE_ADDR2LINE") == true;
}
}  // namespace

bool get_cpp_stacktraces_enabled()
{
    static bool const enabled = compute_cpp_stack_traces_enabled();
    return enabled;
}

static xsigma::unwind::Mode compute_symbolize_mode()
{
    auto envar_c = xsigma::utils::get_env("XSIGMA_SYMBOLIZE_MODE");
    if (envar_c.has_value())
    {
        if (envar_c == "dladdr")
        {
            return unwind::Mode::dladdr;
        }
        if (envar_c == "addr2line")
        {
            return unwind::Mode::addr2line;
        }
        if (envar_c == "fast")
        {
            return unwind::Mode::fast;
        }

        XSIGMA_CHECK(
            false,
            "expected {{dladdr, addr2line, fast}} for XSIGMA_SYMBOLIZE_MODE, got {}",
            envar_c.value());
    }
    else
    {
        return compute_disable_addr2line() ? unwind::Mode::dladdr : unwind::Mode::addr2line;
    }
}

unwind::Mode get_symbolize_mode()
{
    static unwind::Mode const mode = compute_symbolize_mode();
    return mode;
}

}  // namespace xsigma
