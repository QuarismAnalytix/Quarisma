# Core module CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# Note: Threading configuration is now handled centrally by Cmake/tools/threads.cmake
# The variables XSIGMA_USE_PTHREADS, XSIGMA_USE_WIN32_THREADS, and XSIGMA_MAX_THREADS
# are set by the threads.cmake module and available here for configure_file()

# Configure header files
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/xsigma_version_macros.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/xsigma_version_macros.h" @ONLY
)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/xsigma_threads.h.in" "${CMAKE_CURRENT_BINARY_DIR}/xsigma_threads.h"
  @ONLY
)

set(xsigma_headers)
set(xsigma_sources)
set(xsigma_templates)

list(APPEND xsigma_headers "${CMAKE_CURRENT_BINARY_DIR}/xsigma_version_macros.h"
     "${CMAKE_CURRENT_BINARY_DIR}/xsigma_threads.h"
)

file(
  GLOB_RECURSE headers
  LIST_DIRECTORIES false
  "${CMAKE_CURRENT_SOURCE_DIR}/util/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/common/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/logging/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/memory/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/profiler/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/smp/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/c17/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/parallel/*.h"
  ${xsigma_headers}
)

file(
  GLOB_RECURSE sources
  LIST_DIRECTORIES false
  "${CMAKE_CURRENT_SOURCE_DIR}/common/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/util/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/logging/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/memory/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/profiler/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/smp/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/c17/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/parallel/*.cpp"
  ${xsigma_sources}
)

if(NOT XSIGMA_ENABLE_NATIVE_PROFILER)
  file(
    GLOB_RECURSE profiler_native_source_files
    LIST_DIRECTORIES false
    "${CMAKE_CURRENT_SOURCE_DIR}/profiler/native/*.cpp"
  )
  file(
    GLOB_RECURSE profiler_native_header_files
    LIST_DIRECTORIES false
    "${CMAKE_CURRENT_SOURCE_DIR}/profiler/native/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/profiler/native/*.hxx"
  )

  list(REMOVE_ITEM sources ${profiler_native_source_files})
  list(REMOVE_ITEM headers ${profiler_native_header_files})
  message(STATUS "Native profiler disabled: excluding profiler/native source/header files")
endif()

file(
  GLOB_RECURSE templates
  LIST_DIRECTORIES false
  "${CMAKE_CURRENT_SOURCE_DIR}/util/*.hxx" "${CMAKE_CURRENT_SOURCE_DIR}/common/*.hxx"
  "${CMAKE_CURRENT_SOURCE_DIR}/logging/*.hxx" "${CMAKE_CURRENT_SOURCE_DIR}/memory/*.hxx"
  "${CMAKE_CURRENT_SOURCE_DIR}/smp/*.hxx" ${xsigma_templates}
)

# Remove GPU files if neither CUDA nor HIP is enabled
if(NOT XSIGMA_ENABLE_CUDA OR XSIGMA_ENABLE_HIP)
  file(GLOB_RECURSE GPU_SOURCE_FILES memory/gpu/*.cpp)
  file(GLOB_RECURSE GPU_HEADER_FILES memory/gpu/*.h memory/gpu/*.hxx)
  list(REMOVE_ITEM headers ${GPU_HEADER_FILES})
  list(REMOVE_ITEM sources ${GPU_SOURCE_FILES})
  list(REMOVE_ITEM sources "${CMAKE_CURRENT_SOURCE_DIR}/memory/cpu/allocator_device.cpp")
  list(REMOVE_ITEM headers "${CMAKE_CURRENT_SOURCE_DIR}/memory/cpu/allocator_device.h")
endif()

if(XSIGMA_ENABLE_COMPRESSION)
  if(XSIGMA_COMPRESSION_TYPE STREQUAL "SNAPPY")
    list(APPEND headers "${CMAKE_CURRENT_SOURCE_DIR}/compression/snappy.h")
    list(APPEND sources "${CMAKE_CURRENT_SOURCE_DIR}/compression/snappy.cpp")
  endif()
endif()

# Remove logging.cpp and logging.h for NATIVE backend (uses simplified implementation in logger.cpp)
if(XSIGMA_USE_NATIVE_LOGGING)
  list(REMOVE_ITEM sources "${CMAKE_CURRENT_SOURCE_DIR}/logging/logging.cpp")
  list(REMOVE_ITEM headers "${CMAKE_CURRENT_SOURCE_DIR}/logging/logging.h")
  message(
    STATUS
      "NATIVE logging backend: Excluding logging.cpp and logging.h (using simplified fmt-based implementation)"
  )
endif()

if(XSIGMA_ENABLE_TBB)
  # If TBB is enabled, remove STDThread files
  file(GLOB_RECURSE TBB_FILES smp/STDThread/*.cpp)
  list(REMOVE_ITEM sources ${TBB_FILES})
  file(GLOB_RECURSE TBB_HEADER_FILES smp/STDThread/*.h smp/STDThread/*.hxx)
  list(REMOVE_ITEM headers ${TBB_HEADER_FILES})
  list(REMOVE_ITEM templates ${TBB_HEADER_FILES})
else()
  file(GLOB_RECURSE TBB_FILES smp/TBB/*.cpp)
  list(REMOVE_ITEM sources ${TBB_FILES})
  file(GLOB_RECURSE TBB_HEADER_FILES smp/TBB/*.h smp/TBB/*.hxx)
  list(REMOVE_ITEM headers ${TBB_HEADER_FILES})
  list(REMOVE_ITEM templates ${TBB_HEADER_FILES})
endif()

# Remove Kineto files if Kineto is disabled
if(NOT XSIGMA_ENABLE_KINETO)
  file(GLOB_RECURSE KINETO_SOURCE_FILES profiler/kineto_*.cpp)
  file(GLOB_RECURSE KINETO_HEADER_FILES profiler/kineto_*.h)
  list(REMOVE_ITEM sources ${KINETO_SOURCE_FILES})
  list(REMOVE_ITEM headers ${KINETO_HEADER_FILES})
  message(STATUS "Kineto disabled: Excluding Kineto source files from build")
endif()

# Create the Core library Respect BUILD_SHARED_LIBS setting for library type
add_library(Core ${sources} ${headers} ${templates})

# Set appropriate compile definitions based on library type
if(BUILD_SHARED_LIBS)
  target_compile_definitions(Core PUBLIC XSIGMA_SHARED_DEFINE PRIVATE XSIGMA_BUILDING_DLL)
  # Set output directories using XSigma's standardized approach
  if(COMMAND xsigma_set_target_output_directories)
    xsigma_set_target_output_directories(Core)
  endif()
else()
  target_compile_definitions(Core PUBLIC XSIGMA_STATIC_DEFINE)
endif()

# Create alias for consistent naming
add_library(XSigma::Core ALIAS Core)

# Enable Link Time Optimization (LTO) for Core library if enabled LTO is disabled for third-party
# libraries via FILTER_FLAGS to avoid conflicts
if(XSIGMA_ENABLE_LTO)
  set_target_properties(Core PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
  message(STATUS "Core library: Link Time Optimization (LTO) enabled")
else()
  message(STATUS "Core library: Link Time Optimization (LTO) disabled")
endif()

# Apply IWYU to the Core target (if available)
if(COMMAND xsigma_apply_iwyu)
  xsigma_apply_iwyu(Core)
endif()

# Configure Visual Studio source groups to preserve directory structure This ensures the Solution
# Explorer shows the actual filesystem hierarchy
xsigma_module_create_filters("${CMAKE_CURRENT_SOURCE_DIR}")

message(STATUS "Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Compiler ID: ${CMAKE_CXX_COMPILER_ID}, std++=${XSIGMA_CXX_STANDARD}")

# Set target properties
set_target_properties(
  Core PROPERTIES CXX_STANDARD ${XSIGMA_CXX_STANDARD} CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
)

if(MSVC)
  target_compile_options(Core PRIVATE /WX)
endif()

# ============================================================================= Include Directories
# =============================================================================
# Core library include directories are split into: 1. Core's own directories (PUBLIC for external
# consumers) 2. Centralized third-party include directories (PUBLIC to propagate to dependent
# targets like tests)

target_include_directories(
  Core
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Testing> $<INSTALL_INTERFACE:include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add centralized third-party include directories as PUBLIC so they propagate to dependent targets
# (e.g., test executables that link to XSigma::Core)
if(XSIGMA_DEPENDENCY_INCLUDE_DIRS)
  target_include_directories(Core PUBLIC ${XSIGMA_DEPENDENCY_INCLUDE_DIRS})
  message(STATUS "Core library include directories: ${XSIGMA_DEPENDENCY_INCLUDE_DIRS}")
endif()

# Link with build interface target
target_link_libraries(Core PUBLIC XSigma::build)
if(COMMAND xsigma_target_clang_tidy)
  xsigma_target_clang_tidy(Core)
endif()

# ============================================================================= Link with
# Centralized Dependencies
# =============================================================================
# All third-party and system library dependencies are now managed centrally via
# XSIGMA_DEPENDENCY_LIBS, which is populated by Cmake/tools/dependencies.cmake based on enabled
# feature flags. This ensures consistent linking across all targets.
#
# Dependencies are linked as PUBLIC so they propagate to dependent targets (e.g., test executables)
# that link to XSigma::Core. This is necessary because Core's public headers include headers from
# these dependencies (e.g., fmt/format.h in logger.h), so consumers of Core must also link with
# them.

# Link with centralized dependency list
if(XSIGMA_DEPENDENCY_LIBS)
  target_link_libraries(Core PUBLIC ${XSIGMA_DEPENDENCY_LIBS})
  message(STATUS "Core library linked with centralized dependencies: ${XSIGMA_DEPENDENCY_LIBS}")
endif()

# ============================================================================= Compile Definitions
# for Optional Features
# =============================================================================
# Feature-related compile definitions are now centrally managed via
# XSIGMA_DEPENDENCY_COMPILE_DEFINITIONS, which is populated by Cmake/tools/dependencies.cmake based
# on enabled feature flags. These definitions are PUBLIC to propagate to dependent targets, ensuring
# feature macros are available in test files and other dependent code.

# Apply centralized compile definitions
if(XSIGMA_DEPENDENCY_COMPILE_DEFINITIONS)
  target_compile_definitions(Core PUBLIC ${XSIGMA_DEPENDENCY_COMPILE_DEFINITIONS})
  message(STATUS "Core library compile definitions: ${XSIGMA_DEPENDENCY_COMPILE_DEFINITIONS}")
endif()

# Handle Mimalloc dependency tracking
if(XSIGMA_ENABLE_MIMALLOC AND TARGET XSigma::mimalloc)
  add_dependencies(Core XSigma::mimalloc)
endif()

# Install targets
install(
  TARGETS Core
  EXPORT XSigmaTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES
  DESTINATION include
)

# Install headers
install(FILES ${headers} DESTINATION include/XSigma/Core COMPONENT Development)

# Install configured headers
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/xsigma_version_macros.h"
              "${CMAKE_CURRENT_BINARY_DIR}/xsigma_threads.h" DESTINATION include/XSigma/Core
        COMPONENT Development
)

# Configure testing
if(XSIGMA_BUILD_TESTING STREQUAL "ON" OR XSIGMA_BUILD_TESTING STREQUAL "WANT")
  add_subdirectory(Testing/Cxx)
endif()
