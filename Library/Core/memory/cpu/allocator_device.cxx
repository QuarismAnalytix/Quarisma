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
 */

#include "memory/cpu/allocator_device.h"

#include <cstddef>
#include <string>

#include "common/macros.h"
#include "memory/cpu/allocator.h"
#include "memory/helper/memory_allocator.h"  // for allocate, free
#include "util/exception.h"

#if XSIGMA_HAS_CUDA
#include <cuda_runtime.h>
#endif

namespace xsigma
{

std::string allocator_device::Name() const
{
    return "allocator_device";
}

void* allocator_device::allocate_raw(size_t alignment, size_t num_bytes)
{
    // Delegate to static allocate method for consistency
    (void)alignment;  // Alignment is handled internally
    //cppcheck-suppress syntaxError
    if XSIGMA_UNLIKELY (num_bytes == 0 || static_cast<std::ptrdiff_t>(num_bytes) < 0)
    {
        XSIGMA_THROW("allocating {} bytes", num_bytes);
    }

    void* ptr = nullptr;  //NOLINT

#if XSIGMA_HAS_CUDA
    cudaError_t const result = cudaMallocHost(&ptr, num_bytes);
    if (result != cudaSuccess)
    {
        XSIGMA_THROW("CUDA error in allocator_device::allocate_raw: {}", std::to_string(result));
    }
#else
    ptr = cpu::memory_allocator::allocate(num_bytes, 64);
#endif

    return ptr;
}

void allocator_device::deallocate_raw(void* ptr)
{
    if (ptr == nullptr)
    {
        return;
    }

#if XSIGMA_HAS_CUDA
    cudaError_t const result = cudaFreeHost(ptr);
    if (result != cudaSuccess)
    {
        // Log error but don't throw from free
        XSIGMA_LOG_ERROR("CUDA error in allocator_device::free: {}", std::to_string(result));
    }
#else
    cpu::memory_allocator::free(ptr);
#endif
}

allocator_memory_enum allocator_device::GetMemoryType() const noexcept
{
    return allocator_memory_enum::HOST_PINNED;
}
}  // namespace xsigma
