// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef SMP_TOOLS_API_H
#define SMP_TOOLS_API_H

#include "common/export.h"
#include "smp/smp.h"

#include <memory>

#include "smp/common/smp_tools_impl.h"
#include "smp/std_thread/smp_tools_impl.h"

#if XSIGMA_HAS_TBB
#include "smp/tbb/smp_tools_impl.h"
#endif
#if XSIGMA_HAS_OPENMP
#include "smp/openmp/smp_tools_impl.h"
#endif

namespace conductor
{
namespace detail
{
namespace smp
{

using smp_tools_default_impl = smp_tools_impl<default_backend>;

class XSIGMA_VISIBILITY smp_tools_api
{
public:
  //--------------------------------------------------------------------------------
  XSIGMA_API static smp_tools_api& get_instance();

  //--------------------------------------------------------------------------------
  XSIGMA_API backend_type get_backend_type();

  //--------------------------------------------------------------------------------
  XSIGMA_API const char* get_backend();

  //--------------------------------------------------------------------------------
  XSIGMA_API bool set_backend(const char* type);

  //--------------------------------------------------------------------------------
  XSIGMA_API void initialize(int num_threads = 0);

  //--------------------------------------------------------------------------------
  XSIGMA_API int get_estimated_number_of_threads();

  //--------------------------------------------------------------------------------
  XSIGMA_API int get_estimated_default_number_of_threads();

  //------------------------------------------------------------------------------
  XSIGMA_API void set_nested_parallelism(bool is_nested);

  //--------------------------------------------------------------------------------
  XSIGMA_API bool get_nested_parallelism();

  //--------------------------------------------------------------------------------
  XSIGMA_API bool is_parallel_scope();

  //--------------------------------------------------------------------------------
  XSIGMA_API bool get_single_thread();

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
  template <typename FunctorInternal>
  void For(size_t first, size_t last, size_t grain, FunctorInternal& fi)
  {
    switch (m_activated_backend)
    {
      case backend_type::std_thread:
        m_std_thread_backend->For(first, last, grain, fi);
        break;
      case backend_type::TBB:
        m_tbb_backend->For(first, last, grain, fi);
        break;
      case backend_type::OpenMP:
        m_openmp_backend->For(first, last, grain, fi);
        break;
    }
  }


  // disable copying
  smp_tools_api(smp_tools_api const&) = delete;
  void operator=(smp_tools_api const&) = delete;

protected:
  //--------------------------------------------------------------------------------
  XSIGMA_API static void class_initialize();
  XSIGMA_API static void class_finalize();
  friend class smp_tools_api_initialize;

private:
  //--------------------------------------------------------------------------------
  XSIGMA_API smp_tools_api();

  //--------------------------------------------------------------------------------
  XSIGMA_API void refresh_number_of_thread();

  //--------------------------------------------------------------------------------
  template <typename Config>
  smp_tools_api& operator<<(Config const& config)
  {
    this->initialize(config.max_number_of_threads);
    this->set_backend(config.backend.c_str());
    this->set_nested_parallelism(config.nested_parallelism);
    return *this;
  }

  /**
   * Indicate which backend to use.
   */
  backend_type m_activated_backend = default_backend;

  /**
   * Max threads number
   */
  int m_desired_number_of_thread = 0;


  /**
   * std_thread backend
   */

  std::unique_ptr<smp_tools_impl<backend_type::std_thread>> m_std_thread_backend;


  /**
   * TBB backend
   */
#if XSIGMA_HAS_TBB
  std::unique_ptr<smp_tools_impl<backend_type::TBB>> m_tbb_backend;
#else
  std::unique_ptr<smp_tools_default_impl> m_tbb_backend;
#endif

  /**
   * OpenMP backend
   */
#if XSIGMA_HAS_OPENMP
  std::unique_ptr<smp_tools_impl<backend_type::OpenMP>> m_openmp_backend;
#else
  std::unique_ptr<smp_tools_default_impl> m_openmp_backend;
#endif
};

//--------------------------------------------------------------------------------
class XSIGMA_VISIBILITY smp_tools_api_initialize
{
public:
  XSIGMA_API smp_tools_api_initialize();
  XSIGMA_API ~smp_tools_api_initialize();
};

//--------------------------------------------------------------------------------
// This instance will show up in any translation unit that uses smp_tools_api singleton.
static smp_tools_api_initialize smp_tools_api_initializer;

} // namespace smp
} // namespace detail
} // namespace conductor

#endif
/* VTK-HeaderTest-Exclude: smp_tools_api.h */
