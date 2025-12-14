/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * This file is part of XSigma and is licensed under a dual-license model:
 *
 *   - Open-source License (GPLv3):
 *       Free for personal, academic, and research use under the terms of
 *       the GNU General Public License v3.0 or later.
 *
 *   - Commercial License:
 *       A commercial license is required for proprietary, closed-source,
 *       or SaaS usage. Contact us to obtain a commercial agreement.
 *
 * Contact: licensing@xsigma.co.uk
 * Website: https://www.xsigma.co.uk
 *
 * Portions of this code are based on VTK (Visualization Toolkit):

 *   Licensed under BSD-3-Clause
 */

/**
 * @class   smp_tools
 * @brief   A set of parallel (multi-threaded) utility functions.
 *
 * smp_tools provides a set of utility functions that can
 * be used to parallelize parts of code using multiple threads.
 * There are several back-end implementations of parallel functionality
 * (currently std_thread, TBB, and OpenMP) that actual execution is
 * delegated to.
 */

#ifndef SMP_TOOLS_H
#define SMP_TOOLS_H

#include <functional>   // For std::function
#include <type_traits>  // For std::enable_if

#include "common/export.h"
#include "smp/common/smp_tools_api.h"

#if XSIGMA_HAS_TBB
#include <tbb/enumerable_thread_specific.h>
#endif

namespace xsigma
{
namespace detail
{
namespace smp
{

#if XSIGMA_HAS_OPENMP
// Static storage for OpenMP threadprivate
// Each thread gets its own copy automatically via OpenMP's threadprivate pragma.
// NOTE: Do NOT use 'thread_local' with OpenMP's threadprivate pragma - they are incompatible.
// OpenMP's threadprivate pragma provides thread-local storage for OpenMP parallel regions.
unsigned char smp_tools_functor_initialized = 0;
#pragma omp   threadprivate(smp_tools_functor_initialized)
#endif

template <typename T>
class smp_tools_has_initialize
{
    using no_type  = char (&)[1];
    using yes_type = char (&)[2];
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
    using no_type  = char (&)[1];
    using yes_type = char (&)[2];
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
    Functor& f_;
    smp_tools_functor_internal(Functor& f) : f_(f) {}
    void Execute(size_t first, size_t last) { this->f_(first, last); }
    void parallel_for(size_t first, size_t last, size_t grain)
    {
        auto& SMPToolsAPI = smp_tools_api::instance();
        SMPToolsAPI.parallel_for(first, last, grain, *this);
    }
    smp_tools_functor_internal<Functor, false>& operator=(
        const smp_tools_functor_internal<Functor, false>&);
    smp_tools_functor_internal(const smp_tools_functor_internal<Functor, false>&);
};

template <typename Functor>
struct smp_tools_functor_internal<Functor, true>
{
    Functor& f_;

#if XSIGMA_HAS_TBB
    // TBB backend: Use enumerable_thread_specific
    mutable tbb::enumerable_thread_specific<unsigned char> initialized_;
#endif

    smp_tools_functor_internal(Functor& f)
        : f_(f)
#if XSIGMA_HAS_TBB
          ,
          initialized_(0)
#endif
    {
    }

    void Execute(size_t first, size_t last)
    {
#if XSIGMA_HAS_OPENMP
        // OpenMP backend: Use threadprivate static
        if (!smp_tools_functor_initialized)
        {
            this->f_.Initialize();
            smp_tools_functor_initialized = 1;
        }
#elif XSIGMA_HAS_TBB
        // TBB backend: Use enumerable_thread_specific
        unsigned char& inited = initialized_.local();
        if (!inited)
        {
            this->f_.Initialize();
            inited = 1;
        }
#else
        // Native backend: Use standard C++ thread_local
        thread_local unsigned char initialized = 0;
        if (!initialized)
        {
            this->f_.Initialize();
            initialized = 1;
        }
#endif
        this->f_(first, last);
    }

    void parallel_for(size_t first, size_t last, size_t grain)
    {
        auto& SMPToolsAPI = smp_tools_api::instance();
        SMPToolsAPI.parallel_for(first, last, grain, *this);
        this->f_.Reduce();
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
    using type = smp_tools_functor_internal<Functor, init>;
};

template <typename Functor>
class smp_tools_lookup_for<Functor const>
{
    static bool const init = smp_tools_has_initialize_const<Functor>::value;

public:
    using type = smp_tools_functor_internal<Functor const, init>;
};

template <typename T>
using resolved_not_int = typename std::enable_if<!std::is_integral<T>::value, void>::type;

}  // namespace smp
}  // namespace detail
}  // namespace xsigma

class XSIGMA_VISIBILITY smp_tools
{
public:
    ///@{
    /**
   * @brief Execute a for operation in parallel using a functor (legacy API).
   *
   * This API is maintained for backward compatibility with existing code.
   * For new code, prefer the lambda-based overloads above.
   *
   * The functor should implement:
   * - operator()(size_t first, size_t last)
   * - Optional: Initialize() method for per-thread initialization
   * - Optional: Reduce() method for result aggregation
   *
   * @param first The start of the range (inclusive)
   * @param last The end of the range (exclusive)
   * @param grain Hint about coarseness for parallelization
   * @param f Functor object
   */
    template <typename Functor>
    static void parallel_for(size_t first, size_t last, size_t grain, Functor& f)
    {
        typename xsigma::detail::smp::smp_tools_lookup_for<Functor>::type fi(f);
        fi.parallel_for(first, last, grain);
    }

    template <typename Functor>
    static void parallel_for(size_t first, size_t last, size_t grain, const Functor& f)
    {
        typename xsigma::detail::smp::smp_tools_lookup_for<Functor const>::type fi(f);
        fi.parallel_for(first, last, grain);
    }

    /**
   * /!\ This method is not thread safe.
   * Initialize the underlying libraries for execution.
   */
    XSIGMA_API static void initialize(int num_threads = 0);

    /**
   * Get the estimated number of threads being used by the backend.
   */
    XSIGMA_API static int estimated_number_of_threads();

    /**
   * Get the estimated number of threads being used by the backend by default.
   */
    XSIGMA_API static int estimated_default_number_of_threads();

    /**
   * /!\ This method is not thread safe.
   * If true enable nested parallelism for underlying backends.
   */
    XSIGMA_API static void set_nested_parallelism(bool is_nested);

    /**
   * Get true if the nested parallelism is enabled.
   */
    XSIGMA_API static bool nested_parallelism();

    /**
   * Return true if it is called from a parallel scope.
   */
    XSIGMA_API static bool is_parallel_scope();

    /**
   * Returns true if the given thread is specified thread
   * for single scope. Returns false otherwise.
   */
    XSIGMA_API static bool single_thread();

    /**
   * Structure used to specify configuration for local_scope() method.
   */
    struct config
    {
        int  max_number_of_threads_ = 0;
        bool nested_parallelism_    = false;

        config() = default;
        config(int max_num_threads) : max_number_of_threads_(max_num_threads) {}
        config(bool nested) : nested_parallelism_(nested) {}
        config(int max_num_threads, const std::string& backend_name, bool nested)
            : max_number_of_threads_(max_num_threads), nested_parallelism_(nested)
        {
        }
        config(xsigma::detail::smp::smp_tools_api& API)
            : max_number_of_threads_(API.get_internal_desired_number_of_thread()),
              nested_parallelism_(API.nested_parallelism())
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
        auto& SMPToolsAPI = xsigma::detail::smp::smp_tools_api::instance();
        SMPToolsAPI.local_scope<smp_tools::config>(cfg, lambda);
    }
};

#endif
