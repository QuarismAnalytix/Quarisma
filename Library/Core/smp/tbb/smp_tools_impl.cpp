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

#include "smp/common/smp_tools_impl.h"

#include <charconv>
#include <cstdlib>  // For std::getenv()
#include <memory>   // For std::unique_ptr
#include <mutex>    // For std::mutex
#include <stack>    // For std::stack
#include <string>

#include "smp/tbb/smp_tools_impl.h"

#ifdef _MSC_VER
#pragma push_macro("__TBB_NO_IMPLICIT_LINKAGE")
#define __TBB_NO_IMPLICIT_LINKAGE 1
#endif

#include <tbb/task_arena.h>  // For tbb:task_arena

#ifdef _MSC_VER
#pragma pop_macro("__TBB_NO_IMPLICIT_LINKAGE")
#endif

namespace xsigma
{
namespace detail
{
namespace smp
{

static std::unique_ptr<tbb::task_arena> task_arena;
static std::unique_ptr<std::mutex>      xsigma_smp_tools_cs;
static std::unique_ptr<std::stack<int>> thread_id_stack;
static std::unique_ptr<std::mutex>      thread_id_stack_lock;
static int                              specified_num_threads_tbb;  // Default initialized to zero

//------------------------------------------------------------------------------
// Must NOT be initialized. Default initialization to zero is necessary.
// NOTE: This variable must NOT be static - it needs to be visible across
// translation units for proper initialization/deinitialization tracking.
static unsigned int smp_tools_impl_tbb_initialize_count;

//------------------------------------------------------------------------------
smp_tools_impl_tbb_initialize::smp_tools_impl_tbb_initialize()
{
    if (++smp_tools_impl_tbb_initialize_count == 1)
    {
        task_arena           = std::make_unique<tbb::task_arena>();
        xsigma_smp_tools_cs  = std::make_unique<std::mutex>();
        thread_id_stack      = std::make_unique<std::stack<int>>();
        thread_id_stack_lock = std::make_unique<std::mutex>();
    }
}

//------------------------------------------------------------------------------
smp_tools_impl_tbb_initialize::~smp_tools_impl_tbb_initialize() = default;

//------------------------------------------------------------------------------
template <>
smp_tools_impl<backend_type::TBB>::smp_tools_impl() : nested_activated_(true)
{
}

//------------------------------------------------------------------------------
template <>
void smp_tools_impl<backend_type::TBB>::initialize(int num_threads)
{
    xsigma_smp_tools_cs->lock();

    if (num_threads == 0)
    {
        const char* smp_num_threads = std::getenv("SMP_MAX_THREADS");
        if (smp_num_threads != nullptr)
        {
            std::string str(smp_num_threads);
            auto        result = std::from_chars(str.data(), str.data() + str.size(), num_threads);
            if (result.ec != std::errc())
            {
                num_threads = 0;
            }
        }
        else if (task_arena->is_active())
        {
            task_arena->terminate();
            specified_num_threads_tbb = 0;
        }
    }
    if (num_threads > 0 &&
        num_threads <= smp_tools_impl<backend_type::TBB>::estimated_default_number_of_threads())
    {
        if (task_arena->is_active())
        {
            task_arena->terminate();
        }
        task_arena->initialize(num_threads);
        specified_num_threads_tbb = num_threads;
    }

    xsigma_smp_tools_cs->unlock();
}

//------------------------------------------------------------------------------
template <>
int smp_tools_impl<backend_type::TBB>::estimated_number_of_threads()
{
    return specified_num_threads_tbb > 0
               ? specified_num_threads_tbb
               : smp_tools_impl<backend_type::TBB>::estimated_default_number_of_threads();
}

//------------------------------------------------------------------------------
template <>
int smp_tools_impl<backend_type::TBB>::estimated_default_number_of_threads()
{
    return task_arena->max_concurrency();
}

//------------------------------------------------------------------------------
template <>
bool smp_tools_impl<backend_type::TBB>::single_thread()
{
    return thread_id_stack->top() == tbb::this_task_arena::current_thread_index();
}

//------------------------------------------------------------------------------
void smp_tools_impl_for_tbb(
    size_t                   first,
    size_t                   last,
    size_t                   grain,
    execute_functor_ptr_type functor_executer,
    void*                    functor)
{
    thread_id_stack_lock->lock();
    thread_id_stack->emplace(tbb::this_task_arena::current_thread_index());
    thread_id_stack_lock->unlock();

    if (task_arena->is_active())
    {
        task_arena->execute([&] { functor_executer(functor, first, last, grain); });
    }
    else
    {
        functor_executer(functor, first, last, grain);
    }

    thread_id_stack_lock->lock();
    thread_id_stack->pop();
    thread_id_stack_lock->unlock();
}

}  // namespace smp
}  // namespace detail
}  // namespace xsigma
