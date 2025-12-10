// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "smp/common/smp_tools_api.h"
#include "smp/smp.h"

#include <algorithm> // For std::toupper
#include <cstdlib>   // For std::getenv
#include <iostream>  // For std::cerr
#include <string>    // For std::string

namespace conductor
{
namespace detail
{
namespace smp
{

//------------------------------------------------------------------------------
smp_tools_api::smp_tools_api()
{

  this->m_std_thread_backend = std::make_unique<smp_tools_impl<backend_type::std_thread>>();

#if XSIGMA_HAS_TBB
  this->m_tbb_backend = std::make_unique<smp_tools_impl<backend_type::TBB>>();
#endif
#if XSIGMA_HAS_OPENMP
  this->m_openmp_backend = std::make_unique<smp_tools_impl<backend_type::OpenMP>>();
#endif

  // Set backend from env if set
  const char* smp_backend_in_use = std::getenv("SMP_BACKEND_IN_USE");
  if (smp_backend_in_use)
  {
    this->set_backend(smp_backend_in_use);
  }

  // Set max thread number from env
  this->refresh_number_of_thread();
}

//------------------------------------------------------------------------------
// Must NOT be initialized. Default initialization to zero is necessary.
smp_tools_api* smp_tools_api_instance_as_pointer;

//------------------------------------------------------------------------------
smp_tools_api& smp_tools_api::get_instance()
{
  return *smp_tools_api_instance_as_pointer;
}

//------------------------------------------------------------------------------
void smp_tools_api::class_initialize()
{
  if (!smp_tools_api_instance_as_pointer)
  {
    smp_tools_api_instance_as_pointer = new smp_tools_api;
  }
}

//------------------------------------------------------------------------------
void smp_tools_api::class_finalize()
{
  delete smp_tools_api_instance_as_pointer;
  smp_tools_api_instance_as_pointer = nullptr;
}

//------------------------------------------------------------------------------
backend_type smp_tools_api::get_backend_type()
{
  return this->m_activated_backend;
}

//------------------------------------------------------------------------------
const char* smp_tools_api::get_backend()
{
  switch (this->m_activated_backend)
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
  std::string backend(type);
  std::transform(backend.cbegin(), backend.cend(), backend.begin(), ::toupper);
  if (backend == "STDTHREAD" && this->m_std_thread_backend)
  {
    this->m_activated_backend = backend_type::std_thread;
  }
  else if (backend == "TBB" && this->m_tbb_backend)
  {
    this->m_activated_backend = backend_type::TBB;
  }
  else if (backend == "OPENMP" && this->m_openmp_backend)
  {
    this->m_activated_backend = backend_type::OpenMP;
  }
  else
  {
    std::cerr << "WARNING: tried to use a non implemented SMPTools backend \"" << type << "\"!\n";
    std::cerr << "The available backends are:"
              << (this->m_std_thread_backend ? " \"std_thread\"" : "")
              << (this->m_tbb_backend ? " \"TBB\"" : "") << (this->m_openmp_backend ? " \"OpenMP\"" : "")
              << "\n";
    std::cerr << "Using " << this->get_backend() << " instead." << std::endl;
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
  switch (this->m_activated_backend)
  {
    case backend_type::std_thread:
      this->m_std_thread_backend->initialize(num_threads);
      break;
    case backend_type::TBB:
      this->m_tbb_backend->initialize(num_threads);
      break;
    case backend_type::OpenMP:
      this->m_openmp_backend->initialize(num_threads);
      break;
  }
}

//------------------------------------------------------------------------------
int smp_tools_api::get_estimated_default_number_of_threads()
{
  switch (this->m_activated_backend)
  {
    case backend_type::std_thread:
      return this->m_std_thread_backend->get_estimated_default_number_of_threads();
    case backend_type::TBB:
      return this->m_tbb_backend->get_estimated_default_number_of_threads();
    case backend_type::OpenMP:
      return this->m_openmp_backend->get_estimated_default_number_of_threads();
  }
  return 0;
}

//------------------------------------------------------------------------------
int smp_tools_api::get_estimated_number_of_threads()
{
  switch (this->m_activated_backend)
  {
    case backend_type::std_thread:
      return this->m_std_thread_backend->get_estimated_number_of_threads();
    case backend_type::TBB:
      return this->m_tbb_backend->get_estimated_number_of_threads();
    case backend_type::OpenMP:
      return this->m_openmp_backend->get_estimated_number_of_threads();
  }
  return 0;
}

//------------------------------------------------------------------------------
void smp_tools_api::set_nested_parallelism(bool is_nested)
{
  switch (this->m_activated_backend)
  {
    case backend_type::std_thread:
      this->m_std_thread_backend->set_nested_parallelism(is_nested);
      break;
    case backend_type::TBB:
      this->m_tbb_backend->set_nested_parallelism(is_nested);
      break;
    case backend_type::OpenMP:
      this->m_openmp_backend->set_nested_parallelism(is_nested);
      break;
  }
}

//------------------------------------------------------------------------------
bool smp_tools_api::get_nested_parallelism()
{
  switch (this->m_activated_backend)
  {
    case backend_type::std_thread:
      return this->m_std_thread_backend->get_nested_parallelism();
    case backend_type::TBB:
      return this->m_tbb_backend->get_nested_parallelism();
    case backend_type::OpenMP:
      return this->m_openmp_backend->get_nested_parallelism();
  }
  return false;
}

//------------------------------------------------------------------------------
bool smp_tools_api::is_parallel_scope()
{
  switch (this->m_activated_backend)
  {
    case backend_type::std_thread:
      return this->m_std_thread_backend->is_parallel_scope();
    case backend_type::TBB:
      return this->m_tbb_backend->is_parallel_scope();
    case backend_type::OpenMP:
      return this->m_openmp_backend->is_parallel_scope();
  }
  return false;
}

//------------------------------------------------------------------------------
bool smp_tools_api::get_single_thread()
{
  switch (this->m_activated_backend)
  {
    case backend_type::std_thread:
      return this->m_std_thread_backend->get_single_thread();
    case backend_type::TBB:
      return this->m_tbb_backend->get_single_thread();
    case backend_type::OpenMP:
      return this->m_openmp_backend->get_single_thread();
    default:
      return false;
  }
}

//------------------------------------------------------------------------------
// Must NOT be initialized. Default initialization to zero is necessary.
unsigned int smp_tools_api_initialize_count;

//------------------------------------------------------------------------------
smp_tools_api_initialize::smp_tools_api_initialize()
{
  if (++smp_tools_api_initialize_count == 1)
  {
    smp_tools_api::class_initialize();
  }
}

//------------------------------------------------------------------------------
smp_tools_api_initialize::~smp_tools_api_initialize()
{
  if (--smp_tools_api_initialize_count == 0)
  {
    smp_tools_api::class_finalize();
  }
}

} // namespace smp
} // namespace detail
} // namespace conductor
