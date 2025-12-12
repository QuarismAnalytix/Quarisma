// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef SMP_TOOLS_API_H
#define SMP_TOOLS_API_H

#include <memory>

#include "common/export.h"
#include "smp/common/smp_tools_impl.h"
#include "smp/smp.h"

#if XSIGMA_HAS_TBB
#include "smp/tbb/smp_tools_impl.h"
#elif XSIGMA_HAS_OPENMP
#include "smp/openmp/smp_tools_impl.h"
#else
#include "smp/std_thread/smp_tools_impl.h"
#endif

namespace xsigma
{
namespace detail
{
namespace smp
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

using selected_smp_tools_impl = smp_tools_impl<selected_backend_tools>;

class XSIGMA_VISIBILITY smp_tools_api
{
public:
    //--------------------------------------------------------------------------------
    XSIGMA_API static smp_tools_api& instance();

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
    int get_internal_desired_number_of_thread() { return m_desired_number_of_thread; }

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
    smp_tools_api(smp_tools_api const&)  = delete;
    void operator=(smp_tools_api const&) = delete;

private:
    //--------------------------------------------------------------------------------
    XSIGMA_API smp_tools_api();

    //--------------------------------------------------------------------------------
    XSIGMA_API void refresh_number_of_thread();

    //--------------------------------------------------------------------------------
    template <typename Config>
    smp_tools_api& operator<<(Config const& config)
    {
        this->initialize(config.max_number_of_threads_);
        this->set_backend(config.backend.c_str());
        this->set_nested_parallelism(config.nested_parallelism_);
        return *this;
    }

    /**
   * Desired number of threads
   */
    int m_desired_number_of_thread = 0;

    /**
   * Single backend implementation selected at compile-time
   */
    selected_smp_tools_impl backend_impl_;
};

}  // namespace smp
}  // namespace detail
}  // namespace xsigma

#endif
