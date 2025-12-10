// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef STDTHREAD_SMP_TOOLS_IMPL_H
#define STDTHREAD_SMP_TOOLS_IMPL_H

#include <algorithm>   // For std::sort
#include <functional>  // For std::bind

#include "common/export.h"
#include "smp/common/smp_tools_impl.h"
#include "smp/std_thread/smp_thread_pool.h"  // For smp_thread_pool

namespace xsigma
{
namespace detail
{
namespace smp
{

int XSIGMA_API number_of_threads_stdthread();

//--------------------------------------------------------------------------------
template <>
template <typename FunctorInternal>
void smp_tools_impl<backend_type::std_thread>::parallel_for(
    size_t first, size_t last, size_t grain, FunctorInternal& fi)
{
    size_t n = last - first;
    if (n <= 0)
    {
        return;
    }

    if (grain >= n || (!m_nested_activated && smp_thread_pool::instance().is_parallel_scope()))
    {
        fi.Execute(first, last);
    }
    else
    {
        int thread_number = number_of_threads_stdthread();

        if (grain <= 0)
        {
            size_t estimate_grain = (last - first) / (thread_number * 4);
            grain                 = (estimate_grain > 0) ? estimate_grain : 1;
        }

        auto proxy = smp_thread_pool::instance().allocate_threads(thread_number);

        for (size_t from = first; from < last; from += grain)
        {
            const auto to = (std::min)(from + grain, last);
            proxy.do_job([&fi, from, to] { fi.Execute(from, to); });
        }

        proxy.join();
    }
}

//--------------------------------------------------------------------------------
template <>
XSIGMA_API void smp_tools_impl<backend_type::std_thread>::initialize(int);

//--------------------------------------------------------------------------------
template <>
XSIGMA_API int smp_tools_impl<backend_type::std_thread>::estimated_number_of_threads();

//--------------------------------------------------------------------------------
template <>
XSIGMA_API int smp_tools_impl<backend_type::std_thread>::estimated_default_number_of_threads();

//--------------------------------------------------------------------------------
template <>
XSIGMA_API bool smp_tools_impl<backend_type::std_thread>::single_thread();

//--------------------------------------------------------------------------------
template <>
XSIGMA_API bool smp_tools_impl<backend_type::std_thread>::is_parallel_scope();

}  // namespace smp
}  // namespace detail
}  // namespace xsigma

#endif
