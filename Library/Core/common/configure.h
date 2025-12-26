/*
 * Some parts of this implementation were inspired by code from VTK
 * (The Visualization Toolkit), distributed under a BSD-style license.
 * See LICENSE for details.
 */
#ifndef __quarisma_configure_h__
#define __quarisma_configure_h__

// Feature flags are now defined via CMake compile definitions (QUARISMA_HAS_*)
// See Cmake/tools/dependencies.cmake for the mapping from QUARISMA_ENABLE_* to QUARISMA_HAS_*

#include "quarisma_threads.h"
#include "quarisma_version_macros.h"

// NUMA is only enabled on Linux
#if defined(__linux__) && QUARISMA_HAS_NUMA
#define QUARISMA_NUMA_ENABLED
#endif

// Vectorization support
#if (QUARISMA_AVX512 == 1) || (QUARISMA_AVX2 == 1) || (QUARISMA_AVX == 1) || (QUARISMA_SSE == 1)
#define QUARISMA_VECTORIZED
#endif

// Compression configuration
#if QUARISMA_HAS_COMPRESSION == 1
#if QUARISMA_COMPRESSION_TYPE_SNAPPY == 1
#define QUARISMA_COMPRESSION_TYPE_SNAPPY
#endif
#endif

#endif  // __quarisma_configure_h__
