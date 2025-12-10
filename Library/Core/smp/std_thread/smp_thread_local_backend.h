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
//
// This implementation is the same as the OpenMP equivalent but with std::mutex
// and std::lock_guard instead of omp_lock_t and custom lock guard.

#ifndef STDTHREAD_SMP_THREAD_LOCAL_BACKEND_H
#define STDTHREAD_SMP_THREAD_LOCAL_BACKEND_H

#include <atomic>
#include <cstdint>  // For uint_fast32_t
#include <memory>   // For std::unique_ptr
#include <mutex>    // std::mutex, std::lock_guard
#include <thread>

#include "common/export.h"

namespace conductor
{
namespace detail
{
namespace smp
{
namespace std_thread
{

typedef size_t        thread_id_type;
typedef uint_fast32_t hash_type;
typedef void*         storage_pointer_type;

struct XSIGMA_VISIBILITY slot
{
    std::atomic<thread_id_type> thread_id;
    std::mutex                  mutex;
    storage_pointer_type        storage;

    slot();
    ~slot() = default;

private:
    // not copyable
    slot(const slot&);
    void operator=(const slot&);
};

struct XSIGMA_VISIBILITY hash_table_array
{
    size_t                            size, size_lg;
    std::atomic<size_t>               number_of_entries;
    std::unique_ptr<slot[]>           slots;
    std::unique_ptr<hash_table_array> prev;

    explicit hash_table_array(size_t size_lg_param);
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
    XSIGMA_API size_t                get_size() const;

private:
    std::atomic<hash_table_array*>    m_root;        // Non-owning pointer for atomic operations
    std::unique_ptr<hash_table_array> m_root_owner;  // Actual owner
    std::atomic<size_t>               m_size;
    std::mutex                        m_mutex;

    friend class thread_specific_storage_iterator;
};

class XSIGMA_VISIBILITY thread_specific_storage_iterator
{
public:
    thread_specific_storage_iterator()
        : m_thread_specific_storage(nullptr), m_current_array(nullptr), m_current_slot(0)
    {
    }

    XSIGMA_API void set_thread_specific_storage(thread_specific& thread_specific_obj)
    {
        m_thread_specific_storage = &thread_specific_obj;
    }

    XSIGMA_API void set_to_begin()
    {
        m_current_array = m_thread_specific_storage->m_root;
        m_current_slot  = 0;
        if (!m_current_array->slots[0].storage)
        {
            this->forward();
        }
    }

    XSIGMA_API void set_to_end()
    {
        m_current_array = nullptr;
        m_current_slot  = 0;
    }

    XSIGMA_API bool get_initialized() const { return m_thread_specific_storage != nullptr; }

    XSIGMA_API bool get_at_end() const { return m_current_array == nullptr; }

    XSIGMA_API void forward()
    {
        while (true)
        {
            if (++m_current_slot >= m_current_array->size)
            {
                m_current_array = m_current_array->prev.get();
                m_current_slot  = 0;
                if (!m_current_array)
                {
                    break;
                }
            }
            slot* slot_ptr = m_current_array->slots.get() + m_current_slot;
            if (slot_ptr->storage)
            {
                break;
            }
        }
    }

    XSIGMA_API storage_pointer_type& get_storage() const
    {
        slot* slot_ptr = m_current_array->slots.get() + m_current_slot;
        return slot_ptr->storage;
    }

    XSIGMA_API bool operator==(const thread_specific_storage_iterator& it) const
    {
        return (m_thread_specific_storage == it.m_thread_specific_storage) &&
               (m_current_array == it.m_current_array) && (m_current_slot == it.m_current_slot);
    }

private:
    thread_specific*  m_thread_specific_storage;
    hash_table_array* m_current_array;
    size_t            m_current_slot;
};

}  // namespace std_thread
}  // namespace smp
}  // namespace detail
}  // namespace conductor

#endif
/* VTK-HeaderTest-Exclude: INCLUDES:CLASSES */
