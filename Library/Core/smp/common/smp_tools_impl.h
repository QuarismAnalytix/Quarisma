// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef SMP_TOOLS_IMPL_H
#define SMP_TOOLS_IMPL_H

#include "common/export.h"
#include "smp/smp.h"

#include <atomic>


namespace conductor
{
namespace detail
{
namespace smp
{

template <backend_type Backend>
class smp_tools_impl
{
public:
  //--------------------------------------------------------------------------------
  void initialize(int num_threads = 0);

  //--------------------------------------------------------------------------------
  int get_estimated_number_of_threads();

  //--------------------------------------------------------------------------------
  static int get_estimated_default_number_of_threads();

  //--------------------------------------------------------------------------------
  void set_nested_parallelism(bool is_nested);

  //--------------------------------------------------------------------------------
  bool get_nested_parallelism();

  //--------------------------------------------------------------------------------
  bool is_parallel_scope();

  //--------------------------------------------------------------------------------
  bool get_single_thread();

  //--------------------------------------------------------------------------------
  template <typename FunctorInternal>
  void For(size_t first, size_t last, size_t grain, FunctorInternal& fi);

  //--------------------------------------------------------------------------------
  smp_tools_impl();

  //--------------------------------------------------------------------------------
  smp_tools_impl(const smp_tools_impl& other);

  //--------------------------------------------------------------------------------
  void operator=(const smp_tools_impl& other);

private:
  bool m_nested_activated = false;
  std::atomic<bool> m_is_parallel{ false };
};

template <backend_type Backend>
void smp_tools_impl<Backend>::set_nested_parallelism(bool is_nested)
{
  m_nested_activated = is_nested;
}

template <backend_type Backend>
bool smp_tools_impl<Backend>::get_nested_parallelism()
{
  return m_nested_activated;
}

template <backend_type Backend>
bool smp_tools_impl<Backend>::is_parallel_scope()
{
  return m_is_parallel;
}

template <backend_type Backend>
smp_tools_impl<Backend>::smp_tools_impl()
  : m_nested_activated(true)
  , m_is_parallel(false)
{
}

template <backend_type Backend>
smp_tools_impl<Backend>::smp_tools_impl(const smp_tools_impl& other)
  : m_nested_activated(other.m_nested_activated)
  , m_is_parallel(other.m_is_parallel.load())
{
}

template <backend_type Backend>
void smp_tools_impl<Backend>::operator=(const smp_tools_impl& other)
{
  m_nested_activated = other.m_nested_activated;
  m_is_parallel = other.m_is_parallel.load();
}

using execute_functor_ptr_type = void (*)(void*, size_t, size_t, size_t);

} // namespace smp
} // namespace detail
} // namespace conductor

#endif
/* VTK-HeaderTest-Exclude: smp_tools_impl.h */
