// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef TBB_SMP_TOOLS_IMPL_H
#define TBB_SMP_TOOLS_IMPL_H

#include "smp/common/smp_tools_impl.h"
#include "smp/common/smp_tools_internal.h" // For common conductor smp class
#include "common/export.h"

#ifdef _MSC_VER
#pragma push_macro("__TBB_NO_IMPLICIT_LINKAGE")
#define __TBB_NO_IMPLICIT_LINKAGE 1
#endif

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_sort.h>

#ifdef _MSC_VER
#pragma pop_macro("__TBB_NO_IMPLICIT_LINKAGE")
#endif

namespace conductor
{
namespace detail
{
namespace smp
{

void XSIGMA_API smp_tools_impl_for_tbb(size_t first, size_t last, size_t grain,
  execute_functor_ptr_type functor_executer, void* functor);

//------------------------------------------------------------------------------
// Address the static initialization order 'fiasco' by implementing
// the schwarz counter idiom.
class XSIGMA_VISIBILITY smp_tools_impl_tbb_initialize
{
public:
  smp_tools_impl_tbb_initialize();
  ~smp_tools_impl_tbb_initialize();
};

//--------------------------------------------------------------------------------
// This instance will show up in any translation unit that uses smp_tools_impl.
// It will make sure smp_tools_impl statics are initialized before there are used
// and finalized when they are done being used.
static smp_tools_impl_tbb_initialize smp_tools_impl_tbb_initializer;

//--------------------------------------------------------------------------------
template <typename T>
class func_call
{
  T& m_obj;

  void operator=(const func_call&) = delete;

public:
  void operator()(const tbb::blocked_range<size_t>& r) const { m_obj.Execute(r.begin(), r.end()); }

  func_call(T& obj)
    : m_obj(obj)
  {
  }
};

//--------------------------------------------------------------------------------
template <typename FunctorInternal>
void execute_functor_tbb(void* functor, size_t first, size_t last, size_t grain)
{
  FunctorInternal& fi = *reinterpret_cast<FunctorInternal*>(functor);

  size_t range = last - first;
  if (range <= 0)
  {
    return;
  }
  if (grain > 0)
  {
    tbb::parallel_for(
      tbb::blocked_range<size_t>(first, last, grain), func_call<FunctorInternal>(fi));
  }
  else
  {
    // When the grain is not specified, automatically calculate an appropriate grain size so
    // most of the time will still be spent running the calculation and not task overhead.

    // Estimate of how many threads we might be able to run
    const size_t number_threads_estimate = 40;
    // Plan for a few batches per thread so one busy core doesn't stall the whole system
    const size_t batches_per_thread = 5;
    const size_t batches = number_threads_estimate * batches_per_thread;

    if (range >= batches)
    {
      // std::ceil round up for systems without cmath
      size_t calculated_grain = ((range - 1) / batches) + 1;
      tbb::parallel_for(
        tbb::blocked_range<size_t>(first, last, calculated_grain), func_call<FunctorInternal>(fi));
    }
    else
    {
      // Data is too small to generate a reasonable grain. Fallback to default so data still runs
      // on as many threads as possible (Jan 2020: Default is one index per tbb task).
      tbb::parallel_for(tbb::blocked_range<size_t>(first, last), func_call<FunctorInternal>(fi));
    }
  }
}

//--------------------------------------------------------------------------------
template <>
template <typename FunctorInternal>
void smp_tools_impl<backend_type::TBB>::For(
  size_t first, size_t last, size_t grain, FunctorInternal& fi)
{
  if (!m_nested_activated && m_is_parallel)
  {
    fi.Execute(first, last);
  }
  else
  {
    // /!\ This behaviour should be changed if we want more control on nested
    // (e.g only the 2 first nested For are in parallel)
    bool from_parallel_code = m_is_parallel.exchange(true);

    smp_tools_impl_for_tbb(first, last, grain, execute_functor_tbb<FunctorInternal>, &fi);

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
XSIGMA_API void smp_tools_impl<backend_type::TBB>::initialize(int);

//--------------------------------------------------------------------------------
template <>
XSIGMA_API smp_tools_impl<backend_type::TBB>::smp_tools_impl();

//--------------------------------------------------------------------------------
template <>
XSIGMA_API int smp_tools_impl<backend_type::TBB>::get_estimated_default_number_of_threads();

//--------------------------------------------------------------------------------
template <>
XSIGMA_API int smp_tools_impl<backend_type::TBB>::get_estimated_number_of_threads();

//--------------------------------------------------------------------------------
template <>
XSIGMA_API bool smp_tools_impl<backend_type::TBB>::get_single_thread();

} // namespace smp
} // namespace detail
} // namespace conductor

#endif


