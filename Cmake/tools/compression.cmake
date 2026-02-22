# ============================================================================= Logging Backend
# Configuration
# =============================================================================
# Selects the logging backend for runtime diagnostics and debugging. Options are mutually exclusive:
# NATIVE, LOGURU, or GLOG.
# =============================================================================
# Include guard to prevent multiple inclusions
include_guard(GLOBAL)

# Compression Support Flag Controls whether compression support is enabled for data serialization.
# When enabled, allows selection of compression library (snappy or none).
option(QUARISMA_ENABLE_COMPRESSION "Enable compression support" OFF)

# Compression Library Selection Specifies which compression library to use: none or snappy.
set(QUARISMA_COMPRESSION_TYPE "none" CACHE STRING
                                         "Compression library to use. Options are  none, snappy"
)
set_property(CACHE QUARISMA_COMPRESSION_TYPE PROPERTY STRINGS none snappy)
mark_as_advanced(QUARISMA_ENABLE_COMPRESSION QUARISMA_COMPRESSION_TYPE)

# Compression configuration validation and setup
if(QUARISMA_ENABLE_COMPRESSION)
  if(QUARISMA_COMPRESSION_TYPE STREQUAL "SNAPPY")
    set(QUARISMA_COMPRESSION_TYPE_SNAPPY ON)
    message(STATUS "Compression enabled: Snappy")
  elseif(QUARISMA_COMPRESSION_TYPE STREQUAL "NONE")
    set(QUARISMA_ENABLE_COMPRESSION OFF)
    message(STATUS "Compression type set to NONE - disabling compression")
  else()
    message(
      FATAL_ERROR
        "Invalid QUARISMA_COMPRESSION_TYPE: ${QUARISMA_COMPRESSION_TYPE}. Valid options are: NONE, SNAPPY"
    )
  endif()
else()
  message(STATUS "Compression disabled")
endif()
