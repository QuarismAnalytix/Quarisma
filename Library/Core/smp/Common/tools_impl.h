

// This file incorporates code from the Visualization Toolkit (VTK) and remains subject to the BSD-3-Clause VTK license.

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "common/configure.h"
#include "common/macros.h"

namespace quarisma
{
namespace detail
{
namespace smp
{

enum class BackendType : std::uint8_t
{
    STDThread = 0,
    TBB       = 1
};

inline constexpr std::size_t BackendSlotCount = 2;

#if QUARISMA_HAS_TBB
inline constexpr BackendType DefaultBackend = BackendType::TBB;
#else
inline constexpr BackendType DefaultBackend = BackendType::STDThread;
#endif

template <BackendType Backend>
class QUARISMA_VISIBILITY tools_impl
{
public:
    //--------------------------------------------------------------------------------
    void Initialize(int numThreads = 0);

    //--------------------------------------------------------------------------------
    int GetEstimatedNumberOfThreads();

    //--------------------------------------------------------------------------------
    int GetEstimatedDefaultNumberOfThreads();

    //--------------------------------------------------------------------------------
    void SetNestedParallelism(bool isNested) { this->NestedActivated = isNested; }

    //--------------------------------------------------------------------------------
    bool GetNestedParallelism() { return this->NestedActivated; }

    //--------------------------------------------------------------------------------
    bool IsParallelScope() { return this->IsParallel; }

    //--------------------------------------------------------------------------------
    bool GetSingleThread();

    //--------------------------------------------------------------------------------
    template <typename FunctorInternal>
    void For(int first, int last, int grain, FunctorInternal& fi);

    //--------------------------------------------------------------------------------
    template <typename InputIt, typename OutputIt, typename Functor>
    void Transform(InputIt inBegin, InputIt inEnd, OutputIt outBegin, Functor transform);

    //--------------------------------------------------------------------------------
    template <typename InputIt1, typename InputIt2, typename OutputIt, typename Functor>
    void Transform(
        InputIt1 inBegin1, InputIt1 inEnd, InputIt2 inBegin2, OutputIt outBegin, Functor transform);

    //--------------------------------------------------------------------------------
    template <typename Iterator, typename T>
    void Fill(Iterator begin, Iterator end, const T& value);

    //--------------------------------------------------------------------------------
    template <typename RandomAccessIterator>
    void Sort(RandomAccessIterator begin, RandomAccessIterator end);

    //--------------------------------------------------------------------------------
    template <typename RandomAccessIterator, typename Compare>
    void Sort(RandomAccessIterator begin, RandomAccessIterator end, Compare comp);

    //--------------------------------------------------------------------------------
    tools_impl() noexcept = default;

    //--------------------------------------------------------------------------------
    tools_impl(const tools_impl& other)
        : NestedActivated(other.NestedActivated), IsParallel(other.IsParallel.load())
    {
    }

    //--------------------------------------------------------------------------------
    void operator=(const tools_impl& other)
    {
        this->NestedActivated = other.NestedActivated;
        this->IsParallel      = other.IsParallel.load();
    }

private:
    bool              NestedActivated = false;
    std::atomic<bool> IsParallel{false};
};

using ExecuteFunctorPtrType = void (*)(void*, int, int, int);

}  // namespace smp
}  // namespace detail
}  // namespace quarisma

/* QUARISMA-HeaderTest-Exclude: tools_impl.h */
