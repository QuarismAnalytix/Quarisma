#pragma once

#include <pybind11/pybind11.h>
#include <quarisma/csrc/utils/pybind.h>
#include <quarisma/csrc/utils/python_numbers.h>

#include "util/strong_type.h"

namespace pybind11::detail
{
// Strong typedefs don't make much sense in Python since everything is duck
// typed. So instead we simply extract the underlying value and let the caller
// handle correctness.
template <typename T>
class QUARISMA_VISIBILITY strong_pointer_type_caster
{
    template <typename T_>
    static handle cast(const T_& src, return_value_policy /*policy*/, handle /*parent*/)
    {
        const auto* ptr = reinterpret_cast<const void*>(src.value_of());
        return ptr ? handle(THPUtils_packUInt64(reinterpret_cast<intptr_t>(ptr))) : none();
    }

    bool load(handle /*src*/, bool /*convert*/) { return false; }

    PYBIND11_TYPE_CASTER(T, _("strong_pointer"));
};

template <typename T>
class QUARISMA_VISIBILITY strong_uint_type_caster
{
    template <typename T_>
    static handle cast(const T_& src, return_value_policy /*policy*/, handle /*parent*/)
    {
        return handle(THPUtils_packUInt64(src.value_of()));
    }

    bool load(handle /*src*/, bool /*convert*/) { return false; }

    PYBIND11_TYPE_CASTER(T, _("strong_uint"));
};
}  // namespace pybind11::detail
