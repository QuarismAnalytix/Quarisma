#pragma once

#include <cstdint>
#include <iosfwd>

#include "common/macros.h"

namespace quarisma
{
enum class device_enum : int16_t
{
    CPU         = 0,
    CUDA        = 1,
    HIP         = 2,
    PrivateUse1 = 3
};

QUARISMA_API std::ostream& operator<<(std::ostream& str, device_enum const& s);

class QUARISMA_VISIBILITY device_option
{
public:
    using int_t = int16_t;

    QUARISMA_API device_option(device_enum type, device_option::int_t index);

    QUARISMA_API device_option(device_enum type, int index);

    QUARISMA_API bool operator==(const quarisma::device_option& rhs) const noexcept;

    QUARISMA_API int_t index() const noexcept;

    QUARISMA_API device_enum type() const noexcept;

private:
    int_t       index_ = -1;
    device_enum type_{};
};

QUARISMA_API std::ostream& operator<<(std::ostream& str, quarisma::device_option const& s);
}  // namespace quarisma
