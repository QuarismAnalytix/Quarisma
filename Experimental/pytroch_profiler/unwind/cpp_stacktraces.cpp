#include "cpp_stacktraces.h"

#include <cstdlib>
#include <cstring>

#include "util/env.h"
#include "util/exception.h"

namespace quarisma
{
namespace
{
bool compute_cpp_stack_traces_enabled()
{
    return quarisma::utils::check_env("QUARISMA_SHOW_CPP_STACKTRACES") == true;
}

bool compute_disable_addr2line()
{
    return quarisma::utils::check_env("QUARISMA_DISABLE_ADDR2LINE") == true;
}
}  // namespace

bool get_cpp_stacktraces_enabled()
{
    static bool enabled = compute_cpp_stack_traces_enabled();
    return enabled;
}

static quarisma::unwind::Mode compute_symbolize_mode()
{
    auto envar_c = quarisma::utils::get_env("QUARISMA_SYMBOLIZE_MODE");
    if (envar_c.has_value())
    {
        if (envar_c == "dladdr")
        {
            return unwind::Mode::dladdr;
        }
        else if (envar_c == "addr2line")
        {
            return unwind::Mode::addr2line;
        }
        else if (envar_c == "fast")
        {
            return unwind::Mode::fast;
        }
        else
        {
            QUARISMA_CHECK(
                false,
                "expected {dladdr, addr2line, fast} for QUARISMA_SYMBOLIZE_MODE, got ",
                envar_c.value());
        }
    }
    else
    {
        return compute_disable_addr2line() ? unwind::Mode::dladdr : unwind::Mode::addr2line;
    }
}

unwind::Mode get_symbolize_mode()
{
    static unwind::Mode mode = compute_symbolize_mode();
    return mode;
}

}  // namespace quarisma
