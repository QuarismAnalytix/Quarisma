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

#ifndef PARALLEL_TOOLS_API_H
#define PARALLEL_TOOLS_API_H

#include <memory>

#include "common/export.h"
#include "parallel/common/parallel_tools_impl.h"
#include "parallel/parallel.h"

#if XSIGMA_HAS_TBB
#include "parallel/tbb/parallel_tools_impl.h"
#elif XSIGMA_HAS_OPENMP
#include "parallel/openmp/parallel_tools_impl.h"
#else
#include "parallel/std_thread/parallel_tools_impl.h"
#endif

namespace xsigma
{
namespace detail
{
namespace parallel
{

// Compile-time backend selection based on availability
// Priority: TBB > OpenMP > std_thread
#if XSIGMA_HAS_TBB
constexpr backend_type selected_backend_tools = backend_type::TBB;
#elif XSIGMA_HAS_OPENMP
constexpr backend_type selected_backend_tools = backend_type::OpenMP;
#else
constexpr backend_type selected_backend_tools = backend_type::std_thread;
#endif

using selected_parallel_tools_impl = parallel_tools_impl<selected_backend_tools>;

class XSIGMA_VISIBILITY parallel_tools_api
{
public:
    //--------------------------------------------------------------------------------
    XSIGMA_API static parallel_tools_api& instance();

    //--------------------------------------------------------------------------------
    XSIGMA_API static backend_type get_backend_type();

    //--------------------------------------------------------------------------------
    XSIGMA_API static const char* get_backend();

    //--------------------------------------------------------------------------------
    XSIGMA_API bool set_backend(const char* type);

    //--------------------------------------------------------------------------------
    XSIGMA_API void initialize(int num_threads = 0);

    //--------------------------------------------------------------------------------
    XSIGMA_API int estimated_number_of_threads();

    //--------------------------------------------------------------------------------
    XSIGMA_API int estimated_default_number_of_threads();

    //------------------------------------------------------------------------------
    XSIGMA_API void set_nested_parallelism(bool is_nested);

    //--------------------------------------------------------------------------------
    XSIGMA_API bool nested_parallelism();

    //--------------------------------------------------------------------------------
    XSIGMA_API bool is_parallel_scope();

    //--------------------------------------------------------------------------------
    XSIGMA_API bool single_thread();

    //--------------------------------------------------------------------------------
    int get_internal_desired_number_of_thread() { return desired_number_of_thread_; }

    //------------------------------------------------------------------------------
    template <typename Config, typename T>
    void local_scope(Config const& config, T&& lambda)
    {
        const Config old_config(*this);
        *this << config;
        try
        {
            lambda();
        }
        catch (...)
        {
            *this << old_config;
            throw;
        }
        *this << old_config;
    }

    //--------------------------------------------------------------------------------
    // Direct backend execution using compile-time selected backend
    //--------------------------------------------------------------------------------
    template <typename FunctorInternal>
    void parallel_for(size_t first, size_t last, size_t grain, FunctorInternal& fi)
    {
        backend_impl_.parallel_for(first, last, grain, fi);
    }

    // disable copying
    parallel_tools_api(parallel_tools_api const&)  = delete;
    void operator=(parallel_tools_api const&) = delete;

private:
    //--------------------------------------------------------------------------------
    XSIGMA_API parallel_tools_api();

    //--------------------------------------------------------------------------------
    XSIGMA_API void refresh_number_of_thread();

    //--------------------------------------------------------------------------------
    template <typename Config>
    parallel_tools_api& operator<<(Config const& config)
    {
        this->initialize(config.max_number_of_threads_);
        this->set_backend(config.backend_.c_str());
        this->set_nested_parallelism(config.nested_parallelism_);
        return *this;
    }

    /**
   * Desired number of threads
   */
    int desired_number_of_thread_ = 0;

    /**
   * Single backend implementation selected at compile-time
   */
    selected_parallel_tools_impl backend_impl_;
};

}  // namespace parallel
}  // namespace detail
}  // namespace xsigma

#endif
