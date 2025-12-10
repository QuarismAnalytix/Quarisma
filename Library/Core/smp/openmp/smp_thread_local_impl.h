// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME smp_thread_local_impl - A thread local storage implementation using
// platform specific facilities.

#ifndef OPENMP_SMP_THREAD_LOCAL_IMPL_H
#define OPENMP_SMP_THREAD_LOCAL_IMPL_H

#include "smp/common/smp_thread_local_impl_abstract.h"
#include "smp/openmp/smp_thread_local_backend.h"
#include "smp/openmp/smp_tools_impl.h"

#include <iterator>
#include <utility> // For std::move

namespace conductor
{
namespace detail
{
namespace smp
{

template <typename T>
class smp_thread_local_impl<backend_type::OpenMP, T> : public smp_thread_local_impl_abstract<T>
{
  typedef typename smp_thread_local_impl_abstract<T>::it_impl it_impl_abstract;

public:
  smp_thread_local_impl()
    : m_backend(get_number_of_threads_openmp())
  {
  }

  explicit smp_thread_local_impl(const T& exemplar)
    : m_backend(get_number_of_threads_openmp())
    , m_exemplar(exemplar)
  {
  }

  ~smp_thread_local_impl() override
  {
    OpenMP::thread_specific_storage_iterator it;
    it.set_thread_specific_storage(m_backend);
    for (it.set_to_begin(); !it.get_at_end(); it.forward())
    {
      delete reinterpret_cast<T*>(it.get_storage());
    }
  }

  T& local() override
  {
    OpenMP::storage_pointer_type& ptr = m_backend.get_storage();
    T* local_ptr = reinterpret_cast<T*>(ptr);
    if (!ptr)
    {
      ptr = local_ptr = new T(m_exemplar);
    }
    return *local_ptr;
  }

  size_t size() const override { return m_backend.get_size(); }

  class it_impl : public smp_thread_local_impl_abstract<T>::it_impl
  {
  public:
    void increment() override { m_impl.forward(); }

    bool compare(it_impl_abstract* other) override
    {
      return m_impl == static_cast<it_impl*>(other)->m_impl;
    }

    T& get_content() override { return *reinterpret_cast<T*>(m_impl.get_storage()); }

    T* get_content_ptr() override { return reinterpret_cast<T*>(m_impl.get_storage()); }

  protected:
    it_impl* clone_impl() const override { return new it_impl(*this); };

  private:
    OpenMP::thread_specific_storage_iterator m_impl;

    friend class smp_thread_local_impl<backend_type::OpenMP, T>;
  };

  std::unique_ptr<it_impl_abstract> begin() override
  {
    auto it = std::make_unique<it_impl>();
    it->m_impl.set_thread_specific_storage(m_backend);
    it->m_impl.set_to_begin();
    return it;
  }

  std::unique_ptr<it_impl_abstract> end() override
  {
    auto it = std::make_unique<it_impl>();
    it->m_impl.set_thread_specific_storage(m_backend);
    it->m_impl.set_to_end();
    return it;
  }

private:
  OpenMP::thread_specific m_backend;
  T m_exemplar;

  // disable copying
  smp_thread_local_impl(const smp_thread_local_impl&) = delete;
  void operator=(const smp_thread_local_impl&) = delete;
};

} // namespace smp
} // namespace detail
} // namespace conductor

#endif
/* VTK-HeaderTest-Exclude: smp_thread_local_impl.h */

