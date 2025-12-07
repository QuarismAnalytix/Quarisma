# =============================================================================
# XSigma Enzyme Automatic Differentiation Integration Module
# =============================================================================
# This module integrates Enzyme AD (https://enzyme.mit.edu/) for automatic
# differentiation. Enzyme works by loading as an LLVM plugin and requires
# special compiler flags to enable differentiation capabilities.
#
# Enzyme provides:
# - Forward-mode automatic differentiation
# - Reverse-mode automatic differentiation (backpropagation)
# - High-performance AD with minimal overhead
# - Works with C and C++ code
#
# Requirements:
# - Clang/LLVM compiler (GCC not supported)
# - Enzyme plugin library (ClangEnzyme-*.so or LLVMEnzyme-*.so)
# =============================================================================

cmake_minimum_required(VERSION 3.16)

# Include guard to prevent multiple inclusions
include_guard(GLOBAL)

# Enzyme AD Support Flag
# Controls whether Enzyme automatic differentiation is enabled.
# When enabled, Enzyme provides high-performance AD capabilities.
# Note: Already defined in main CMakeLists.txt, just documenting here
if(NOT DEFINED XSIGMA_ENABLE_ENZYME)
  option(XSIGMA_ENABLE_ENZYME "Enable Enzyme automatic differentiation support" OFF)
  mark_as_advanced(XSIGMA_ENABLE_ENZYME)
endif()

# Only proceed if Enzyme is enabled
if(NOT XSIGMA_ENABLE_ENZYME)
  message(STATUS "Enzyme automatic differentiation support is disabled (XSIGMA_ENABLE_ENZYME=OFF)")
  return()
endif()

message(STATUS "Configuring Enzyme automatic differentiation support...")

# =============================================================================
# Step 1: Verify compiler compatibility
# =============================================================================
# Enzyme requires Clang/LLVM compiler
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message(WARNING
    "\n"
    "================================================================================\n"
    "WARNING: Enzyme AD requires Clang/LLVM compiler\n"
    "================================================================================\n"
    "\n"
    "Current compiler: ${CMAKE_CXX_COMPILER_ID}\n"
    "Enzyme automatic differentiation only works with Clang/LLVM.\n"
    "\n"
    "To use Enzyme:\n"
    "1. Install Clang/LLVM:\n"
    "   - macOS: brew install llvm\n"
    "   - Ubuntu: apt install clang llvm\n"
    "\n"
    "2. Reconfigure with Clang:\n"
    "   cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang ..\n"
    "\n"
    "Disabling Enzyme support.\n"
    "================================================================================\n"
  )
  set(XSIGMA_ENABLE_ENZYME OFF CACHE BOOL "Enable Enzyme automatic differentiation support" FORCE)
  return()
endif()

message(STATUS "✅ Compiler check passed: ${CMAKE_CXX_COMPILER_ID}")

# =============================================================================
# Step 2: Find Enzyme plugin library
# =============================================================================
# Enzyme can be installed in several ways:
# 1. System-wide installation (e.g., via package manager)
# 2. LLVM plugin directory
# 3. Custom installation path specified via ENZYME_PLUGIN_PATH

# Option to specify custom Enzyme plugin path
set(ENZYME_PLUGIN_PATH "" CACHE PATH "Path to Enzyme plugin library (ClangEnzyme-*.so or LLVMEnzyme-*.so)")
mark_as_advanced(ENZYME_PLUGIN_PATH)

set(ENZYME_PLUGIN_FOUND FALSE)
set(ENZYME_PLUGIN_LIBRARY "")

# Try to find Enzyme plugin library
if(ENZYME_PLUGIN_PATH AND EXISTS "${ENZYME_PLUGIN_PATH}")
  # User provided explicit path
  set(ENZYME_PLUGIN_LIBRARY "${ENZYME_PLUGIN_PATH}")
  set(ENZYME_PLUGIN_FOUND TRUE)
  message(STATUS "Using user-specified Enzyme plugin: ${ENZYME_PLUGIN_LIBRARY}")
else()
  # Search in common locations
  set(_enzyme_search_paths
    "/usr/lib"
    "/usr/local/lib"
    "/opt/homebrew/lib"
    "/opt/homebrew/opt/llvm/lib"
    "$ENV{LLVM_DIR}/lib"
    "${CMAKE_CXX_COMPILER_EXTERNAL_TOOLCHAIN}/lib"
  )

  # Try to extract LLVM installation directory from compiler
  get_filename_component(_compiler_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
  get_filename_component(_llvm_root "${_compiler_dir}" DIRECTORY)
  list(APPEND _enzyme_search_paths "${_llvm_root}/lib")

  # Search for Enzyme plugin
  # Extract major version from compiler version (e.g., 21.1.6 -> 21)
  string(REGEX MATCH "^([0-9]+)" _llvm_major_version "${CMAKE_CXX_COMPILER_VERSION}")

  message(STATUS "Searching for Enzyme plugin...")
  message(STATUS "  Compiler version: ${CMAKE_CXX_COMPILER_VERSION}")
  message(STATUS "  LLVM major version: ${_llvm_major_version}")
  message(STATUS "  Search paths: ${_enzyme_search_paths}")

  # On macOS, the libraries don't have 'lib' prefix and use .dylib extension
  # Try to find the plugin files directly using file(GLOB ...)
  set(_found_enzyme_files)
  foreach(_search_path ${_enzyme_search_paths})
    file(GLOB _enzyme_candidates
      "${_search_path}/ClangEnzyme-${CMAKE_CXX_COMPILER_VERSION}.dylib"
      "${_search_path}/ClangEnzyme-${_llvm_major_version}.dylib"
      "${_search_path}/ClangEnzyme.dylib"
      "${_search_path}/LLVMEnzyme-${CMAKE_CXX_COMPILER_VERSION}.dylib"
      "${_search_path}/LLVMEnzyme-${_llvm_major_version}.dylib"
      "${_search_path}/LLVMEnzyme.dylib"
      "${_search_path}/ClangEnzyme-${CMAKE_CXX_COMPILER_VERSION}.so"
      "${_search_path}/ClangEnzyme-${_llvm_major_version}.so"
      "${_search_path}/ClangEnzyme.so"
      "${_search_path}/LLVMEnzyme-${CMAKE_CXX_COMPILER_VERSION}.so"
      "${_search_path}/LLVMEnzyme-${_llvm_major_version}.so"
      "${_search_path}/LLVMEnzyme.so"
    )
    if(_enzyme_candidates)
      list(APPEND _found_enzyme_files ${_enzyme_candidates})
    endif()
  endforeach()

  if(_found_enzyme_files)
    list(GET _found_enzyme_files 0 ENZYME_PLUGIN_LIBRARY)
    message(STATUS "  Found candidates: ${_found_enzyme_files}")
  endif()

  message(STATUS "  Result: ${ENZYME_PLUGIN_LIBRARY}")

  if(ENZYME_PLUGIN_LIBRARY)
    set(ENZYME_PLUGIN_FOUND TRUE)
    message(STATUS "✅ Found Enzyme plugin: ${ENZYME_PLUGIN_LIBRARY}")
  endif()
endif()

# Check if Enzyme was found
if(NOT ENZYME_PLUGIN_FOUND)
  message(WARNING
    "\n"
    "================================================================================\n"
    "WARNING: Enzyme plugin library not found\n"
    "================================================================================\n"
    "\n"
    "Enzyme automatic differentiation requires the Enzyme LLVM plugin.\n"
    "\n"
    "INSTALLATION METHODS:\n"
    "\n"
    "1. Build from source:\n"
    "   git clone https://github.com/EnzymeAD/Enzyme.git\n"
    "   cd Enzyme/enzyme\n"
    "   mkdir build && cd build\n"
    "   cmake -G Ninja .. \\\n"
    "     -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm \\\n"
    "     -DCMAKE_BUILD_TYPE=Release\n"
    "   ninja\n"
    "\n"
    "2. Specify custom path:\n"
    "   cmake -DENZYME_PLUGIN_PATH=/path/to/ClangEnzyme-*.so ..\n"
    "\n"
    "3. macOS (Homebrew):\n"
    "   brew install enzyme\n"
    "\n"
    "For more information: https://enzyme.mit.edu/\n"
    "\n"
    "Disabling Enzyme support.\n"
    "================================================================================\n"
  )
  set(XSIGMA_ENABLE_ENZYME OFF CACHE BOOL "Enable Enzyme automatic differentiation support" FORCE)
  return()
endif()

# =============================================================================
# Step 3: Configure Enzyme compiler flags
# =============================================================================
# Enzyme requires special flags to load the plugin and enable AD

# Compiler flags to load Enzyme plugin
# For LLVM 13+, use the new pass manager with -fpass-plugin
# Note: We only use -fpass-plugin, not the legacy -Xclang -load
set(ENZYME_COMPILER_FLAGS
  "-fpass-plugin=${ENZYME_PLUGIN_LIBRARY}"
)

# Option to enable Enzyme optimization flags
# NOTE: -fno-exceptions and -fno-rtti are disabled by default because they
# conflict with code that uses exceptions and RTTI (which XSigma does).
# Enzyme can work with exceptions and RTTI enabled, though it may have
# slightly lower performance.
option(ENZYME_ENABLE_OPTIMIZATIONS "Enable Enzyme-specific optimizations (disables exceptions/RTTI)" OFF)
mark_as_advanced(ENZYME_ENABLE_OPTIMIZATIONS)

if(ENZYME_ENABLE_OPTIMIZATIONS)
  list(APPEND ENZYME_COMPILER_FLAGS
    "-fno-exceptions"  # Enzyme works better without exceptions
    "-fno-rtti"        # Disable RTTI for better performance
  )
  message(WARNING "Enzyme optimizations enabled: -fno-exceptions and -fno-rtti will be applied. This may break code that uses exceptions or RTTI.")
endif()

# Export Enzyme flags for use by targets
set(ENZYME_COMPILE_OPTIONS ${ENZYME_COMPILER_FLAGS} CACHE STRING "Enzyme compiler flags" FORCE)

message(STATUS "Enzyme compiler flags: ${ENZYME_COMPILE_OPTIONS}")

# =============================================================================
# Step 4: Create XSigma::enzyme interface target
# =============================================================================
# Create an interface library that other targets can link against

if(NOT TARGET XSigma::enzyme)
  add_library(XSigma::enzyme INTERFACE IMPORTED GLOBAL)

  # Add Enzyme compiler flags to the interface
  target_compile_options(XSigma::enzyme INTERFACE ${ENZYME_COMPILE_OPTIONS})

  # Add Enzyme compile definition
  target_compile_definitions(XSigma::enzyme INTERFACE XSIGMA_HAS_ENZYME=1)

  message(STATUS "✅ Created XSigma::enzyme interface target")
endif()

# =============================================================================
# Step 5: Export Enzyme information
# =============================================================================

set(ENZYME_FOUND TRUE CACHE BOOL "Enzyme was found successfully" FORCE)
set(ENZYME_PLUGIN_LIBRARY "${ENZYME_PLUGIN_LIBRARY}" CACHE FILEPATH "Enzyme plugin library path" FORCE)

message(STATUS "Enzyme automatic differentiation configuration complete")
message(STATUS "  Plugin: ${ENZYME_PLUGIN_LIBRARY}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
