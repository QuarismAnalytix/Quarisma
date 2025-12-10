// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef OPENMP_SMP_TOOLS_IMPL_H
#define OPENMP_SMP_TOOLS_IMPL_H

#include <algorithm>  // For std::sort

#include "common/export.h"
#include "smp/common/smp_tools_impl.h"

namespace xsigma
{
namespace detail
{
namespace smp
{

int XSIGMA_API  number_of_threads_openmp();
bool XSIGMA_API single_thread_openmp();
void XSIGMA_API smp_tools_impl_for_openmp(
    size_t                   first,
    size_t                   last,
    size_t                   grain,
    execute_functor_ptr_type functor_executer,
    void*                    functor,
    bool                     nested_activated);

//------------------------------------------------------------------------------
// Address the static initialization order 'fiasco' by implementing
// the schwarz counter idiom.
class XSIGMA_VISIBILITY smp_tools_impl_openmp_initialize
{
public:
    smp_tools_impl_openmp_initialize();
    ~smp_tools_impl_openmp_initialize();
};

//--------------------------------------------------------------------------------
// This instance will show up in any translation unit that uses smp_tools_impl.
// It will make sure smp_tools_impl statics are initialized before there are used
// and finalized when they are done being used.
static smp_tools_impl_openmp_initialize smp_tools_impl_openmp_initializer;

//--------------------------------------------------------------------------------
template <typename FunctorInternal>
void execute_functor_openmp(void* functor, size_t from, size_t grain, size_t last)
{
    const size_t to = std::min(from + grain, last);

    FunctorInternal& fi = *reinterpret_cast<FunctorInternal*>(functor);
    fi.Execute(from, to);
}

//--------------------------------------------------------------------------------
template <>
template <typename FunctorInternal>
void smp_tools_impl<backend_type::OpenMP>::parallel_for(
    size_t first, size_t last, size_t grain, FunctorInternal& fi)
{
    size_t n = last - first;
    if (n <= 0)
    {
        return;
    }

    if (grain >= n)
    {
        fi.Execute(first, last);
    }
    else
    {
        // /!\ This behaviour should be changed if we want more control on nested
        // (e.g only the 2 first nested For are in parallel)
        bool from_parallel_code = m_is_parallel.exchange(true);

        smp_tools_impl_for_openmp(
            first, last, grain, execute_functor_openmp<FunctorInternal>, &fi, m_nested_activated);

        // Atomic contortion to achieve m_is_parallel &= from_parallel_code.
        // This compare&exchange basically boils down to:
        // if (m_is_parallel == true_flag)
        //   m_is_parallel = from_parallel_code;
        // else
        //   true_flag = m_is_parallel;
        // Which either leaves m_is_parallel as false or sets it to from_parallel_code (i.e. &=).
        // Note that the return value of compare_exchange_weak() is not needed,
        // and that no looping is necessary.
        bool true_flag = true;
        m_is_parallel.compare_exchange_weak(true_flag, from_parallel_code);
    }
}

//--------------------------------------------------------------------------------
template <>
XSIGMA_API void smp_tools_impl<backend_type::OpenMP>::initialize(int);

//--------------------------------------------------------------------------------
template <>
XSIGMA_API int smp_tools_impl<backend_type::OpenMP>::estimated_number_of_threads();

//--------------------------------------------------------------------------------
template <>
XSIGMA_API int smp_tools_impl<backend_type::OpenMP>::estimated_default_number_of_threads();

//--------------------------------------------------------------------------------
template <>
XSIGMA_API bool smp_tools_impl<backend_type::OpenMP>::single_thread();

}  // namespace smp
}  // namespace detail
}  // namespace xsigma

#endif
