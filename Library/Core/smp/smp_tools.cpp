// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "smp_tools.h"

//------------------------------------------------------------------------------
const char* smp_tools::get_backend()
{
    auto& SMPToolsAPI = conductor::detail::smp::smp_tools_api::get_instance();
    return SMPToolsAPI.get_backend();
}

//------------------------------------------------------------------------------
bool smp_tools::set_backend(const char* backend)
{
    auto& SMPToolsAPI = conductor::detail::smp::smp_tools_api::get_instance();
    return SMPToolsAPI.set_backend(backend);
}

//------------------------------------------------------------------------------
void smp_tools::initialize(int num_threads)
{
    auto& SMPToolsAPI = conductor::detail::smp::smp_tools_api::get_instance();
    SMPToolsAPI.initialize(num_threads);
}

//------------------------------------------------------------------------------
int smp_tools::get_estimated_number_of_threads()
{
    auto& SMPToolsAPI = conductor::detail::smp::smp_tools_api::get_instance();
    return SMPToolsAPI.get_estimated_number_of_threads();
}

//------------------------------------------------------------------------------
int smp_tools::get_estimated_default_number_of_threads()
{
    auto& SMPToolsAPI = conductor::detail::smp::smp_tools_api::get_instance();
    return SMPToolsAPI.get_estimated_default_number_of_threads();
}

//------------------------------------------------------------------------------
void smp_tools::set_nested_parallelism(bool is_nested)
{
    auto& SMPToolsAPI = conductor::detail::smp::smp_tools_api::get_instance();
    SMPToolsAPI.set_nested_parallelism(is_nested);
}

//------------------------------------------------------------------------------
bool smp_tools::get_nested_parallelism()
{
    auto& SMPToolsAPI = conductor::detail::smp::smp_tools_api::get_instance();
    return SMPToolsAPI.get_nested_parallelism();
}

//------------------------------------------------------------------------------
bool smp_tools::is_parallel_scope()
{
    auto& SMPToolsAPI = conductor::detail::smp::smp_tools_api::get_instance();
    return SMPToolsAPI.is_parallel_scope();
}

//------------------------------------------------------------------------------
bool smp_tools::get_single_thread()
{
    auto& SMPToolsAPI = conductor::detail::smp::smp_tools_api::get_instance();
    return SMPToolsAPI.get_single_thread();
}
