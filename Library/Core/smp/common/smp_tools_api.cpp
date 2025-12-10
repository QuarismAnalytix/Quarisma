// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "smp/common/smp_tools_api.h"

#include <algorithm>  // For std::toupper
#include <cstdlib>    // For std::getenv
#include <iostream>   // For std::cerr
#include <string>     // For std::string

#include "smp/smp.h"

namespace xsigma
{
namespace detail
{
namespace smp
{

//------------------------------------------------------------------------------
smp_tools_api::smp_tools_api()
{
    // Single backend instance is created as member variable (no dynamic allocation needed)
    // Set max thread number from env
    this->refresh_number_of_thread();
}

//------------------------------------------------------------------------------
// Singleton instance
static smp_tools_api* smp_tools_api_instance = nullptr;

//------------------------------------------------------------------------------
smp_tools_api& smp_tools_api::instance()
{
    if (!smp_tools_api_instance)
    {
        smp_tools_api_instance = new smp_tools_api();
    }
    return *smp_tools_api_instance;
}

//------------------------------------------------------------------------------
backend_type smp_tools_api::get_backend_type()
{
    return selected_backend_tools;
}

//------------------------------------------------------------------------------
const char* smp_tools_api::get_backend()
{
    switch (selected_backend_tools)
    {
    case backend_type::std_thread:
        return "std_thread";
    case backend_type::TBB:
        return "TBB";
    case backend_type::OpenMP:
        return "OpenMP";
    }
    return nullptr;
}

//------------------------------------------------------------------------------
bool smp_tools_api::set_backend(const char* type)
{
    // Backend is selected at compile-time, so we just verify the requested backend
    // matches the compile-time selected one
    std::string backend(type);
    std::transform(backend.cbegin(), backend.cend(), backend.begin(), ::toupper);

    const char* current_backend = this->get_backend();
    std::string current_backend_upper(current_backend);
    std::transform(
        current_backend_upper.cbegin(),
        current_backend_upper.cend(),
        current_backend_upper.begin(),
        ::toupper);

    if (backend == "STDTHREAD")
    {
        backend = "STD_THREAD";
    }

    if (backend != current_backend_upper)
    {
        std::cerr << "WARNING: Backend selection is compile-time only. Requested \"" << type
                  << "\" but using \"" << current_backend << "\".\n";
        return false;
    }

    this->refresh_number_of_thread();
    return true;
}

//------------------------------------------------------------------------------
void smp_tools_api::initialize(int num_threads)
{
    this->m_desired_number_of_thread = num_threads;
    this->refresh_number_of_thread();
}

//------------------------------------------------------------------------------
void smp_tools_api::refresh_number_of_thread()
{
    const int num_threads = this->m_desired_number_of_thread;
    backend_impl_.initialize(num_threads);
}

//------------------------------------------------------------------------------
int smp_tools_api::estimated_default_number_of_threads()
{
    return backend_impl_.estimated_default_number_of_threads();
}

//------------------------------------------------------------------------------
int smp_tools_api::estimated_number_of_threads()
{
    return backend_impl_.estimated_number_of_threads();
}

//------------------------------------------------------------------------------
void smp_tools_api::set_nested_parallelism(bool is_nested)
{
    backend_impl_.set_nested_parallelism(is_nested);
}

//------------------------------------------------------------------------------
bool smp_tools_api::nested_parallelism()
{
    return backend_impl_.nested_parallelism();
}

//------------------------------------------------------------------------------
bool smp_tools_api::is_parallel_scope()
{
    return backend_impl_.is_parallel_scope();
}

//------------------------------------------------------------------------------
bool smp_tools_api::single_thread()
{
    return backend_impl_.single_thread();
}

}  // namespace smp
}  // namespace detail
}  // namespace xsigma
