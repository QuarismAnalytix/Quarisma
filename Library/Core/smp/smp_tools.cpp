// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "smp_tools.h"

//------------------------------------------------------------------------------
void smp_tools::initialize(int num_threads)
{
    auto& SMPToolsAPI = xsigma::detail::smp::smp_tools_api::instance();
    SMPToolsAPI.initialize(num_threads);
}

//------------------------------------------------------------------------------
int smp_tools::estimated_number_of_threads()
{
    auto& SMPToolsAPI = xsigma::detail::smp::smp_tools_api::instance();
    return SMPToolsAPI.estimated_number_of_threads();
}

//------------------------------------------------------------------------------
int smp_tools::estimated_default_number_of_threads()
{
    auto& SMPToolsAPI = xsigma::detail::smp::smp_tools_api::instance();
    return SMPToolsAPI.estimated_default_number_of_threads();
}

//------------------------------------------------------------------------------
void smp_tools::set_nested_parallelism(bool is_nested)
{
    auto& SMPToolsAPI = xsigma::detail::smp::smp_tools_api::instance();
    SMPToolsAPI.set_nested_parallelism(is_nested);
}

//------------------------------------------------------------------------------
bool smp_tools::nested_parallelism()
{
    auto& SMPToolsAPI = xsigma::detail::smp::smp_tools_api::instance();
    return SMPToolsAPI.nested_parallelism();
}

//------------------------------------------------------------------------------
bool smp_tools::is_parallel_scope()
{
    auto& SMPToolsAPI = xsigma::detail::smp::smp_tools_api::instance();
    return SMPToolsAPI.is_parallel_scope();
}

//------------------------------------------------------------------------------
bool smp_tools::single_thread()
{
    auto& SMPToolsAPI = xsigma::detail::smp::smp_tools_api::instance();
    return SMPToolsAPI.single_thread();
}
