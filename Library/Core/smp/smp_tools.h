// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   smp_tools
 * @brief   A set of parallel (multi-threaded) utility functions.
 *
 * smp_tools provides a set of utility functions that can
 * be used to parallelize parts of code using multiple threads.
 * There are several back-end implementations of parallel functionality
 * (currently std_thread, TBB, and OpenMP) that actual execution is
 * delegated to.
 *
 * @sa
 * smp_thread_local
 * smp_thread_local_object
 */

#ifndef SMP_TOOLS_H
#define SMP_TOOLS_H

#include <functional>   // For std::function
#include <type_traits>  // For std::enable_if

#include "common/export.h"
#include "smp/common/smp_tools_api.h"
#include "smp_thread_local.h"

namespace conductor
{
namespace detail
{
namespace smp
{
template <typename T>
class smp_tools_has_initialize
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
class smp_tools_has_initialize_const
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
struct smp_tools_functor_internal;

template <typename Functor>
struct smp_tools_functor_internal<Functor, false>
{
    Functor& F;
    smp_tools_functor_internal(Functor& f) : F(f) {}
    void Execute(size_t first, size_t last) { this->F(first, last); }
    void For(size_t first, size_t last, size_t grain)
    {
        auto& SMPToolsAPI = smp_tools_api::get_instance();
        SMPToolsAPI.For(first, last, grain, *this);
    }
    smp_tools_functor_internal<Functor, false>& operator=(
        const smp_tools_functor_internal<Functor, false>&);
    smp_tools_functor_internal(const smp_tools_functor_internal<Functor, false>&);
};

template <typename Functor>
struct smp_tools_functor_internal<Functor, true>
{
    Functor&                        F;
    smp_thread_local<unsigned char> Initialized;
    smp_tools_functor_internal(Functor& f) : F(f), Initialized(0) {}
    void Execute(size_t first, size_t last)
    {
        unsigned char& inited = this->Initialized.local();
        if (!inited)
        {
            this->F.Initialize();
            inited = 1;
        }
        this->F(first, last);
    }
    void For(size_t first, size_t last, size_t grain)
    {
        auto& SMPToolsAPI = smp_tools_api::get_instance();
        SMPToolsAPI.For(first, last, grain, *this);
        this->F.Reduce();
    }
    smp_tools_functor_internal<Functor, true>& operator=(
        const smp_tools_functor_internal<Functor, true>&);
    smp_tools_functor_internal(const smp_tools_functor_internal<Functor, true>&);
};

template <typename Functor>
class smp_tools_lookup_for
{
    static bool const init = smp_tools_has_initialize<Functor>::value;

public:
    typedef smp_tools_functor_internal<Functor, init> type;
};

template <typename Functor>
class smp_tools_lookup_for<Functor const>
{
    static bool const init = smp_tools_has_initialize_const<Functor>::value;

public:
    typedef smp_tools_functor_internal<Functor const, init> type;
};

template <typename Iterator, typename Functor, bool Init>
struct smp_tools_range_functor;

template <typename Iterator, typename Functor>
struct smp_tools_range_functor<Iterator, Functor, false>
{
    Functor&  F;
    Iterator& Begin;
    smp_tools_range_functor(Iterator& begin, Functor& f) : F(f), Begin(begin) {}
    void operator()(size_t first, size_t last)
    {
        Iterator itFirst(Begin);
        std::advance(itFirst, first);
        Iterator itLast(itFirst);
        std::advance(itLast, last - first);
        this->F(itFirst, itLast);
    }
};

template <typename Iterator, typename Functor>
struct smp_tools_range_functor<Iterator, Functor, true>
{
    Functor&  F;
    Iterator& Begin;
    smp_tools_range_functor(Iterator& begin, Functor& f) : F(f), Begin(begin) {}
    void Initialize() { this->F.Initialize(); }
    void operator()(size_t first, size_t last)
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
class smp_tools_lookup_range_for
{
    static bool const init = smp_tools_has_initialize<Functor>::value;

public:
    typedef smp_tools_range_functor<Iterator, Functor, init> type;
};

template <typename Iterator, typename Functor>
class smp_tools_lookup_range_for<Iterator, Functor const>
{
    static bool const init = smp_tools_has_initialize_const<Functor>::value;

public:
    typedef smp_tools_range_functor<Iterator, Functor const, init> type;
};

template <typename T>
using resolved_not_int = typename std::enable_if<!std::is_integral<T>::value, void>::type;

}  // namespace smp
}  // namespace detail
}  // namespace conductor

class XSIGMA_VISIBILITY smp_tools
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
    static void For(size_t first, size_t last, size_t grain, Functor& f)
    {
        typename conductor::detail::smp::smp_tools_lookup_for<Functor>::type fi(f);
        fi.For(first, last, grain);
    }

    template <typename Functor>
    static void For(size_t first, size_t last, size_t grain, Functor const& f)
    {
        typename conductor::detail::smp::smp_tools_lookup_for<Functor const>::type fi(f);
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
    static void For(size_t first, size_t last, Functor& f)
    {
        smp_tools::For(first, last, 0, f);
    }

    template <typename Functor>
    static void For(size_t first, size_t last, Functor const& f)
    {
        smp_tools::For(first, last, 0, f);
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
   */
    template <typename Iter, typename Functor>
    static conductor::detail::smp::resolved_not_int<Iter> For(
        Iter begin, Iter end, size_t grain, Functor& f)
    {
        size_t size = std::distance(begin, end);
        typename conductor::detail::smp::smp_tools_lookup_range_for<Iter, Functor>::type fi(
            begin, f);
        smp_tools::For(0, size, grain, fi);
    }

    template <typename Iter, typename Functor>
    static conductor::detail::smp::resolved_not_int<Iter> For(
        Iter begin, Iter end, size_t grain, Functor const& f)
    {
        size_t size = std::distance(begin, end);
        typename conductor::detail::smp::smp_tools_lookup_range_for<Iter, Functor const>::type fi(
            begin, f);
        smp_tools::For(0, size, grain, fi);
    }
    ///@}

    ///@{
    /**
   * Execute a for operation in parallel. Begin and end iterators
   * define the range over which to operate (which is defined
   * by the operator). The operation executed is defined by
   * operator() of the functor object. Uses a default value
   * for the grain.
   */
    template <typename Iter, typename Functor>
    static conductor::detail::smp::resolved_not_int<Iter> For(Iter begin, Iter end, Functor& f)
    {
        smp_tools::For(begin, end, 0, f);
    }

    template <typename Iter, typename Functor>
    static conductor::detail::smp::resolved_not_int<Iter> For(
        Iter begin, Iter end, Functor const& f)
    {
        smp_tools::For(begin, end, 0, f);
    }
    ///@}

    /**
   * Get the backend in use.
   */
    XSIGMA_API static const char* get_backend();

    /**
   * /!\ This method is not thread safe.
   * Change the backend in use.
   * The options can be: "std_thread", "TBB" or "OpenMP"
   */
    XSIGMA_API static bool set_backend(const char* backend);

    /**
   * /!\ This method is not thread safe.
   * Initialize the underlying libraries for execution.
   */
    XSIGMA_API static void initialize(int num_threads = 0);

    /**
   * Get the estimated number of threads being used by the backend.
   */
    XSIGMA_API static int get_estimated_number_of_threads();

    /**
   * Get the estimated number of threads being used by the backend by default.
   */
    XSIGMA_API static int get_estimated_default_number_of_threads();

    /**
   * /!\ This method is not thread safe.
   * If true enable nested parallelism for underlying backends.
   */
    XSIGMA_API static void set_nested_parallelism(bool is_nested);

    /**
   * Get true if the nested parallelism is enabled.
   */
    XSIGMA_API static bool get_nested_parallelism();

    /**
   * Return true if it is called from a parallel scope.
   */
    XSIGMA_API static bool is_parallel_scope();

    /**
   * Returns true if the given thread is specified thread
   * for single scope. Returns false otherwise.
   */
    XSIGMA_API static bool get_single_thread();

    /**
   * Structure used to specify configuration for local_scope() method.
   */
    struct config
    {
        int         max_number_of_threads = 0;
        std::string backend = conductor::detail::smp::smp_tools_api::get_instance().get_backend();
        bool        nested_parallelism = false;

        config() = default;
        config(int max_num_threads) : max_number_of_threads(max_num_threads) {}
        config(std::string backend_name) : backend(backend_name) {}
        config(bool nested) : nested_parallelism(nested) {}
        config(int max_num_threads, std::string backend_name, bool nested)
            : max_number_of_threads(max_num_threads),
              backend(backend_name),
              nested_parallelism(nested)
        {
        }
        config(conductor::detail::smp::smp_tools_api& API)
            : max_number_of_threads(API.get_internal_desired_number_of_thread()),
              backend(API.get_backend()),
              nested_parallelism(API.get_nested_parallelism())
        {
        }
    };

    /**
   * Threshold used by various cases to switch between serial and threaded execution.
   */
    static constexpr size_t THRESHOLD = 100000;

    /**
   * /!\ This method is not thread safe.
   * Change the number of threads locally within this scope and call a functor.
   */
    template <typename T>
    static void local_scope(config const& cfg, T&& lambda)
    {
        auto& SMPToolsAPI = conductor::detail::smp::smp_tools_api::get_instance();
        SMPToolsAPI.local_scope<smp_tools::config>(cfg, lambda);
    }
};

#endif
