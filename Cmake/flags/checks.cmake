# ============================================================================= Quarisma System
# Validation and Checks Module
# =============================================================================
# This module performs efficient system validation with aggressive caching to minimize CMake
# reconfiguration overhead while ensuring all required dependencies and capabilities are available.
#
# Performance Optimizations: - Cached validation results to avoid redundant checks - Fast platform
# detection with cached results - Efficient compiler capability validation - Streamlined dependency
# checking
# =============================================================================

# Guard against multiple inclusions for performance
if(QUARISMA_CHECKS_CONFIGURED)
  return()
endif()
set(QUARISMA_CHECKS_CONFIGURED TRUE CACHE INTERNAL "Checks module loaded")

# Create cache key for validation results
set(QUARISMA_VALIDATION_CACHE_KEY
    "${CMAKE_CXX_COMPILER_ID}_${CMAKE_CXX_COMPILER_VERSION}_${CMAKE_SYSTEM_NAME}_${CMAKE_SYSTEM_VERSION}"
    CACHE INTERNAL "Validation cache key"
)

# Check if validation has already been completed for this configuration
if(QUARISMA_VALIDATION_COMPLETED STREQUAL QUARISMA_VALIDATION_CACHE_KEY)
  message(STATUS "Quarisma: Using cached validation results")
  return()
endif()

message(STATUS "Quarisma: Performing system validation...")

# ============================================================================= Platform Detection
# with Caching
# =============================================================================

# Fast platform detection with cached results
if(NOT DEFINED QUARISMA_PLATFORM_DETECTED)
  if(WIN32)
    set(QUARISMA_PLATFORM "Windows" CACHE INTERNAL "Detected platform")
    set(QUARISMA_PLATFORM_WINDOWS TRUE CACHE INTERNAL "Windows platform detected")
  elseif(APPLE)
    set(QUARISMA_PLATFORM "macOS" CACHE INTERNAL "Detected platform")
    set(QUARISMA_PLATFORM_MACOS TRUE CACHE INTERNAL "macOS platform detected")
  elseif(UNIX)
    set(QUARISMA_PLATFORM "Linux" CACHE INTERNAL "Detected platform")
    set(QUARISMA_PLATFORM_LINUX TRUE CACHE INTERNAL "Linux platform detected")
  else()
    message(WARNING "Quarisma: Unknown platform detected")
    set(QUARISMA_PLATFORM "Unknown" CACHE INTERNAL "Detected platform")
  endif()
  set(QUARISMA_PLATFORM_DETECTED TRUE CACHE INTERNAL "Platform detection completed")
  message(STATUS "Quarisma: Platform detected: ${QUARISMA_PLATFORM}")
endif()

# ============================================================================= Compiler Version
# Validation (Updated for Modern Requirements)
# =============================================================================

# Updated minimum compiler versions for C++17 support and modern optimizations
set(QUARISMA_MIN_GCC_VERSION "7.0")
set(QUARISMA_MIN_CLANG_VERSION "5.0")
set(QUARISMA_MIN_APPLE_CLANG_VERSION "9.0")
set(QUARISMA_MIN_MSVC_VERSION "19.14") # VS 2017 15.7
set(QUARISMA_MIN_INTEL_VERSION "18.0")

set(QUARISMA_COMPILER_ID ${CMAKE_CXX_COMPILER_ID} CACHE INTERNAL "Compiler ID")
mark_as_advanced(QUARISMA_COMPILER_ID)

# GCC version check
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${QUARISMA_MIN_GCC_VERSION})
    message(
      FATAL_ERROR
        "Quarisma requires GCC ${QUARISMA_MIN_GCC_VERSION} or later for C++17 support and modern optimizations. Found: ${CMAKE_CXX_COMPILER_VERSION}"
    )
  endif()
  set(QUARISMA_COMPILER_GCC TRUE CACHE INTERNAL "GCC compiler detected")
  message(STATUS "Quarisma: GCC ${CMAKE_CXX_COMPILER_VERSION} validated")
  set(QUARISMA_COMPILER_ID "gcc" CACHE INTERNAL "Compiler ID")

  # Clang version check
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${QUARISMA_MIN_CLANG_VERSION})
    message(
      FATAL_ERROR
        "Quarisma requires Clang ${QUARISMA_MIN_CLANG_VERSION} or later for C++17 support and modern optimizations. Found: ${CMAKE_CXX_COMPILER_VERSION}"
    )
  endif()
  set(QUARISMA_COMPILER_CLANG TRUE CACHE INTERNAL "Clang compiler detected")
  message(STATUS "Quarisma: Clang ${CMAKE_CXX_COMPILER_VERSION} validated")
  set(QUARISMA_COMPILER_ID "clang" CACHE INTERNAL "Compiler ID")

  # Apple Clang version check
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${QUARISMA_MIN_APPLE_CLANG_VERSION})
    message(
      FATAL_ERROR
        "Quarisma requires Apple Clang ${QUARISMA_MIN_APPLE_CLANG_VERSION} or later for C++17 support and modern optimizations. Found: ${CMAKE_CXX_COMPILER_VERSION}"
    )
  endif()
  set(QUARISMA_COMPILER_APPLE_CLANG TRUE CACHE INTERNAL "Apple Clang compiler detected")
  message(STATUS "Quarisma: Apple Clang ${CMAKE_CXX_COMPILER_VERSION} validated")
  set(QUARISMA_COMPILER_ID "clang" CACHE INTERNAL "Compiler ID")

  # MSVC version check
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${QUARISMA_MIN_MSVC_VERSION})
    message(
      FATAL_ERROR
        "Quarisma requires MSVC ${QUARISMA_MIN_MSVC_VERSION} or later (Visual Studio 2017 15.7+) for C++17 support and modern optimizations. Found: ${CMAKE_CXX_COMPILER_VERSION}"
    )
  endif()
  set(QUARISMA_COMPILER_MSVC TRUE CACHE INTERNAL "MSVC compiler detected")
  message(STATUS "Quarisma: MSVC ${CMAKE_CXX_COMPILER_VERSION} validated")
  set(QUARISMA_COMPILER_ID "msvc" CACHE INTERNAL "Compiler ID")

  # Intel C++ version check
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${QUARISMA_MIN_INTEL_VERSION})
    message(
      FATAL_ERROR
        "Quarisma requires Intel C++ ${QUARISMA_MIN_INTEL_VERSION} or later for C++17 support and modern optimizations. Found: ${CMAKE_CXX_COMPILER_VERSION}"
    )
  endif()
  set(QUARISMA_COMPILER_INTEL TRUE CACHE INTERNAL "Intel compiler detected")
  message(STATUS "Quarisma: Intel C++ ${CMAKE_CXX_COMPILER_VERSION} validated")
  set(QUARISMA_COMPILER_ID "intel" CACHE INTERNAL "Compiler ID")

else()
  message(WARNING "Quarisma: Unknown compiler '${CMAKE_CXX_COMPILER_ID}'. Build may fail.")
endif()

# ============================================================================= C++ Standard
# Validation
# =============================================================================

# Ensure C++17 is properly configured (updated from C++11)
if(NOT QUARISMA_IGNORE_CMAKE_CXX_STANDARD_CHECKS)
  # Validate that the requested C++ standard is supported
  if(QUARISMA_CXX_STANDARD LESS 11)
    message(
      FATAL_ERROR "Quarisma requires C++11 or later. Current setting: C++${QUARISMA_CXX_STANDARD}"
    )
  endif()

  # Ensure standard is properly set
  set(CMAKE_CXX_STANDARD ${QUARISMA_CXX_STANDARD})
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)

  message(STATUS "Quarisma: C++${QUARISMA_CXX_STANDARD} standard validated and configured")
endif()

# ============================================================================= Essential System
# Dependencies Validation
# =============================================================================

include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckSymbolExists)

# Threading support validation (cached)
if(NOT DEFINED QUARISMA_THREADING_VALIDATED)
  find_package(Threads REQUIRED)
  if(NOT Threads_FOUND)
    message(FATAL_ERROR "Quarisma: Threading support is required but not found")
  endif()
  set(QUARISMA_THREADING_VALIDATED TRUE CACHE INTERNAL "Threading validation completed")
  message(STATUS "Quarisma: Threading support validated")
endif()

# Math library validation (cached)
if(NOT DEFINED QUARISMA_MATH_LIB_VALIDATED)
  if(NOT WIN32)
    check_library_exists(m sin "" HAVE_LIBM)
    if(NOT HAVE_LIBM)
      message(FATAL_ERROR "Quarisma: Math library (libm) is required but not found")
    endif()
  endif()
  set(QUARISMA_MATH_LIB_VALIDATED TRUE CACHE INTERNAL "Math library validation completed")
  message(STATUS "Quarisma: Math library support validated")
endif()

# ============================================================================= Compiler Capability
# Validation (Cached)
# =============================================================================

include(CheckCXXSourceCompiles)

# C++17 features validation (cached)
if(NOT DEFINED QUARISMA_CXX17_FEATURES_VALIDATED)
  # Test structured bindings
  check_cxx_source_compiles(
    "
        #include <tuple>
        int main() {
            auto [a, b] = std::make_tuple(1, 2);
            return a + b - 3;
        }
    "
    QUARISMA_HAS_STRUCTURED_BINDINGS
  )

  # Test if constexpr
  check_cxx_source_compiles(
    "
        constexpr int test_func(int x) {
            if constexpr (true) {
                return x * 2;
            } else {
                return x;
            }
        }
        int main() {
            constexpr int result = test_func(5);
            return result - 10;
        }
    "
    QUARISMA_HAS_IF_CONSTEXPR
  )

  # Test std::optional
  check_cxx_source_compiles(
    "
        #include <optional>
        int main() {
            std::optional<int> opt = 42;
            return opt.value_or(0) - 42;
        }
    "
    QUARISMA_HAS_STD_OPTIONAL
  )

  if(NOT QUARISMA_HAS_STRUCTURED_BINDINGS OR NOT QUARISMA_HAS_IF_CONSTEXPR OR NOT
                                                                          QUARISMA_HAS_STD_OPTIONAL
  )
    message(WARNING "Quarisma: Compiler does not support required C++17 features")
  endif()

  set(QUARISMA_CXX17_FEATURES_VALIDATED TRUE CACHE INTERNAL "C++17 features validation completed")
  message(STATUS "Quarisma: C++17 features validated")
endif()

# Exception handling validation (cached)
if(NOT DEFINED QUARISMA_EXCEPTION_HANDLING_VALIDATED)
  check_cxx_source_compiles(
    "
        #include <stdexcept>
        int main() {
            try {
                throw std::runtime_error(\"test\");
            } catch (const std::exception& e) {
                return 0;
            }
            return 1;
        }
    "
    QUARISMA_HAS_EXCEPTION_HANDLING
  )

  if(NOT QUARISMA_HAS_EXCEPTION_HANDLING)
    message(WARNING "Quarisma: Exception handling not available - some features may be limited")
  endif()

  set(QUARISMA_EXCEPTION_HANDLING_VALIDATED TRUE CACHE INTERNAL
                                                     "Exception handling validation completed"
  )
  message(STATUS "Quarisma: Exception handling validated")
endif()

# ============================================================================= Platform-Specific
# Validations
# =============================================================================

# Windows-specific checks
if(QUARISMA_PLATFORM_WINDOWS)
  if(NOT DEFINED QUARISMA_WINDOWS_VALIDATED)
    check_include_file("windows.h" HAVE_WINDOWS_H)
    if(NOT HAVE_WINDOWS_H)
      message(FATAL_ERROR "Quarisma: Windows.h header not found on Windows platform")
    endif()
    set(QUARISMA_WINDOWS_VALIDATED TRUE CACHE INTERNAL "Windows validation completed")
    message(STATUS "Quarisma: Windows platform validation completed")
  endif()
endif()

# Unix-specific checks
if(QUARISMA_PLATFORM_LINUX OR QUARISMA_PLATFORM_MACOS)
  if(NOT DEFINED QUARISMA_UNIX_VALIDATED)
    check_include_file("unistd.h" HAVE_UNISTD_H)
    check_include_file("pthread.h" HAVE_PTHREAD_H)
    if(NOT HAVE_UNISTD_H OR NOT HAVE_PTHREAD_H)
      message(FATAL_ERROR "Quarisma: Required Unix headers not found")
    endif()
    set(QUARISMA_UNIX_VALIDATED TRUE CACHE INTERNAL "Unix validation completed")
    message(STATUS "Quarisma: Unix platform validation completed")
  endif()
endif()

# ============================================================================= Validation
# Completion and Caching
# =============================================================================

# Mark validation as completed for this configuration
set(QUARISMA_VALIDATION_COMPLETED "${QUARISMA_VALIDATION_CACHE_KEY}"
    CACHE INTERNAL "Validation completed for configuration"
)

# Store validation summary
set(QUARISMA_VALIDATION_SUMMARY
    "Platform: ${QUARISMA_PLATFORM}, Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}, C++: ${QUARISMA_CXX_STANDARD}"
    CACHE INTERNAL "Validation summary"
)

message(STATUS "Quarisma: System validation completed successfully")
message(STATUS "Quarisma: ${QUARISMA_VALIDATION_SUMMARY}")

# ============================================================================= End of checks.cmake
# =============================================================================
