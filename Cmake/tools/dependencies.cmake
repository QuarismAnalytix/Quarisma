# ============================================================================= Quarisma Centralized
# Dependency Management Module
# =============================================================================
# This module centralizes all third-party and system dependency management for Quarisma. It populates
# three cache variables based on enabled feature flags:
#
# 1. QUARISMA_DEPENDENCY_LIBS: Libraries to link against
# 2. QUARISMA_DEPENDENCY_INCLUDE_DIRS: Include directories for compilation
# 3. QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS: Compile definitions for feature availability
#
# This ensures consistent dependency management across all targets (Core library, tests, benchmarks,
# etc.). The module is included after all feature modules (CUDA, HIP, TBB, etc.) are loaded,
# allowing it to aggregate dependencies from all enabled features.
#
# NAMING CONVENTION: - CMake options and variables use QUARISMA_ENABLE_XXX (e.g., QUARISMA_ENABLE_CUDA)
# - C++ preprocessor macros use QUARISMA_HAS_XXX (e.g., QUARISMA_HAS_CUDA) - This module maps
# QUARISMA_ENABLE_XXX to QUARISMA_HAS_XXX for configure_file() processing
# =============================================================================

include_guard(GLOBAL)

# ============================================================================= Early Dependency
# Checks
# =============================================================================
# Check for optional dependencies and disable features if libraries are not found This must happen
# before feature flag mapping to ensure correct QUARISMA_HAS_* values

# Intel ITT API support - check if library is available
if(QUARISMA_ENABLE_ITTAPI)
  find_package(ITT)
  if(NOT ITT_FOUND)
    message(STATUS "ITT API not found - disabling QUARISMA_ENABLE_ITTAPI")
    set(QUARISMA_ENABLE_ITTAPI OFF CACHE BOOL "Enable Intel ITT API for VTune profiling." FORCE)
  endif()
endif()

# ============================================================================= Feature Flag Mapping
# =============================================================================
# Map CMake QUARISMA_ENABLE_* variables to QUARISMA_HAS_* compile definitions This ensures consistent
# naming: CMake uses ENABLE, C++ code uses HAS All feature flags are defined as compile definitions
# (1 or 0) rather than using configure_file()

# MEMKIND support
if(QUARISMA_ENABLE_MEMKIND)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_MEMKIND=1)
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_MEMKIND=0)
endif()

# MKL support
if(QUARISMA_ENABLE_MKL)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_MKL=1)
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_MKL=0)
endif()

# TBB support
if(QUARISMA_ENABLE_TBB)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_TBB=1)
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_TBB=0)
endif()

# Loguru support
if(QUARISMA_ENABLE_LOGURU)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_LOGURU=1)
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_LOGURU=0)
endif()

# Mimalloc support
if(QUARISMA_ENABLE_MIMALLOC)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_MIMALLOC=1)
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_MIMALLOC=0)
endif()

# NUMA support
if(QUARISMA_ENABLE_NUMA)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_NUMA=1)
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_NUMA=0)
endif()

# SVML support
if(QUARISMA_ENABLE_SVML)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_SVML=1)
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_SVML=0)
endif()

# CUDA support
if(QUARISMA_ENABLE_CUDA)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_CUDA=1)
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_CUDA=0)
endif()

# HIP support
if(QUARISMA_ENABLE_HIP)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_HIP=1)
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_HIP=0)
endif()

# Google Test support
if(QUARISMA_ENABLE_GTEST)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_GTEST=1)
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_GTEST=0)
endif()

# Kineto profiling support Note: QUARISMA_ENABLE_KINETO may have been disabled in
# ThirdParty/CMakeLists.txt if library not found
if(QUARISMA_ENABLE_KINETO)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_KINETO=1)
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_KINETO=0)
endif()

# Intel ITT API support Note: CMake uses QUARISMA_ENABLE_ITTAPI, but C++ code uses QUARISMA_HAS_ITT
# Note: QUARISMA_ENABLE_ITTAPI may have been disabled in dependencies.cmake if library not found
if(QUARISMA_ENABLE_ITTAPI)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_ITT=1)
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_ITT=0)
endif()

# Exception pointer support (detected by compiler checks in utils.cmake)
if(QUARISMA_USE_EXCEPTION_PTR)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_USE_EXCEPTION_PTR=1)
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_USE_EXCEPTION_PTR=0)
endif()

# Vectorization support (SSE, AVX, AVX2, AVX512 - detected by compiler checks in utils.cmake)
if(QUARISMA_SSE)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_SSE=1)
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_SSE=0)
endif()

if(QUARISMA_AVX)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_AVX=1)
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_AVX=0)
endif()

if(QUARISMA_AVX2)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_AVX2=1)
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_AVX2=0)
endif()

if(QUARISMA_AVX512)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_AVX512=1)
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_AVX512=0)
endif()

# Optional feature flags (not always set)
if(QUARISMA_SOBOL_1111)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_SOBOL_1111=1)
endif()

if(QUARISMA_LU_PIVOTING)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_LU_PIVOTING=1)
endif()

# Initialize the dependency lists if not already done
if(NOT DEFINED QUARISMA_DEPENDENCY_LIBS)
  set(QUARISMA_DEPENDENCY_LIBS "")
endif()

if(NOT DEFINED QUARISMA_DEPENDENCY_INCLUDE_DIRS)
  set(QUARISMA_DEPENDENCY_INCLUDE_DIRS "")
endif()

if(NOT DEFINED QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS)
  set(QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS "")
endif()

# ============================================================================= Mandatory Core
# Dependencies
# =============================================================================
# These libraries are always linked regardless of feature flags

# fmt - Header-only formatting library Quarisma::fmt is an alias to fmt::fmt-header-only (set in
# ThirdParty/CMakeLists.txt) This ensures compatibility with Kineto and avoids shared library issues
if(TARGET Quarisma::fmt)
  list(APPEND QUARISMA_DEPENDENCY_LIBS Quarisma::fmt)
  list(APPEND QUARISMA_DEPENDENCY_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/fmt/include")
  message(STATUS "Dependency: Quarisma::fmt (header-only) added to QUARISMA_DEPENDENCY_LIBS")
endif()

if(TARGET Quarisma::cpuinfo)
  list(APPEND QUARISMA_DEPENDENCY_LIBS Quarisma::cpuinfo)
  list(APPEND QUARISMA_DEPENDENCY_INCLUDE_DIRS
       "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/cpuinfo/include"
  )
  message(STATUS "Dependency: Quarisma::cpuinfo added to QUARISMA_DEPENDENCY_LIBS")
endif()

# ============================================================================= Optional Feature
# Dependencies
# =============================================================================
# These libraries are conditionally linked based on feature flags

# Magic Enum support
if(QUARISMA_ENABLE_MAGICENUM AND TARGET Quarisma::magic_enum)
  list(APPEND QUARISMA_DEPENDENCY_LIBS Quarisma::magic_enum)
  list(APPEND QUARISMA_DEPENDENCY_INCLUDE_DIRS
       "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/magic_enum/include"
  )
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_USE_MAGICENUM=1)
  message(STATUS "Dependency: Quarisma::magic_enum added to QUARISMA_DEPENDENCY_LIBS")
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_USE_MAGICENUM=0)
endif()

# Logging backend dependencies (mutually exclusive)
if(QUARISMA_ENABLE_LOGURU AND TARGET Quarisma::loguru)
  list(APPEND QUARISMA_DEPENDENCY_LIBS Quarisma::loguru)
  list(APPEND QUARISMA_DEPENDENCY_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/loguru")
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_USE_LOGURU=1)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_USE_GLOG=0)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_USE_NATIVE_LOGGING=0)
  message(STATUS "Dependency: Quarisma::loguru added to QUARISMA_DEPENDENCY_LIBS")
elseif(QUARISMA_ENABLE_GLOG AND TARGET Quarisma::glog)
  list(APPEND QUARISMA_DEPENDENCY_LIBS Quarisma::glog)
  list(APPEND QUARISMA_DEPENDENCY_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/glog/src")
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_USE_GLOG=1)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_USE_LOGURU=0)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_USE_NATIVE_LOGGING=0)
  message(STATUS "Dependency: Quarisma::glog added to QUARISMA_DEPENDENCY_LIBS")
elseif(QUARISMA_ENABLE_NATIVE_LOGGING)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_USE_NATIVE_LOGGING=1)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_USE_LOGURU=0)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_USE_GLOG=0)
  message(STATUS "Dependency: NATIVE logging backend selected")
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_USE_NATIVE_LOGGING=0)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_USE_LOGURU=0)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_USE_GLOG=0)
endif()

# TBB (Threading Building Blocks) support
if(QUARISMA_ENABLE_TBB)
  if(TARGET TBB::tbb)
    list(APPEND QUARISMA_DEPENDENCY_LIBS TBB::tbb)
    message(STATUS "Dependency: TBB::tbb added to QUARISMA_DEPENDENCY_LIBS")
  endif()

  if(TARGET TBB::tbbmalloc)
    list(APPEND QUARISMA_DEPENDENCY_LIBS TBB::tbbmalloc)
    message(STATUS "Dependency: TBB::tbbmalloc added to QUARISMA_DEPENDENCY_LIBS")
  endif()
endif()

# Mimalloc support
if(QUARISMA_ENABLE_MIMALLOC AND TARGET Quarisma::mimalloc)
  list(APPEND QUARISMA_DEPENDENCY_LIBS Quarisma::mimalloc)
  list(APPEND QUARISMA_DEPENDENCY_INCLUDE_DIRS
       "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/mimalloc/include"
  )
  message(STATUS "Dependency: Quarisma::mimalloc added to QUARISMA_DEPENDENCY_LIBS")
endif()

# PyTorch Kineto profiling library support
if(QUARISMA_ENABLE_KINETO)
  # Always add the compile definition and include directories when Kineto is enabled This allows
  # Kineto-specific code to be compiled
  list(APPEND QUARISMA_DEPENDENCY_INCLUDE_DIRS
       "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/kineto/libkineto/include"
  )

  # If the Kineto target exists, also link with it
  if(TARGET Quarisma::kineto)
    list(APPEND QUARISMA_DEPENDENCY_LIBS Quarisma::kineto)
    message(STATUS "Dependency: Quarisma::kineto added to QUARISMA_DEPENDENCY_LIBS")
  else()
    message(
      STATUS "Kineto enabled but library not found - Kineto code will compile but may not link"
    )
  endif()
endif()

# Intel ITT API support Note: QUARISMA_HAS_ITTAPI is defined via configure_file() in quarisma_features.h
# ITT library check already performed in early dependency checks section
if(QUARISMA_ENABLE_ITTAPI AND ITT_FOUND)
  list(APPEND QUARISMA_DEPENDENCY_LIBS ${ITT_LIBRARIES})
  list(APPEND QUARISMA_DEPENDENCY_INCLUDE_DIRS ${ITT_INCLUDE_DIR})
  message(STATUS "Dependency: ITT libraries added to QUARISMA_DEPENDENCY_LIBS")
endif()

# GPU support (CUDA or HIP) Note: CUDA and HIP libraries are already added by cuda.cmake and
# hip.cmake via list(APPEND QUARISMA_DEPENDENCY_LIBS ...) calls, so they're already in the list

# Compression support
if(QUARISMA_ENABLE_COMPRESSION)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_COMPRESSION=1)
  if(QUARISMA_COMPRESSION_TYPE_SNAPPY)
    list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_COMPRESSION_TYPE_SNAPPY=1)
  else()
    list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_COMPRESSION_TYPE_SNAPPY=0)
  endif()
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_COMPRESSION=0)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_COMPRESSION_TYPE_SNAPPY=0)
endif()

# Allocation statistics support (optional feature flag)
if(QUARISMA_ENABLE_ALLOCATION_STATS)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_ALLOCATION_STATS=1)
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_ALLOCATION_STATS=0)
endif()

# ============================================================================= Summary
# =============================================================================
message(STATUS "QUARISMA_DEPENDENCY_LIBS populated with: ${QUARISMA_DEPENDENCY_LIBS}")
message(STATUS "QUARISMA_DEPENDENCY_INCLUDE_DIRS populated with: ${QUARISMA_DEPENDENCY_INCLUDE_DIRS}")
message(
  STATUS
    "QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS populated with: ${QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS}"
)
