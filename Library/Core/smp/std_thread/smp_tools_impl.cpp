// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "smp/common/smp_tools_impl.h"

#include <charconv>
#include <cstdlib>  // For std::getenv()
#include <string>
#include <thread>  // For std::thread::hardware_concurrency()

#include "smp/std_thread/smp_tools_impl.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace xsigma
{
namespace detail
{
namespace smp
{
static int specified_num_threads_std;  // Default initialized to zero

//------------------------------------------------------------------------------
int number_of_threads_stdthread()
{
    return specified_num_threads_std ? specified_num_threads_std
                                     : std::thread::hardware_concurrency();
}

//------------------------------------------------------------------------------
template <>
void smp_tools_impl<backend_type::std_thread>::initialize(int num_threads)
{
    const int max_threads =
        smp_tools_impl<backend_type::std_thread>::estimated_default_number_of_threads();
    if (num_threads == 0)
    {
        const char* smp_num_threads = std::getenv("SMP_MAX_THREADS");
        if (smp_num_threads)
        {
            std::string str(smp_num_threads);
            auto        result = std::from_chars(str.data(), str.data() + str.size(), num_threads);
            if (result.ec != std::errc())
            {
                num_threads = 0;
            }
        }
        else
        {
            specified_num_threads_std = 0;
        }
    }
    if (num_threads > 0)
    {
        num_threads               = std::min(num_threads, max_threads);
        specified_num_threads_std = num_threads;
    }
}

//------------------------------------------------------------------------------
template <>
int smp_tools_impl<backend_type::std_thread>::estimated_number_of_threads()
{
    return specified_num_threads_std > 0
               ? specified_num_threads_std
               : smp_tools_impl<backend_type::std_thread>::estimated_default_number_of_threads();
}

//------------------------------------------------------------------------------
template <>
int smp_tools_impl<backend_type::std_thread>::estimated_default_number_of_threads()
{
#if defined(__EMSCRIPTEN_PTHREADS__) && (XSIGMA_WEBASSEMBLY_SMP_THREAD_POOL_SIZE > 0)
    int max_threads = XSIGMA_WEBASSEMBLY_SMP_THREAD_POOL_SIZE;
#else
    int max_threads = std::thread::hardware_concurrency();
#endif
    return max_threads;
}

//------------------------------------------------------------------------------
template <>
bool smp_tools_impl<backend_type::std_thread>::single_thread()
{
    return smp_thread_pool::instance().single_thread();
}

//------------------------------------------------------------------------------
template <>
bool smp_tools_impl<backend_type::std_thread>::is_parallel_scope()
{
    return smp_thread_pool::instance().is_parallel_scope();
}

}  // namespace smp
}  // namespace detail
}  // namespace xsigma
