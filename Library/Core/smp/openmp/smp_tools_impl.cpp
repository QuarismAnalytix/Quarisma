// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "smp/common/smp_tools_impl.h"
#include "smp/openmp/smp_tools_impl.h"

#include <cstdlib> // For std::getenv()
#include <omp.h>
#include <stack> // For std::stack
#include <string>
#include <charconv>

namespace conductor
{
namespace detail
{
namespace smp
{

static int specified_num_threads_omp; // Default initialized to zero
static std::stack<int>* thread_id_stack;

//------------------------------------------------------------------------------
// Must NOT be initialized. Default initialization to zero is necessary.
unsigned int smp_tools_impl_openmp_initialize_count;

//------------------------------------------------------------------------------
smp_tools_impl_openmp_initialize::smp_tools_impl_openmp_initialize()
{
  if (++smp_tools_impl_openmp_initialize_count == 1)
  {
    thread_id_stack = new std::stack<int>;
  }
}

//------------------------------------------------------------------------------
smp_tools_impl_openmp_initialize::~smp_tools_impl_openmp_initialize()
{
  if (--smp_tools_impl_openmp_initialize_count == 0)
  {
    delete thread_id_stack;
    thread_id_stack = nullptr;
  }
}

//------------------------------------------------------------------------------
template <>
void smp_tools_impl<backend_type::OpenMP>::initialize(int num_threads)
{
  const int max_threads = smp_tools_impl<backend_type::OpenMP>::get_estimated_default_number_of_threads();
  if (num_threads == 0)
  {
    const char* vtk_smp_num_threads = std::getenv("SMP_MAX_THREADS");
    if (vtk_smp_num_threads)
    {
      std::string str(vtk_smp_num_threads);
      auto result = std::from_chars(str.data(), str.data() + str.size(), num_threads);
      if (result.ec != std::errc())
      {
        num_threads = 0;
      }
    }
    else if (specified_num_threads_omp)
    {
      specified_num_threads_omp = 0;
      omp_set_num_threads(max_threads);
    }
  }
#pragma omp single
  if (num_threads > 0)
  {
    num_threads = std::min(num_threads, max_threads);
    specified_num_threads_omp = num_threads;
    omp_set_num_threads(num_threads);
  }
}

//------------------------------------------------------------------------------
int get_number_of_threads_openmp()
{
  return (specified_num_threads_omp > 0)
    ? specified_num_threads_omp
    : smp_tools_impl<backend_type::OpenMP>::get_estimated_default_number_of_threads();
}

//------------------------------------------------------------------------------
bool get_single_thread_openmp()
{
  return thread_id_stack->top() == omp_get_thread_num();
}

//------------------------------------------------------------------------------
template <>
int smp_tools_impl<backend_type::OpenMP>::get_estimated_number_of_threads()
{
  return get_number_of_threads_openmp();
}

//------------------------------------------------------------------------------
template <>
int smp_tools_impl<backend_type::OpenMP>::get_estimated_default_number_of_threads()
{
  return omp_get_max_threads();
}

//------------------------------------------------------------------------------
template <>
bool smp_tools_impl<backend_type::OpenMP>::get_single_thread()
{
  return get_single_thread_openmp();
}

//------------------------------------------------------------------------------
void smp_tools_impl_for_openmp(size_t first, size_t last, size_t grain,
  execute_functor_ptr_type functor_executer, void* functor, bool nested_activated)
{
  if (grain <= 0)
  {
    size_t estimate_grain = (last - first) / (get_number_of_threads_openmp() * 4);
    grain = (estimate_grain > 0) ? estimate_grain : 1;
  }

  omp_set_max_active_levels(nested_activated);

#pragma omp single
  thread_id_stack->emplace(omp_get_thread_num());

#pragma omp parallel for schedule(runtime)
  for (size_t from = first; from < last; from += grain)
  {
    functor_executer(functor, from, grain, last);
  }

#pragma omp single
  thread_id_stack->pop();
}

} // namespace smp
} // namespace detail
} // namespace conductor

