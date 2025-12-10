// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef SMP_THREAD_LOCAL_IMPL_ABSTRACT_H
#define SMP_THREAD_LOCAL_IMPL_ABSTRACT_H

#include <memory>

#include "smp/common/smp_tools_impl.h"

namespace conductor
{
namespace detail
{
namespace smp
{

template <typename T>
class smp_thread_local_impl_abstract
{
public:
  virtual ~smp_thread_local_impl_abstract() = default;

  virtual T& local() = 0;

  virtual size_t size() const = 0;

  class it_impl
  {
  public:
    it_impl() = default;
    virtual ~it_impl() = default;
    it_impl(const it_impl&) = default;
    it_impl(it_impl&&) noexcept = default;
    it_impl& operator=(const it_impl&) = default;
    it_impl& operator=(it_impl&&) noexcept = default;

    virtual void increment() = 0;

    virtual bool compare(it_impl* other) = 0;

    virtual T& get_content() = 0;

    virtual T* get_content_ptr() = 0;

    std::unique_ptr<it_impl> clone() const { return std::unique_ptr<it_impl>(clone_impl()); }

  protected:
    virtual it_impl* clone_impl() const = 0;
  };

  virtual std::unique_ptr<it_impl> begin() = 0;

  virtual std::unique_ptr<it_impl> end() = 0;
};

template <backend_type Backend, typename T>
class smp_thread_local_impl : public smp_thread_local_impl_abstract<T>
{
};

} // namespace smp
} // namespace detail
} // namespace conductor

#endif
/* VTK-HeaderTest-Exclude: smp_thread_local_impl_abstract.h */
