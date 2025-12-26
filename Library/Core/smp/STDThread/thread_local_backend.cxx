

// This file incorporates code from the Visualization Toolkit (VTK) and remains subject to the BSD-3-Clause VTK license.

#include "smp/STDThread/thread_local_backend.h"

#include <cmath>  // For std::floor & std::log2
#include <cstddef>
#include <mutex>

#include "smp/STDThread/thread_pool.h"

namespace xsigma::detail::smp::STDThread
{

static ThreadIdType GetThreadId()
{
    return thread_pool::GetInstance().GetThreadId();
}

// 32 bit FNV-1a hash function
static inline HashType GetHash(ThreadIdType id)
{
    const HashType offset_basis = 2166136261U;
    const HashType FNV_prime    = 16777619U;

    auto*       bp   = reinterpret_cast<unsigned char*>(&id);
    const auto* be   = bp + sizeof(id);
    HashType    hval = offset_basis;
    while (bp < be)
    {
        hval ^= static_cast<HashType>(*bp++);
        hval *= FNV_prime;
    }

    return hval;
}

Slot::Slot() : ThreadId(0), Storage(nullptr) {}

HashTableArray::HashTableArray(size_t sizeLg)
    : Size(1ULL << sizeLg), SizeLg(sizeLg), NumberOfEntries(0), Prev(nullptr)
{
    this->Slots = new Slot[this->Size];
}

HashTableArray::~HashTableArray()
{
    delete[] this->Slots;
}

// Recursively lookup the slot containing threadId in the HashTableArray
// linked list -- array
static Slot* LookupSlot(HashTableArray* array, ThreadIdType threadId, size_t hash)
{
    if (array == nullptr)
    {
        return nullptr;
    }

    const auto mask = array->Size - 1U;
    Slot*      slot = nullptr;  //NOLINT

    // since load factor is maintained below 0.5, this loop should hit an
    // empty slot if the queried slot does not exist in this array
    for (size_t idx = hash & mask;; idx = (idx + 1) & mask)  // linear probing
    {
        slot                    = array->Slots + idx;
        const auto slotThreadId = slot->ThreadId.load();
        if (slotThreadId == 0)  // empty slot means threadId doesn't exist in this array
        {
            slot = LookupSlot(array->Prev, threadId, hash);
            break;
        }
        if (slotThreadId == threadId)
        {
            break;
        }
    }

    return slot;
}

// Lookup threadId. Try to acquire a slot if it doesn't already exist.
// Does not block. Returns nullptr if acquire fails due to high load factor.
// Returns true in 'firstAccess' if threadID did not exist previously.
static Slot* AcquireSlot(
    HashTableArray* array, ThreadIdType threadId, size_t hash, bool& firstAccess)
{
    const auto mask = array->Size - 1U;
    Slot*      slot = nullptr;
    firstAccess     = false;

    for (size_t idx = hash & mask;; idx = (idx + 1) & mask)
    {
        slot                    = array->Slots + idx;
        const auto slotThreadId = slot->ThreadId.load();
        if (slotThreadId == 0)  // unused?
        {
            const std::scoped_lock lguard(slot->Mutex);

            const auto size = array->NumberOfEntries++;
            if ((size * 2) > array->Size)  // load factor is above threshold
            {
                --array->NumberOfEntries;
                return nullptr;  // indicate need for resizing
            }

            if (slot->ThreadId.load() == 0)  // not acquired in the meantime?
            {
                slot->ThreadId.store(threadId);
                // check previous arrays for the entry
                Slot* prevSlot = LookupSlot(array->Prev, threadId, hash);
                if (prevSlot != nullptr)
                {
                    slot->Storage = prevSlot->Storage;
                    // Do not clear PrevSlot's ThreadId as our technique of stopping
                    // linear probing at empty slots relies on slots not being
                    // "freed". Instead, clear previous slot's storage pointer as
                    // ThreadSpecificStorageIterator relies on this information to
                    // ensure that it doesn't iterate over the same thread's storage
                    // more than once.
                    prevSlot->Storage = nullptr;
                }
                else  // first time access
                {
                    slot->Storage = nullptr;
                    firstAccess   = true;
                }
                break;
            }
        }
        else if (slotThreadId == threadId)
        {
            break;
        }
    }

    return slot;
}

ThreadSpecific::ThreadSpecific(unsigned numThreads) : Size(0)
{
    const int  lastSetBit = (numThreads != 0 ? (int)std::floor(std::log2(numThreads)) : 0);
    const auto initSizeLg = (lastSetBit + 2);
    this->Root            = new HashTableArray(initSizeLg);
}

ThreadSpecific::~ThreadSpecific()
{
    HashTableArray const* array = this->Root;
    while (array != nullptr)
    {
        HashTableArray const* tofree = array;
        array                        = array->Prev;
        delete tofree;
    }
}

StoragePointerType& ThreadSpecific::GetStorage()
{
    const auto threadId = GetThreadId();
    const auto hash     = GetHash(threadId);

    Slot* slot = nullptr;
    while (slot == nullptr)
    {
        bool            firstAccess = false;
        HashTableArray* array       = this->Root.load();
        slot                        = AcquireSlot(array, threadId, hash, firstAccess);
        if (slot == nullptr)  // not enough room, resize
        {
            const std::scoped_lock lguard(this->Mutex);

            if (this->Root == array)
            {
                auto* newArray = new HashTableArray(array->SizeLg + 1);
                newArray->Prev = array;
                this->Root.store(newArray);
            }
        }
        else if (firstAccess)
        {
            this->Size++;
        }
    }
    return slot->Storage;
}

size_t ThreadSpecific::GetSize() const
{
    return this->Size;
}

}  // namespace xsigma::detail::smp::STDThread
