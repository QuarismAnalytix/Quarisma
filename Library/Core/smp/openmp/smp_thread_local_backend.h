// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Thread Specific Storage is implemented as a Hash Table, with the Thread Id
// as the key and a Pointer to the data as the value. The Hash Table implements
// Open Addressing with Linear Probing. A fixed-size array (hash_table_array) is
// used as the hash table. The size of this array is allocated to be large
// enough to store thread specific data for all the threads with a Load Factor
// of 0.5. In case the number of threads changes dynamically and the current
// array is not able to accommodate more entries, a new array is allocated that
// is twice the size of the current array. To avoid rehashing and blocking the
// threads, a rehash is not performed immediately. Instead, a linked list of
// hash table arrays is maintained with the current array at the root and older
// arrays along the list. All lookups are sequentially performed along the
// linked list. If the root array does not have an entry, it is created for
// faster lookup next time. The thread_specific::get_storage() function is thread
// safe and only blocks when a new array needs to be allocated, which should be
// rare.

#ifndef OPENMP_SMP_THREAD_LOCAL_BACKEND_H
#define OPENMP_SMP_THREAD_LOCAL_BACKEND_H

#include "common/export.h"

#include <atomic>
#include <cstdint>
#include <omp.h>

namespace conductor
{
namespace detail
{
namespace smp
{
namespace OpenMP
{

typedef void* thread_id_type;
typedef uint32_t hash_type;
typedef void* storage_pointer_type;

struct XSIGMA_VISIBILITY slot
{
  std::atomic<thread_id_type> thread_id;
  omp_lock_t modify_lock;
  storage_pointer_type storage;

  slot();
  ~slot();

private:
  // not copyable
  slot(const slot&);
  void operator=(const slot&);
};

struct XSIGMA_VISIBILITY hash_table_array
{
  size_t size, size_lg;
  std::atomic<size_t> number_of_entries;
  slot* slots;
  hash_table_array* prev;

  explicit hash_table_array(size_t size_lg);
  ~hash_table_array();

private:
  // disallow copying
  hash_table_array(const hash_table_array&);
  void operator=(const hash_table_array&);
};

class XSIGMA_VISIBILITY thread_specific final
{
public:
  explicit thread_specific(unsigned num_threads);
  ~thread_specific();

  XSIGMA_API storage_pointer_type& get_storage();
  XSIGMA_API size_t get_size() const;

private:
  std::atomic<hash_table_array*> m_root;
  std::atomic<size_t> m_count;

  friend class thread_specific_storage_iterator;
};

inline size_t thread_specific::get_size() const
{
  return m_count;
}

class XSIGMA_VISIBILITY thread_specific_storage_iterator
{
public:
  thread_specific_storage_iterator()
    : m_thread_specific_storage(nullptr)
    , m_current_array(nullptr)
    , m_current_slot(0)
  {
  }

  XSIGMA_API void set_thread_specific_storage(thread_specific& thread_specific_obj)
  {
    m_thread_specific_storage = &thread_specific_obj;
  }

  XSIGMA_API void set_to_begin()
  {
    m_current_array = m_thread_specific_storage->m_root;
    m_current_slot = 0;
    if (!m_current_array->slots->storage)
    {
      this->forward();
    }
  }

  XSIGMA_API void set_to_end()
  {
    m_current_array = nullptr;
    m_current_slot = 0;
  }

  XSIGMA_API bool get_initialized() const { return m_thread_specific_storage != nullptr; }

  XSIGMA_API bool get_at_end() const { return m_current_array == nullptr; }

  XSIGMA_API void forward()
  {
    for (;;)
    {
      if (++m_current_slot >= m_current_array->size)
      {
        m_current_array = m_current_array->prev;
        m_current_slot = 0;
        if (!m_current_array)
        {
          break;
        }
      }
      slot* slot_ptr = m_current_array->slots + m_current_slot;
      if (slot_ptr->storage)
      {
        break;
      }
    }
  }

  XSIGMA_API storage_pointer_type& get_storage() const
  {
    slot* slot_ptr = m_current_array->slots + m_current_slot;
    return slot_ptr->storage;
  }

  XSIGMA_API bool operator==(const thread_specific_storage_iterator& it) const
  {
    return (m_thread_specific_storage == it.m_thread_specific_storage) &&
      (m_current_array == it.m_current_array) && (m_current_slot == it.m_current_slot);
  }

private:
  thread_specific* m_thread_specific_storage;
  hash_table_array* m_current_array;
  size_t m_current_slot;
};

} // namespace OpenMP
} // namespace smp
} // namespace detail
} // namespace conductor

#endif
/* VTK-HeaderTest-Exclude: smp_thread_local_backend.h */


