# ============================================================================= Logging Backend
# Configuration
# =============================================================================
# Selects the logging backend for runtime diagnostics and debugging. Options are mutually exclusive:
# NATIVE, LOGURU, or GLOG.
# =============================================================================
# Include guard to prevent multiple inclusions
include_guard(GLOBAL)

# Logging Backend Selection Specifies which logging backend to use: NATIVE (built-in), LOGURU, or
# GLOG. NATIVE: Lightweight built-in logging; LOGURU: Feature-rich logging; GLOG: Google's logging
# library.
set(QUARISMA_LOGGING_BACKEND "LOGURU"
    CACHE STRING "Logging backend to use. Options are NATIVE, LOGURU, or GLOG"
)
set_property(CACHE QUARISMA_LOGGING_BACKEND PROPERTY STRINGS NATIVE LOGURU GLOG)
mark_as_advanced(QUARISMA_LOGGING_BACKEND)

# Validate logging backend selection
if(NOT QUARISMA_LOGGING_BACKEND MATCHES "^(NATIVE|LOGURU|GLOG)$")
  message(
    FATAL_ERROR
      "QUARISMA_LOGGING_BACKEND must be NATIVE, LOGURU, or GLOG (got: ${QUARISMA_LOGGING_BACKEND})"
  )
endif()

# Set preprocessor definitions based on selected backend
if(QUARISMA_LOGGING_BACKEND STREQUAL "NATIVE")
  message(STATUS "Using NATIVE logging backend")
  set(QUARISMA_ENABLE_NATIVE_LOGGING ON)
elseif(QUARISMA_LOGGING_BACKEND STREQUAL "LOGURU")
  message(STATUS "Using LOGURU logging backend")
  set(QUARISMA_ENABLE_LOGURU ON)
elseif(QUARISMA_LOGGING_BACKEND STREQUAL "GLOG")
  message(STATUS "Using GLOG logging backend")
  set(QUARISMA_ENABLE_GLOG ON)
endif()
