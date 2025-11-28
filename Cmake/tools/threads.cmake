# =============================================================================
# XSigma Threading Library Detection and Configuration
# =============================================================================
# This module detects the threading library available on the platform and sets
# appropriate CMake variables and compiler definitions:
# - XSIGMA_USE_PTHREADS: Set to 1 on Unix-like systems with pthreads support
# - XSIGMA_USE_WIN32_THREADS: Set to 1 on Windows systems
# - XSIGMA_MAX_THREADS: Maximum number of threads supported (default: 64)
#
# The module also ensures the appropriate thread library is linked to targets.
# =============================================================================

cmake_minimum_required(VERSION 3.16)

# Include guard to prevent multiple inclusions
include_guard(GLOBAL)

message(STATUS "XSigma: Detecting threading library support...")

# =============================================================================
# Thread Library Detection
# =============================================================================

# Use CMake's built-in thread detection
find_package(Threads REQUIRED)

# =============================================================================
# Platform-Specific Thread Configuration
# =============================================================================

# Initialize thread flags to 0 (following XSigma convention of explicit values)
set(XSIGMA_USE_PTHREADS 0 CACHE INTERNAL "Use POSIX threads (pthreads)")
set(XSIGMA_USE_WIN32_THREADS 0 CACHE INTERNAL "Use Win32 threads")

# Detect threading model based on platform and CMake's thread detection
if(WIN32)
  # Windows platform uses Win32 threads
  set(XSIGMA_USE_WIN32_THREADS 1 CACHE INTERNAL "Use Win32 threads" FORCE)
  set(XSIGMA_USE_PTHREADS 0 CACHE INTERNAL "Use POSIX threads (pthreads)" FORCE)
  message(STATUS "XSigma: Threading model: Win32 threads")
  
elseif(CMAKE_USE_PTHREADS_INIT OR CMAKE_THREAD_LIBS_INIT MATCHES "pthread")
  # Unix-like systems (Linux, macOS, BSD, etc.) with pthreads
  set(XSIGMA_USE_PTHREADS 1 CACHE INTERNAL "Use POSIX threads (pthreads)" FORCE)
  set(XSIGMA_USE_WIN32_THREADS 0 CACHE INTERNAL "Use Win32 threads" FORCE)
  message(STATUS "XSigma: Threading model: POSIX threads (pthreads)")
  
else()
  # Fallback: no threading support detected
  message(WARNING "XSigma: No threading library detected. Multi-threading will be disabled.")
  set(XSIGMA_USE_PTHREADS 0 CACHE INTERNAL "Use POSIX threads (pthreads)" FORCE)
  set(XSIGMA_USE_WIN32_THREADS 0 CACHE INTERNAL "Use Win32 threads" FORCE)
endif()

# =============================================================================
# Maximum Thread Count Configuration
# =============================================================================

# Set maximum number of threads (can be overridden by user)
if(NOT DEFINED XSIGMA_MAX_THREADS)
  set(XSIGMA_MAX_THREADS 64 CACHE STRING "Maximum number of threads supported by XSigma")
endif()

# Validate XSIGMA_MAX_THREADS
if(XSIGMA_MAX_THREADS LESS 1)
  message(FATAL_ERROR "XSigma: XSIGMA_MAX_THREADS must be at least 1 (got ${XSIGMA_MAX_THREADS})")
endif()

message(STATUS "XSigma: Maximum threads: ${XSIGMA_MAX_THREADS}")

# =============================================================================
# Thread Library Linking
# =============================================================================

# Create an interface library for threading support
if(NOT TARGET XSigma::threads)
  add_library(XSigma_threads INTERFACE)
  add_library(XSigma::threads ALIAS XSigma_threads)
  
  # Link the appropriate thread library
  if(XSIGMA_USE_PTHREADS)
    # Link pthreads library on Unix-like systems
    target_link_libraries(XSigma_threads INTERFACE Threads::Threads)
    
    # Add pthread compile flags if needed (for some compilers)
    if(CMAKE_THREAD_LIBS_INIT MATCHES "-pthread")
      target_compile_options(XSigma_threads INTERFACE "-pthread")
    endif()
    
  elseif(XSIGMA_USE_WIN32_THREADS)
    # Win32 threads are part of the Windows API, no additional linking needed
    # But we ensure Threads::Threads is available for consistency
    target_link_libraries(XSigma_threads INTERFACE Threads::Threads)
  endif()
  
  # Export thread configuration as compile definitions on the interface target
  target_compile_definitions(XSigma_threads INTERFACE
    XSIGMA_USE_PTHREADS=${XSIGMA_USE_PTHREADS}
    XSIGMA_USE_WIN32_THREADS=${XSIGMA_USE_WIN32_THREADS}
    XSIGMA_MAX_THREADS=${XSIGMA_MAX_THREADS}
  )

  message(STATUS "XSigma: Created XSigma::threads interface library")

  # Add to XSIGMA_DEPENDENCY_LIBS for automatic linking to Core and other libraries
  # This ensures all XSigma libraries have access to threading support
  get_property(_temp_libs GLOBAL PROPERTY _XSIGMA_DEPENDENCY_LIBS)
  list(APPEND _temp_libs XSigma::threads)
  set_property(GLOBAL PROPERTY _XSIGMA_DEPENDENCY_LIBS "${_temp_libs}")
  message(STATUS "XSigma: XSigma::threads added to XSIGMA_DEPENDENCY_LIBS")
endif()

# =============================================================================
# Summary
# =============================================================================

message(STATUS "XSigma: Thread configuration summary:")
message(STATUS "  - XSIGMA_USE_PTHREADS: ${XSIGMA_USE_PTHREADS}")
message(STATUS "  - XSIGMA_USE_WIN32_THREADS: ${XSIGMA_USE_WIN32_THREADS}")
message(STATUS "  - XSIGMA_MAX_THREADS: ${XSIGMA_MAX_THREADS}")
message(STATUS "  - CMAKE_THREAD_LIBS_INIT: ${CMAKE_THREAD_LIBS_INIT}")

