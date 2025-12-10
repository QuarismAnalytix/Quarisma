// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "smp/openmp/smp_thread_local_backend.h"

#include <omp.h>

#include <algorithm>
#include <cmath>  // For std::floor & std::log2

namespace conductor
{
namespace detail
{
namespace smp
{
namespace OpenMP
{

static thread_id_type get_thread_id()
{
    static int thread_private_data;
#pragma omp threadprivate(thread_private_data)
    return &thread_private_data;
}

// 32 bit FNV-1a hash function
inline hash_type get_hash(thread_id_type id)
{
    const hash_type offset_basis = 2166136261u;
    const hash_type FNV_prime    = 16777619u;

    unsigned char* bp   = reinterpret_cast<unsigned char*>(&id);
    unsigned char* be   = bp + sizeof(id);
    hash_type      hval = offset_basis;
    while (bp < be)
    {
        hval ^= static_cast<hash_type>(*bp++);
        hval *= FNV_prime;
    }

    return hval;
}

class lock_guard
{
public:
    lock_guard(omp_lock_t& lock, bool wait) : m_lock(lock), m_status(0)
    {
        if (wait)
        {
            omp_set_lock(&m_lock);
            m_status = 1;
        }
        else
        {
            m_status = omp_test_lock(&m_lock);
        }
    }

    bool success() const { return m_status != 0; }

    void release()
    {
        if (m_status)
        {
            omp_unset_lock(&m_lock);
            m_status = 0;
        }
    }

    ~lock_guard() { this->release(); }

private:
    // not copyable
    lock_guard(const lock_guard&);
    void operator=(const lock_guard&);

    omp_lock_t& m_lock;
    int         m_status;
};

slot::slot() : thread_id(0), storage(0)
{
    omp_init_lock(&this->modify_lock);
}

slot::~slot()
{
    omp_destroy_lock(&this->modify_lock);
}

hash_table_array::hash_table_array(size_t size_lg_param)
    : size(1u << size_lg_param),
      size_lg(size_lg_param),
      number_of_entries(0),
      slots(std::make_unique<slot[]>(1u << size_lg_param)),
      prev(nullptr)
{
}

hash_table_array::~hash_table_array() {}

// Recursively lookup the slot containing thread_id in the hash_table_array
// linked list -- array
static slot* lookup_slot(hash_table_array* array, thread_id_type thread_id, size_t hash)
{
    if (!array)
    {
        return nullptr;
    }

    size_t mask     = array->size - 1u;
    slot*  slot_ptr = nullptr;

    // since load factor is maintained below 0.5, this loop should hit an
    // empty slot if the queried slot does not exist in this array
    for (size_t idx = hash & mask;; idx = (idx + 1) & mask)  // linear probing
    {
        slot_ptr                      = array->slots.get() + idx;
        thread_id_type slot_thread_id = slot_ptr->thread_id.load();  // atomic read
        if (!slot_thread_id)  // empty slot means thread_id doesn't exist in this array
        {
            slot_ptr = lookup_slot(array->prev.get(), thread_id, hash);
            break;
        }
        else if (slot_thread_id == thread_id)
        {
            break;
        }
    }

    return slot_ptr;
}

// Lookup thread_id. Try to acquire a slot if it doesn't already exist.
// Does not block. Returns nullptr if acquire fails due to high load factor.
// Returns true in 'first_access' if thread_id did not exist previously.
static slot* acquire_slot(
    hash_table_array* array, thread_id_type thread_id, size_t hash, bool& first_access)
{
    size_t mask     = array->size - 1u;
    slot*  slot_ptr = nullptr;
    first_access    = false;

    for (size_t idx = hash & mask;; idx = (idx + 1) & mask)
    {
        slot_ptr                      = array->slots.get() + idx;
        thread_id_type slot_thread_id = slot_ptr->thread_id.load();  // atomic read
        if (!slot_thread_id)                                         // unused?
        {
            // empty slot means thread_id does not exist, try to acquire the slot
            lock_guard lguard(slot_ptr->modify_lock, false);  // try to get exclusive access
            if (lguard.success())
            {
                size_t size_val = ++array->number_of_entries;  // atomic
                if ((size_val * 2) > array->size)              // load factor is above threshold
                {
                    --array->number_of_entries;  // atomic revert
                    return nullptr;              // indicate need for resizing
                }

                if (!slot_ptr->thread_id.load())  // not acquired in the meantime?
                {
                    slot_ptr->thread_id.store(thread_id);  // atomically acquire
                    // check previous arrays for the entry
                    slot* prev_slot = lookup_slot(array->prev.get(), thread_id, hash);
                    if (prev_slot)
                    {
                        slot_ptr->storage = prev_slot->storage;
                        // Do not clear prev_slot's thread_id as our technique of stopping
                        // linear probing at empty slots relies on slots not being
                        // "freed". Instead, clear previous slot's storage pointer as
                        // thread_specific_storage_iterator relies on this information to
                        // ensure that it doesn't iterate over the same thread's storage
                        // more than once.
                        prev_slot->storage = nullptr;
                    }
                    else  // first time access
                    {
                        slot_ptr->storage = nullptr;
                        first_access      = true;
                    }
                    break;
                }
            }
        }
        else if (slot_thread_id == thread_id)
        {
            break;
        }
    }

    return slot_ptr;
}

thread_specific::thread_specific(unsigned num_threads)
    : m_root(nullptr), m_root_owner(nullptr), m_count(0)
{
    const int last_set_bit = (num_threads != 0 ? std::floor(std::log2(num_threads)) : 0);
    // initial size should be more than twice the number of threads
    const size_t init_size_lg = (last_set_bit + 2);
    m_root_owner              = std::make_unique<hash_table_array>(init_size_lg);
    m_root                    = m_root_owner.get();
}

thread_specific::~thread_specific()
{
    // Ownership is automatically managed by unique_ptr
}

storage_pointer_type& thread_specific::get_storage()
{
    thread_id_type thread_id = get_thread_id();
    size_t         hash      = get_hash(thread_id);

    slot* slot_ptr = nullptr;
    while (!slot_ptr)
    {
        bool              first_access = false;
        hash_table_array* array        = m_root.load();
        slot_ptr                       = acquire_slot(array, thread_id, hash, first_access);
        if (!slot_ptr)  // not enough room, resize
        {
#pragma omp critical(HashTableResize)
            if (m_root == array)
            {
                auto new_array  = std::make_unique<hash_table_array>(array->size_lg + 1);
                new_array->prev = std::move(m_root_owner);
                m_root_owner    = std::move(new_array);
                m_root.store(m_root_owner.get());  // atomic copy
            }
        }
        else if (first_access)
        {
            ++m_count;  // atomic increment
        }
    }
    return slot_ptr->storage;
}

}  // namespace OpenMP
}  // namespace smp
}  // namespace detail
}  // namespace conductor
