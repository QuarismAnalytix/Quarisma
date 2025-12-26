
// This file incorporates code from the Visualization Toolkit (VTK) and remains subject to the BSD-3-Clause VTK license.

/**
 * @class   tools
 * @brief   A set of parallel (multi-threaded) utility functions.
 *
 * tools provides a set of utility functions that can
 * be used to parallelize parts of QUARISMA code using multiple threads.
 * There are several back-end implementations of parallel functionality
 * (currently STDThread or TBB when enabled) that actual execution is
 * delegated to.
 *
 * @sa
 * quarisma_thread_local
 * thread_localObject
 */

#pragma once

#include <functional>   // For std::function
#include <iterator>     // for distance
#include <string>       // for allocator, string
#include <type_traits>  // For std:::enable_if

#include "common/configure.h"
// For export macro
#include "smp/Common/tools_api.h"
#include "smp/quarisma_thread_local.h"  // For Initialized

#ifndef DOXYGEN_SHOULD_SKIP_THIS
namespace quarisma
{
namespace detail
{
namespace smp
{
template <typename T>
class tools_Has_Initialize
{
    typedef char (&no_type)[1];
    typedef char (&yes_type)[2];
    template <typename U, void (U::*)()>
    struct V
    {
    };
    template <typename U>
    static yes_type check(V<U, &U::Initialize>*);
    template <typename U>
    static no_type check(...);

public:
    static bool const value = sizeof(check<T>(nullptr)) == sizeof(yes_type);
};

template <typename T>
class tools_Has_Initialize_const
{
    typedef char (&no_type)[1];
    typedef char (&yes_type)[2];
    template <typename U, void (U::*)() const>
    struct V
    {
    };
    template <typename U>
    static yes_type check(V<U, &U::Initialize>*);
    template <typename U>
    static no_type check(...);

public:
    static bool const value = sizeof(check<T>(0)) == sizeof(yes_type);
};

template <typename Functor, bool Init>
struct tools_FunctorInternal;

template <typename Functor>
struct tools_FunctorInternal<Functor, false>
{
    Functor& F;
    tools_FunctorInternal(Functor& f) : F(f) {}
    void Execute(int first, int last) { this->F(first, last); }
    void For(int first, int last, int grain)
    {
        auto& SMPToolsAPI = tools_api::GetInstance();
        SMPToolsAPI.For(first, last, grain, *this);
    }
    tools_FunctorInternal<Functor, false>& operator=(const tools_FunctorInternal<Functor, false>&);
    tools_FunctorInternal(const tools_FunctorInternal<Functor, false>&);
};

template <typename Functor>
struct tools_FunctorInternal<Functor, true>
{
    Functor&                           F;
    quarisma_thread_local<unsigned char> Initialized;
    tools_FunctorInternal(Functor& f) : F(f), Initialized(0) {}
    void Execute(int first, int last)
    {
        unsigned char& inited = this->Initialized.Local();
        if (!inited)
        {
            this->F.Initialize();
            inited = 1;
        }
        this->F(first, last);
    }
    void For(int first, int last, int grain)
    {
        auto& SMPToolsAPI = tools_api::GetInstance();
        SMPToolsAPI.For(first, last, grain, *this);
        this->F.Reduce();
    }
    tools_FunctorInternal<Functor, true>& operator=(const tools_FunctorInternal<Functor, true>&);
    tools_FunctorInternal(const tools_FunctorInternal<Functor, true>&);
};

template <typename Functor>
class tools_Lookup_For
{
    static bool const init = tools_Has_Initialize<Functor>::value;

public:
    typedef tools_FunctorInternal<Functor, init> type;
};

template <typename Functor>
class tools_Lookup_For<Functor const>
{
    static bool const init = tools_Has_Initialize_const<Functor>::value;

public:
    typedef tools_FunctorInternal<Functor const, init> type;
};

template <typename Iterator, typename Functor, bool Init>
struct tools_RangeFunctor;

template <typename Iterator, typename Functor>
struct tools_RangeFunctor<Iterator, Functor, false>
{
    Functor&  F;
    Iterator& Begin;
    tools_RangeFunctor(Iterator& begin, Functor& f) : F(f), Begin(begin) {}
    void operator()(int first, int last)
    {
        Iterator itFirst(Begin);
        std::advance(itFirst, first);
        Iterator itLast(itFirst);
        std::advance(itLast, last - first);
        this->F(itFirst, itLast);
    }
};

template <typename Iterator, typename Functor>
struct tools_RangeFunctor<Iterator, Functor, true>
{
    Functor&  F;
    Iterator& Begin;
    tools_RangeFunctor(Iterator& begin, Functor& f) : F(f), Begin(begin) {}
    void Initialize() { this->F.Initialize(); }
    void operator()(int first, int last)
    {
        Iterator itFirst(Begin);
        std::advance(itFirst, first);
        Iterator itLast(itFirst);
        std::advance(itLast, last - first);
        this->F(itFirst, itLast);
    }
    void Reduce() { this->F.Reduce(); }
};

template <typename Iterator, typename Functor>
class tools_Lookup_RangeFor
{
    static bool const init = tools_Has_Initialize<Functor>::value;

public:
    typedef tools_RangeFunctor<Iterator, Functor, init> type;
};

template <typename Iterator, typename Functor>
class tools_Lookup_RangeFor<Iterator, Functor const>
{
    static bool const init = tools_Has_Initialize_const<Functor>::value;

public:
    typedef tools_RangeFunctor<Iterator, Functor const, init> type;
};

template <typename T>
using resolvedNotInt = typename std::enable_if<!std::is_integral<T>::value, void>::type;

}  // namespace smp
}  // namespace detail
}  // namespace quarisma
#endif  // DOXYGEN_SHOULD_SKIP_THIS

namespace quarisma
{
class QUARISMA_VISIBILITY tools
{
public:
    ///@{
    /**
   * Execute a for operation in parallel. First and last
   * define the range over which to operate (which is defined
   * by the operator). The operation executed is defined by
   * operator() of the functor object. The grain gives the parallel
   * engine a hint about the coarseness over which to parallelize
   * the function (as defined by last-first of each execution of
   * operator() ).
   */
    template <typename Functor>
    static void For(int first, int last, int grain, Functor& f)
    {
        typename quarisma::detail::smp::tools_Lookup_For<Functor>::type fi(f);
        fi.For(first, last, grain);
    }

    template <typename Functor>
    static void For(int first, int last, int grain, Functor const& f)
    {
        typename quarisma::detail::smp::tools_Lookup_For<Functor const>::type fi(f);
        fi.For(first, last, grain);
    }
    ///@}

    ///@{
    /**
   * Execute a for operation in parallel. First and last
   * define the range over which to operate (which is defined
   * by the operator). The operation executed is defined by
   * operator() of the functor object. The grain gives the parallel
   * engine a hint about the coarseness over which to parallelize
   * the function (as defined by last-first of each execution of
   * operator() ). Uses a default value for the grain.
   */
    template <typename Functor>
    static void For(int first, int last, Functor& f)
    {
        tools::For(first, last, 0, f);
    }

    template <typename Functor>
    static void For(int first, int last, Functor const& f)
    {
        tools::For(first, last, 0, f);
    }
    ///@}

    ///@{
    /**
   * Execute a for operation in parallel. Begin and end iterators
   * define the range over which to operate (which is defined
   * by the operator). The operation executed is defined by
   * operator() of the functor object. The grain gives the parallel
   * engine a hint about the coarseness over which to parallelize
   * the function (as defined by last-first of each execution of
   * operator() ).
   *
   * Usage example:
   * \code
   * template<class IteratorT>
   * class ExampleFunctor
   * {
   *   void operator()(IteratorT begin, IteratorT end)
   *   {
   *     for (IteratorT it = begin; it != end; ++it)
   *     {
   *       // Do stuff
   *     }
   *   }
   * };
   * ExampleFunctor<std::set<int>::iterator> worker;
   * tools::For(container.begin(), container.end(), 5, worker);
   * \endcode
   *
   * Lambda are also supported through Functor const&
   * function overload:
   * \code
   * tools::For(container.begin(), container.end(), 5,
   *    [](std::set<int>::iterator begin, std::set<int>::iterator end) {
   *      // Do stuff
   *    });
   * \endcode
   */
    template <typename Iter, typename Functor>
    static quarisma::detail::smp::resolvedNotInt<Iter> For(
        Iter begin, Iter end, int grain, Functor& f)
    {
        int size = (int)std::distance(begin, end);
        typename quarisma::detail::smp::tools_Lookup_RangeFor<Iter, Functor>::type fi(begin, f);
        tools::For(0, size, grain, fi);
    }

    template <typename Iter, typename Functor>
    static quarisma::detail::smp::resolvedNotInt<Iter> For(
        Iter begin, Iter end, int grain, Functor const& f)
    {
        int size = std::distance(begin, end);
        typename quarisma::detail::smp::tools_Lookup_RangeFor<Iter, Functor const>::type fi(begin, f);
        tools::For(0, size, grain, fi);
    }
    ///@}

    ///@{
    /**
   * Execute a for operation in parallel. Begin and end iterators
   * define the range over which to operate (which is defined
   * by the operator). The operation executed is defined by
   * operator() of the functor object. Uses a default value
   * for the grain.
   *
   * Usage example:
   * \code
   * template<class IteratorT>
   * class ExampleFunctor
   * {
   *   void operator()(IteratorT begin, IteratorT end)
   *   {
   *     for (IteratorT it = begin; it != end; ++it)
   *     {
   *       // Do stuff
   *     }
   *   }
   * };
   * ExampleFunctor<std::set<int>::iterator> worker;
   * tools::For(container.begin(), container.end(), worker);
   * \endcode
   *
   * Lambda are also supported through Functor const&
   * function overload:
   * \code
   * tools::For(container.begin(), container.end(),
   *    [](std::set<int>::iterator begin, std::set<int>::iterator end) {
   *      // Do stuff
   *    });
   * \endcode
   */
    template <typename Iter, typename Functor>
    static quarisma::detail::smp::resolvedNotInt<Iter> For(Iter begin, Iter end, Functor& f)
    {
        tools::For(begin, end, 0, f);
    }

    template <typename Iter, typename Functor>
    static quarisma::detail::smp::resolvedNotInt<Iter> For(Iter begin, Iter end, Functor const& f)
    {
        tools::For(begin, end, 0, f);
    }
    ///@}

    /**
   * Get the backend in use.
   */
    QUARISMA_API static const char* GetBackend();

    /**
   * /!\ This method is not thread safe.
   * Change the backend in use.
   * The options can be: "STDThread" (default) or "TBB" (when QUARISMA_HAS_TBB is enabled)
   *
   * QUARISMA_SMP_BACKEND_IN_USE env variable can also be used to set the default SMPTools
   * backend, in that case SetBackend() doesn't need to be called.
   * The backend selected with SetBackend() have the priority over QUARISMA_SMP_BACKEND_IN_USE.
   *
   * SetBackend() will return true if the backend was found and available.
   */
    QUARISMA_API static bool SetBackend(const char* backend);

    /**
   * /!\ This method is not thread safe.
   * Initialize the underlying libraries for execution. This is
   * not required as it is automatically defined by the libraries.
   * However, it can be used to control the maximum number of thread used.
   * Make sure to call it before the parallel operation.
   *
   * If Initialize is called without argument it will reset
   * to the maximum number of threads or use the QUARISMA_SMP_MAX_THREADS
   * env variable if it is defined.
   *
   * Note: If QUARISMA_SMP_MAX_THREADS env variable is defined the SMPTools will try
   * to use it to set the maximum number of threads. Initialize() doesn't
   * need to be called.
   */
    QUARISMA_API static void Initialize(int numThreads = 0);

    /**
   * Get the estimated number of threads being used by the backend.
   * This should be used as just an estimate since the number of threads may
   * vary dynamically and a particular task may not be executed on all the
   * available threads.
   */
    QUARISMA_API static int GetEstimatedNumberOfThreads();

    /**
   * Get the estimated number of threads being used by the backend by default.
   * This should be used as just an estimate since the number of threads may
   * vary dynamically and a particular task may not be executed on all the
   * available threads.
   */
    QUARISMA_API static int GetEstimatedDefaultNumberOfThreads();

    /**
   * /!\ This method is not thread safe.
   * If true enable nested parallelism for underlying backends.
   * When enabled the comportement is different for each backend:
   *    - TBB uses a single shared thread pool even for nested regions.
   *    - STDThread creates nested thread pools when required.
   *
   * Default to false except for TBB.
   */
    QUARISMA_API static void SetNestedParallelism(bool isNested);

    /**
   * Get true if the nested parallelism is enabled.
   * By default, nested parallelism is enabled only for TBB.
   */
    QUARISMA_API static bool GetNestedParallelism();

    /**
   * Return true if it is called from a parallel scope.
   */
    QUARISMA_API static bool IsParallelScope();

    /**
   * Returns true if the given thread is specified thread
   * for single scope. Returns false otherwise.
   */
    QUARISMA_API static bool GetSingleThread();

    /**
   * Structure used to specify configuration for LocalScope() method.
   * Several parameters can be configured:
   *    - MaxNumberOfThreads set the maximum number of threads.
   *    - Backend set a specific SMPTools backend.
   *    - NestedParallelism, if true enable nested parallelism.
   */
    struct Config
    {
        int         MaxNumberOfThreads = 0;
        std::string Backend            = quarisma::detail::smp::tools_api::GetInstance().GetBackend();
        bool        NestedParallelism  = false;

        Config() = default;
        Config(int maxNumberOfThreads) : MaxNumberOfThreads(maxNumberOfThreads) {}
        Config(std::string backend) : Backend(backend) {}
        Config(bool nestedParallelism) : NestedParallelism(nestedParallelism) {}
        Config(int maxNumberOfThreads, std::string backend, bool nestedParallelism)
            : MaxNumberOfThreads(maxNumberOfThreads),
              Backend(backend),
              NestedParallelism(nestedParallelism)
        {
        }
#ifndef DOXYGEN_SHOULD_SKIP_THIS
        Config(quarisma::detail::smp::tools_api& API)
            : MaxNumberOfThreads(API.GetInternalDesiredNumberOfThread()),
              Backend(API.GetBackend()),
              NestedParallelism(API.GetNestedParallelism())
        {
        }
#endif  // DOXYGEN_SHOULD_SKIP_THIS
    };
    /**
   * @brief This is an empirically determined threshold used by various cases to switch between
   * serial and threaded execution. There is a startup cost to threading which is not worth it for
   * small numbers of elements. This variable should be used only for small/light computations.
   *
   * The developer can use this value by choosing one of the following options:
   * 1. In the first option, the value is used as the grain size for the parallel operation:
   * \code
   * tools::For(0, size, tools::THRESHOLD, worker);
   * \endcode
   *
   * 2. In the second option, an explicit branch allows for backend control
   * \code
   * if (tools::THRESHOLD > size)
   * {
   *   // Do the work in serial
   * }
   * else
   * {
   *   // Do the work in parallel
   * }
   * \endcode
   */
    static constexpr int THRESHOLD = 100000;

    /**
   * /!\ This method is not thread safe.
   * Change the number of threads locally within this scope and call a functor which
   * should contains a tools method.
   *
   * Usage example:
   * \code
   * tools::LocalScope(
   *   tools::Config{ 4, "STDThread", false }, [&]() { tools::For(0, size, worker); });
   * \endcode
   */
    template <typename T>
    static void LocalScope(Config const& config, T&& lambda)
    {
        auto& SMPToolsAPI = quarisma::detail::smp::tools_api::GetInstance();
        SMPToolsAPI.LocalScope<tools::Config>(config, lambda);
    }

    /**
   * A convenience method for transforming data. It is a drop in replacement for
   * std::transform(), it does a unary operation on the input ranges. The data array must have the
   * same length. The performed transformation is defined by operator() of the functor object.
   *
   * Usage example with quarismaDataArray:
   * \code
   * const auto range0 = quarisma::DataArrayValueRange<1>(array0);
   * auto range1 = quarisma::DataArrayValueRange<1>(array1);
   * tools::Transform(
   *   range0.cbegin(), range0.cend(), range1.begin(), [](double x) { return x - 1; });
   * \endcode
   *
   * Please visit quarismaDataArrayRange.h documentation for more information and optimisation.
   */
    template <typename InputIt, typename OutputIt, typename Functor>
    static void Transform(InputIt inBegin, InputIt inEnd, OutputIt outBegin, Functor transform)
    {
        auto& SMPToolsAPI = quarisma::detail::smp::tools_api::GetInstance();
        SMPToolsAPI.Transform(inBegin, inEnd, outBegin, transform);
    }

    /**
   * A convenience method for transforming data. It is a drop in replacement for
   * std::transform(), it does a binary operation on the input ranges. The data array must have the
   * same length. The performed transformation is defined by operator() of the functor object.
   *
   * Usage example with quarismaDataArray:
   * \code
   * const auto range0 = quarisma::DataArrayValueRange<1>(array0);
   * auto range1 = quarisma::DataArrayValueRange<1>(array1);
   * tools::Transform(
   *   range0.cbegin(), range0.cend(), range1.cbegin(), range1.begin(),
   *   [](double x, double y) { return x * y; });
   * \endcode
   *
   * Please visit quarismaDataArrayRange.h documentation for more information and optimisation.
   */
    template <typename InputIt1, typename InputIt2, typename OutputIt, typename Functor>
    static void Transform(
        InputIt1 inBegin1, InputIt1 inEnd, InputIt2 inBegin2, OutputIt outBegin, Functor transform)
    {
        auto& SMPToolsAPI = quarisma::detail::smp::tools_api::GetInstance();
        SMPToolsAPI.Transform(inBegin1, inEnd, inBegin2, outBegin, transform);
    }

    /**
   * A convenience method for filling data. It is a drop in replacement for std::fill(),
   * it assign the given value to the element in ranges.
   *
   * Usage example with quarismaDataArray:
   * \code
   * // Fill range with its first tuple value
   * auto range = quarisma::DataArrayTupleRange<1>(array);
   * const auto value = *range.begin();
   * tools::Fill(range.begin(), range.end(), value);
   * \endcode
   *
   * Please visit quarismaDataArrayRange.h documentation for more information and optimisation.
   */
    template <typename Iterator, typename T>
    static void Fill(Iterator begin, Iterator end, const T& value)
    {
        auto& SMPToolsAPI = quarisma::detail::smp::tools_api::GetInstance();
        SMPToolsAPI.Fill(begin, end, value);
    }

    /**
   * A convenience method for sorting data. It is a drop in replacement for
   * std::sort(). Under the hood different methods are used. For example,
   * tbb::parallel_sort is used in TBB.
   */
    template <typename RandomAccessIterator>
    static void Sort(RandomAccessIterator begin, RandomAccessIterator end)
    {
        auto& SMPToolsAPI = quarisma::detail::smp::tools_api::GetInstance();
        SMPToolsAPI.Sort(begin, end);
    }

    /**
   * A convenience method for sorting data. It is a drop in replacement for
   * std::sort(). Under the hood different methods are used. For example,
   * tbb::parallel_sort is used in TBB. This version of Sort() takes a
   * comparison class.
   */
    template <typename RandomAccessIterator, typename Compare>
    static void Sort(RandomAccessIterator begin, RandomAccessIterator end, Compare comp)
    {
        auto& SMPToolsAPI = quarisma::detail::smp::tools_api::GetInstance();
        SMPToolsAPI.Sort(begin, end, comp);
    }
};
}  // namespace quarisma
