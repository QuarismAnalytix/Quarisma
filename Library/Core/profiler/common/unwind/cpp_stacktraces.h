#pragma once

#include "common/export.h"
#include "profiler/common/unwind/unwind.h"

namespace quarisma
{
QUARISMA_API bool get_cpp_stacktraces_enabled();
QUARISMA_API quarisma::unwind::Mode get_symbolize_mode();
}  // namespace quarisma
