include_guard(GLOBAL)

# =============================================================================
# Feature Flag Mapping
# =============================================================================
# Map CMake XSIGMA_ENABLE_* variables to XSIGMA_HAS_* compile definitions This ensures consistent
# naming: CMake uses ENABLE, C++ code uses HAS All feature flags are defined as compile definitions
# (1 or 0) rather than using configure_file()

function(compile_definition enable_flag)
  string(REPLACE "ENABLE" "HAS" definition_name "${enable_flag}")
  if(NOT DEFINED ${enable_flag})
    set(_enabled OFF)
  else()
    set(_enabled ${${enable_flag}})
  endif()
  if(_enabled)
    set(_value 1)
  else()
    set(_value 0)
  endif()
  list(APPEND XSIGMA_DEPENDENCY_COMPILE_DEFINITIONS "${definition_name}=${_value}")
  set(XSIGMA_DEPENDENCY_COMPILE_DEFINITIONS "${XSIGMA_DEPENDENCY_COMPILE_DEFINITIONS}" PARENT_SCOPE)
endfunction()

# MKL support
compile_definition(XSIGMA_ENABLE_MKL)

# TBB support
compile_definition(XSIGMA_ENABLE_TBB)

# Loguru support
compile_definition(XSIGMA_ENABLE_LOGURU)

# GLOG support
compile_definition(XSIGMA_ENABLE_GLOG)

# Native logging support
compile_definition(XSIGMA_ENABLE_NATIVE_LOGGING)

# Mimalloc support
compile_definition(XSIGMA_ENABLE_MIMALLOC)

# NUMA support
compile_definition(XSIGMA_ENABLE_NUMA)

# SVML support
compile_definition(XSIGMA_ENABLE_SVML)

# CUDA support
compile_definition(XSIGMA_ENABLE_CUDA)

# HIP support
compile_definition(XSIGMA_ENABLE_HIP)

# ROCm support
compile_definition(XSIGMA_ENABLE_ROCM)

# Google Test support
compile_definition(XSIGMA_ENABLE_GTEST)

# Kineto support
compile_definition(XSIGMA_ENABLE_KINETO)

# Intel ITT API support
compile_definition(XSIGMA_ENABLE_ITT)

# Native profiler support (derived from XSIGMA_PROFILER_TYPE)
compile_definition(XSIGMA_ENABLE_NATIVE_PROFILER)

# OpenMP support
compile_definition(XSIGMA_ENABLE_OPENMP)

# Experimental features support
compile_definition(XSIGMA_ENABLE_EXPERIMENTAL)

# Magic Enum support
compile_definition(XSIGMA_ENABLE_MAGICENUM)

# Exception pointer support (detected by compiler checks in utils.cmake)
if(XSIGMA_HAS_EXCEPTION_PTR)
  list(APPEND XSIGMA_DEPENDENCY_COMPILE_DEFINITIONS XSIGMA_HAS_EXCEPTION_PTR=1)
else()
  list(APPEND XSIGMA_DEPENDENCY_COMPILE_DEFINITIONS XSIGMA_HAS_EXCEPTION_PTR=0)
endif()

# Vectorization support (SSE, AVX, AVX2, AVX512 - detected by compiler checks in utils.cmake)
if(XSIGMA_SSE)
  list(APPEND XSIGMA_DEPENDENCY_COMPILE_DEFINITIONS XSIGMA_SSE=1)
else()
  list(APPEND XSIGMA_DEPENDENCY_COMPILE_DEFINITIONS XSIGMA_SSE=0)
endif()

if(XSIGMA_AVX)
  list(APPEND XSIGMA_DEPENDENCY_COMPILE_DEFINITIONS XSIGMA_AVX=1)
else()
  list(APPEND XSIGMA_DEPENDENCY_COMPILE_DEFINITIONS XSIGMA_AVX=0)
endif()

if(XSIGMA_AVX2)
  list(APPEND XSIGMA_DEPENDENCY_COMPILE_DEFINITIONS XSIGMA_AVX2=1)
else()
  list(APPEND XSIGMA_DEPENDENCY_COMPILE_DEFINITIONS XSIGMA_AVX2=0)
endif()

if(XSIGMA_AVX512)
  list(APPEND XSIGMA_DEPENDENCY_COMPILE_DEFINITIONS XSIGMA_AVX512=1)
else()
  list(APPEND XSIGMA_DEPENDENCY_COMPILE_DEFINITIONS XSIGMA_AVX512=0)
endif()

# Optional feature flags (not always set)
if(XSIGMA_SOBOL_1111)
  list(APPEND XSIGMA_DEPENDENCY_COMPILE_DEFINITIONS XSIGMA_SOBOL_1111=1)
endif()

if(XSIGMA_LU_PIVOTING)
  list(APPEND XSIGMA_DEPENDENCY_COMPILE_DEFINITIONS XSIGMA_LU_PIVOTING=1)
endif()

# Compression support
compile_definition(XSIGMA_ENABLE_COMPRESSION)
if(XSIGMA_ENABLE_COMPRESSION)
  if(XSIGMA_COMPRESSION_TYPE_SNAPPY)
    list(APPEND XSIGMA_DEPENDENCY_COMPILE_DEFINITIONS XSIGMA_COMPRESSION_TYPE_SNAPPY=1)
  else()
    list(APPEND XSIGMA_DEPENDENCY_COMPILE_DEFINITIONS XSIGMA_COMPRESSION_TYPE_SNAPPY=0)
  endif()
else()
  list(APPEND XSIGMA_DEPENDENCY_COMPILE_DEFINITIONS XSIGMA_COMPRESSION_TYPE_SNAPPY=0)
endif()

# Allocation statistics support (optional feature flag)
compile_definition(XSIGMA_ENABLE_ALLOCATION_STATS)

if(XSIGMA_ENABLE_NATIVE_PROFILER)
  list(APPEND XSIGMA_DEPENDENCY_COMPILE_DEFINITIONS XSIGMA_HAS_NATIVE_PROFILER=1)
else()
  list(APPEND XSIGMA_DEPENDENCY_COMPILE_DEFINITIONS XSIGMA_HAS_NATIVE_PROFILER=0)
endif()

# Threading support (detected by threads.cmake)
# These flags indicate which threading library is available on the platform
if(XSIGMA_USE_PTHREADS)
  list(APPEND XSIGMA_DEPENDENCY_COMPILE_DEFINITIONS XSIGMA_HAS_PTHREADS=1)
else()
  list(APPEND XSIGMA_DEPENDENCY_COMPILE_DEFINITIONS XSIGMA_HAS_PTHREADS=0)
endif()

if(XSIGMA_USE_WIN32_THREADS)
  list(APPEND XSIGMA_DEPENDENCY_COMPILE_DEFINITIONS XSIGMA_HAS_WIN32_THREADS=1)
else()
  list(APPEND XSIGMA_DEPENDENCY_COMPILE_DEFINITIONS XSIGMA_HAS_WIN32_THREADS=0)
endif()

message("XSIGMA_DEPENDENCY_COMPILE_DEFINITIONS: ${XSIGMA_DEPENDENCY_COMPILE_DEFINITIONS}")
