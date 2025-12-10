# =============================================================================
# SMP Backend Selection Configuration
# =============================================================================
# This module handles the selection of the SMP (Symmetric Multi-Processing)
# backend for XSigma. Users can specify which backend to use via:
# - Command-line: python setup.py config --smp.std (or --smp.openmp, --smp.tbb)
# - CMake directly: cmake -DXSIGMA_SMP_BACKEND=std (or openmp, tbb)
#
# ARGUMENT FLOW:
# 1. User runs: python setup.py config --smp.tbb
# 2. parse_args() in setup.py converts "--smp.tbb" to "smp.tbb"
# 3. XsigmaFlags.__process_arg_list() processes "smp.tbb" and sets:
#    - self.__value["smp_backend"] = "tbb"
#    - Adds to builder_suffix: "_smp_tbb"
# 4. XsigmaFlags.create_cmake_flags() generates: -DXSIGMA_SMP_BACKEND=tbb
# 5. CMake receives XSIGMA_SMP_BACKEND=tbb and this module processes it
# 6. This module enables/disables XSIGMA_ENABLE_TBB and XSIGMA_ENABLE_OPENMP
# 7. compile_definitions.cmake converts these to XSIGMA_HAS_TBB and XSIGMA_HAS_OPENMP
#
# SUPPORTED BACKENDS:
#   - std:     Standard C++ thread backend (std_thread)
#              Result: XSIGMA_HAS_TBB=0, XSIGMA_HAS_OPENMP=0
#   - openmp:  OpenMP backend for parallel processing
#              Result: XSIGMA_HAS_OPENMP=1, XSIGMA_HAS_TBB=0
#   - tbb:     Intel Threading Building Blocks (TBB) backend
#              Result: XSIGMA_HAS_TBB=1, XSIGMA_HAS_OPENMP=0
#
# Only one backend can be active at a time. The selected backend controls
# which implementation is compiled and linked into the XSigma Core library.

include_guard(GLOBAL)

# Define the SMP backend option with default value
set(XSIGMA_SMP_BACKEND "std"
    CACHE STRING "SMP backend selection: std, openmp, or tbb")
set_property(CACHE XSIGMA_SMP_BACKEND PROPERTY STRINGS std openmp tbb)

# Validate the selected backend
string(TOLOWER "${XSIGMA_SMP_BACKEND}" _smp_backend_lower)
if(NOT _smp_backend_lower MATCHES "^(std|openmp|tbb)$")
    message(FATAL_ERROR
        "Invalid XSIGMA_SMP_BACKEND value: '${XSIGMA_SMP_BACKEND}'. "
        "Valid options are: std, openmp, or tbb"
    )
endif()

# Configure backend-specific settings based on selection
if(_smp_backend_lower STREQUAL "std")
    message(STATUS "XSigma: SMP backend set to std_thread (standard C++ threads)")
    # std_thread is always available, no additional configuration needed
    
elseif(_smp_backend_lower STREQUAL "openmp")
    message(STATUS "XSigma: SMP backend set to OpenMP")
    # Enable OpenMP support
    set(XSIGMA_ENABLE_OPENMP ON CACHE BOOL "Enable OpenMP parallel processing support" FORCE)
    # Disable TBB if it was enabled
    set(XSIGMA_ENABLE_TBB OFF CACHE BOOL "Enable Intel TBB (Threading Building Blocks) support" FORCE)
    
elseif(_smp_backend_lower STREQUAL "tbb")
    message(STATUS "XSigma: SMP backend set to TBB (Intel Threading Building Blocks)")
    # Enable TBB support
    set(XSIGMA_ENABLE_TBB ON CACHE BOOL "Enable Intel TBB (Threading Building Blocks) support" FORCE)
    # Disable OpenMP if it was enabled
    set(XSIGMA_ENABLE_OPENMP OFF CACHE BOOL "Enable OpenMP parallel processing support" FORCE)
endif()

message(STATUS "XSigma: SMP backend configuration complete")

