// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @file smp.h
 * @brief SMP (Shared Memory Parallelism) configuration and backend definitions
 */

#ifndef SMP_H
#define SMP_H

// Maximum number of backends
#define SMP_MAX_BACKENDS_NB 3

namespace conductor
{
namespace detail
{
namespace smp
{

/**
 * Backend types available for SMP execution
 */
enum class backend_type
{
    std_thread = 0,
    TBB        = 1,
    OpenMP     = 2
};

// Default backend selection
// Uses XSIGMA_HAS_TBB and XSIGMA_HAS_OPENMP which are defined by CMake
// as compile definitions with explicit values (1 or 0)
#if XSIGMA_HAS_TBB
constexpr backend_type default_backend = backend_type::TBB;
#elif XSIGMA_HAS_OPENMP
constexpr backend_type default_backend = backend_type::OpenMP;
#else
constexpr backend_type default_backend = backend_type::std_thread;
#endif

}  // namespace smp
}  // namespace detail
}  // namespace conductor

#endif  // SMP_H
