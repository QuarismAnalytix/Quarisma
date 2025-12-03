/*
 * Some parts of this implementation were inspired by code from VTK
 * (The Visualization Toolkit), distributed under a BSD-style license.
 * See LICENSE for details.
 */
#ifndef __xsigma_configure_h__
#define __xsigma_configure_h__

// Feature flags are now defined via CMake compile definitions (XSIGMA_HAS_*)
// See Cmake/tools/dependencies.cmake for the mapping from XSIGMA_ENABLE_* to XSIGMA_HAS_*

// NUMA is only enabled on Linux
#if defined(__linux__) && XSIGMA_HAS_NUMA
#define XSIGMA_NUMA_ENABLED
#endif

// Vectorization support
#if (XSIGMA_AVX512 == 1) || (XSIGMA_AVX2 == 1) || (XSIGMA_AVX == 1) || (XSIGMA_SSE == 1)
#define XSIGMA_VECTORIZED
#endif

// Compression configuration
#if XSIGMA_HAS_COMPRESSION == 1
#if XSIGMA_COMPRESSION_TYPE_SNAPPY == 1
#define XSIGMA_COMPRESSION_TYPE_SNAPPY
#endif
#endif

#endif  // __xsigma_configure_h__
