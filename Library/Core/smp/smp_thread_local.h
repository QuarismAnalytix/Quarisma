// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   smp_thread_local
 * @brief   A SMP-aware thread local object
 *
 * smp_thread_local provides a thread local storage independent of the backend
 * in use. It is used primarily by smp_tools classes.
 */

#ifndef SMP_THREAD_LOCAL_H
#define SMP_THREAD_LOCAL_H

#include "common/export.h"
#include "smp/common/smp_thread_local_api.h"

template <typename T>
class smp_thread_local
{
public:
  smp_thread_local()
    : m_internal()
  {
  }

  explicit smp_thread_local(const T& exemplar)
    : m_internal(exemplar)
  {
  }

  T& local()
  {
    return m_internal.local();
  }

  size_t size() const
  {
    return m_internal.size();
  }

  class iterator
  {
  public:
    iterator& operator++()
    {
      ++m_iter;
      return *this;
    }

    iterator operator++(int)
    {
      iterator copy = *this;
      ++m_iter;
      return copy;
    }

    bool operator==(const iterator& other) const
    {
      return m_iter == other.m_iter;
    }

    bool operator!=(const iterator& other) const
    {
      return m_iter != other.m_iter;
    }

    T& operator*()
    {
      return *m_iter;
    }

    T* operator->()
    {
      return &*m_iter;
    }

    iterator()
      : m_iter()
    {
    }

  private:
    friend class smp_thread_local<T>;
    typename conductor::detail::smp::smp_thread_local_api<T>::iterator m_iter;
  };

  iterator begin()
  {
    iterator iter;
    iter.m_iter = m_internal.begin();
    return iter;
  }

  iterator end()
  {
    iterator iter;
    iter.m_iter = m_internal.end();
    return iter;
  }

private:
  conductor::detail::smp::smp_thread_local_api<T> m_internal;

  // not copyable
  smp_thread_local(const smp_thread_local&) = delete;
  void operator=(const smp_thread_local&) = delete;
};

#endif
/* VTK-HeaderTest-Exclude: smp_thread_local.h */
