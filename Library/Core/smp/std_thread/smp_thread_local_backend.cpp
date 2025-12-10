// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "smp/std_thread/smp_thread_local_backend.h"

#include <algorithm>
#include <cmath>       // For std::floor & std::log2
#include <functional>  // For std::hash
#include <thread>      // For std::thread

#include "smp/std_thread/smp_thread_pool.h"

namespace conductor
{
namespace detail
{
namespace smp
{
namespace std_thread
{

static thread_id_type get_thread_id()
{
    return smp_thread_pool::get_instance().get_thread_id();
}

// 32 bit FNV-1a hash function
inline hash_type get_hash(thread_id_type id)
{
    constexpr hash_type offset_basis = 2166136261u;
    constexpr hash_type FNV_prime    = 16777619u;

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

slot::slot() : thread_id(0), storage(nullptr) {}

hash_table_array::hash_table_array(size_t size_lg_param)
    : size(1ULL << size_lg_param),
      size_lg(size_lg_param),
      number_of_entries(0),
      slots(std::make_unique<slot[]>(1ULL << size_lg_param)),
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
        thread_id_type slot_thread_id = slot_ptr->thread_id.load();
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
// Returns true in 'first_access' if threadID did not exist previously.
static slot* acquire_slot(
    hash_table_array* array, thread_id_type thread_id, size_t hash, bool& first_access)
{
    size_t mask     = array->size - 1u;
    slot*  slot_ptr = nullptr;
    first_access    = false;

    for (size_t idx = hash & mask;; idx = (idx + 1) & mask)
    {
        slot_ptr                      = array->slots.get() + idx;
        thread_id_type slot_thread_id = slot_ptr->thread_id.load();
        if (!slot_thread_id)  // unused?
        {
            std::lock_guard<std::mutex> lguard(slot_ptr->mutex);

            size_t size_val = array->number_of_entries++;
            if ((size_val * 2) > array->size)  // load factor is above threshold
            {
                --array->number_of_entries;
                return nullptr;  // indicate need for resizing
            }

            if (!slot_ptr->thread_id.load())  // not acquired in the meantime?
            {
                slot_ptr->thread_id.store(thread_id);
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
        else if (slot_thread_id == thread_id)
        {
            break;
        }
    }

    return slot_ptr;
}

thread_specific::thread_specific(unsigned num_threads)
    : m_root(nullptr), m_root_owner(nullptr), m_size(0)
{
    const int    last_set_bit = (num_threads != 0 ? std::floor(std::log2(num_threads)) : 0);
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
            std::lock_guard<std::mutex> lguard(m_mutex);

            if (m_root == array)
            {
                auto new_array  = std::make_unique<hash_table_array>(array->size_lg + 1);
                new_array->prev = std::move(m_root_owner);
                m_root_owner    = std::move(new_array);
                m_root.store(m_root_owner.get());
            }
        }
        else if (first_access)
        {
            m_size++;
        }
    }
    return slot_ptr->storage;
}

size_t thread_specific::get_size() const
{
    return m_size;
}

}  // namespace std_thread
}  // namespace smp
}  // namespace detail
}  // namespace conductor
