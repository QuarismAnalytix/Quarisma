

// This file incorporates code from the Visualization Toolkit (VTK) and remains subject to the BSD-3-Clause VTK license.

#ifndef STDThreadtools_impl_txx
#define STDThreadtools_impl_txx

#include <algorithm>   // for min
#include <atomic>      // for atomic
#include <functional>  // for bind
#include <iterator>    // for distance
#include <memory>      // for allocator_traits<>::...
#include <thread>      // for thread
#include <vector>      // for vector

#include "common/macros.h"
#include "smp/Common/tools_impl.h"      // for tools_impl, Back...
#include "smp/Common/tools_internal.h"  // for UnaryTransformCall
#include "smp/STDThread/thread_pool.h"  // for thread_pool

namespace xsigma
{
namespace detail
{
namespace smp
{

int XSIGMA_API GetNumberOfThreadsSTDThread();

//--------------------------------------------------------------------------------
template <>
template <typename FunctorInternal>
void tools_impl<BackendType::STDThread>::For(int first, int last, int grain, FunctorInternal& fi)
{
    int n = last - first;
    if (n <= 0)
    {
        return;
    }

    if (grain >= n || (!this->NestedActivated && thread_pool::GetInstance().IsParallelScope()))
    {
        fi.Execute(first, last);
    }
    else
    {
        int threadNumber = GetNumberOfThreadsSTDThread();

        if (grain <= 0)
        {
            int estimateGrain = (last - first) / (threadNumber * 4);
            grain             = (estimateGrain > 0) ? estimateGrain : 1;
        }

        auto proxy = thread_pool::GetInstance().AllocateThreads(threadNumber);

        for (int from = first; from < last; from += grain)
        {
            const auto to = (std::min)(from + grain, last);
            proxy.DoJob([&fi, from, to] { fi.Execute(from, to); });
        }

        proxy.Join();
    }
}

//--------------------------------------------------------------------------------
template <>
template <typename InputIt, typename OutputIt, typename Functor>
void tools_impl<BackendType::STDThread>::Transform(
    InputIt inBegin, InputIt inEnd, OutputIt outBegin, Functor transform)
{
    auto size = (int)std::distance(inBegin, inEnd);

    UnaryTransformCall<InputIt, OutputIt, Functor> exec(inBegin, outBegin, transform);
    this->For(0, size, 0, exec);
}

//--------------------------------------------------------------------------------
template <>
template <typename InputIt1, typename InputIt2, typename OutputIt, typename Functor>
void tools_impl<BackendType::STDThread>::Transform(
    InputIt1 inBegin1, InputIt1 inEnd, InputIt2 inBegin2, OutputIt outBegin, Functor transform)
{
    auto size = (int)std::distance(inBegin1, inEnd);

    BinaryTransformCall<InputIt1, InputIt2, OutputIt, Functor> exec(
        inBegin1, inBegin2, outBegin, transform);
    this->For(0, size, 0, exec);
}

//--------------------------------------------------------------------------------
template <>
template <typename Iterator, typename T>
void tools_impl<BackendType::STDThread>::Fill(Iterator begin, Iterator end, const T& value)
{
    auto size = (int)std::distance(begin, end);

    FillFunctor<T>                                         fill(value);
    UnaryTransformCall<Iterator, Iterator, FillFunctor<T>> exec(begin, begin, fill);
    this->For(0, size, 0, exec);
}

//--------------------------------------------------------------------------------
template <>
template <typename RandomAccessIterator>
void tools_impl<BackendType::STDThread>::Sort(RandomAccessIterator begin, RandomAccessIterator end)
{
    std::sort(begin, end);
}

//--------------------------------------------------------------------------------
template <>
template <typename RandomAccessIterator, typename Compare>
void tools_impl<BackendType::STDThread>::Sort(
    RandomAccessIterator begin, RandomAccessIterator end, Compare comp)
{
    std::sort(begin, end, comp);
}

//--------------------------------------------------------------------------------
template <>
void tools_impl<BackendType::STDThread>::Initialize(int);

//--------------------------------------------------------------------------------
template <>
int tools_impl<BackendType::STDThread>::GetEstimatedNumberOfThreads();

//--------------------------------------------------------------------------------
template <>
int tools_impl<BackendType::STDThread>::GetEstimatedDefaultNumberOfThreads();

//--------------------------------------------------------------------------------
template <>
bool tools_impl<BackendType::STDThread>::GetSingleThread();

//--------------------------------------------------------------------------------
template <>
bool tools_impl<BackendType::STDThread>::IsParallelScope();
}  // namespace smp
}  // namespace detail
}  // namespace xsigma

#endif
