#pragma once

#include "common/macros.h"

namespace quarisma
{
class QUARISMA_VISIBILITY cpu_info
{
    QUARISMA_DELETE_CLASS(cpu_info);

public:
    QUARISMA_API static void info();

    QUARISMA_API static void cpuinfo_cach(
        std::ptrdiff_t& l1, std::ptrdiff_t& l2, std::ptrdiff_t& l3, std::ptrdiff_t& l3_count);
};
}  // namespace quarisma