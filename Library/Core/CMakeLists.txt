# Core module CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# Configure threading
if(WIN32)
  set(QUARISMA_USE_WIN32_THREADS ON)
  set(QUARISMA_MAX_THREADS 64)
else()
  set(QUARISMA_USE_PTHREADS ON)
  set(QUARISMA_MAX_THREADS 64)
endif()

# Configure header files
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/quarisma_version_macros.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/quarisma_version_macros.h" @ONLY
)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/quarisma_threads.h.in" "${CMAKE_CURRENT_BINARY_DIR}/quarisma_threads.h"
  @ONLY
)

set(quarisma_headers)
set(quarisma_sources)
set(quarisma_templates)

list(APPEND quarisma_headers "${CMAKE_CURRENT_BINARY_DIR}/quarisma_version_macros.h"
     "${CMAKE_CURRENT_BINARY_DIR}/quarisma_threads.h"
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
  "${CMAKE_CURRENT_SOURCE_DIR}/smp_new/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/c17/*.h"
  ${quarisma_headers}
)

file(
  GLOB_RECURSE sources
  LIST_DIRECTORIES false
  "${CMAKE_CURRENT_SOURCE_DIR}/common/*.cxx"
  "${CMAKE_CURRENT_SOURCE_DIR}/util/*.cxx"
  "${CMAKE_CURRENT_SOURCE_DIR}/logging/*.cxx"
  "${CMAKE_CURRENT_SOURCE_DIR}/memory/*.cxx"
  "${CMAKE_CURRENT_SOURCE_DIR}/profiler/*.cxx"
  "${CMAKE_CURRENT_SOURCE_DIR}/smp/*.cxx"
  "${CMAKE_CURRENT_SOURCE_DIR}/smp_new/*.cxx"
  "${CMAKE_CURRENT_SOURCE_DIR}/c17/*.h"
  ${quarisma_sources}
)

file(
  GLOB_RECURSE templates
  LIST_DIRECTORIES false
  "${CMAKE_CURRENT_SOURCE_DIR}/util/*.hxx"
  "${CMAKE_CURRENT_SOURCE_DIR}/common/*.hxx"
  "${CMAKE_CURRENT_SOURCE_DIR}/logging/*.hxx"
  "${CMAKE_CURRENT_SOURCE_DIR}/memory/*.hxx"
  "${CMAKE_CURRENT_SOURCE_DIR}/smp/*.hxx"
  "${CMAKE_CURRENT_SOURCE_DIR}/smp_new/*.hxx"
  ${quarisma_templates}
)

# Remove GPU files if neither CUDA nor HIP is enabled
if(NOT QUARISMA_ENABLE_CUDA OR QUARISMA_ENABLE_HIP)
  file(GLOB_RECURSE GPU_SOURCE_FILES memory/gpu/*.cxx)
  file(GLOB_RECURSE GPU_HEADER_FILES memory/gpu/*.h memory/gpu/*.hxx)
  list(REMOVE_ITEM headers ${GPU_HEADER_FILES})
  list(REMOVE_ITEM sources ${GPU_SOURCE_FILES})
  list(REMOVE_ITEM sources "${CMAKE_CURRENT_SOURCE_DIR}/memory/cpu/allocator_device.cxx")
  list(REMOVE_ITEM headers "${CMAKE_CURRENT_SOURCE_DIR}/memory/cpu/allocator_device.h")
endif()

if(QUARISMA_ENABLE_COMPRESSION)
  if(QUARISMA_COMPRESSION_TYPE STREQUAL "SNAPPY")
    list(APPEND headers "${CMAKE_CURRENT_SOURCE_DIR}/compression/snappy.h")
    list(APPEND sources "${CMAKE_CURRENT_SOURCE_DIR}/compression/snappy.cxx")
  endif()
endif()

# Remove logging.cxx and logging.h for NATIVE backend (uses simplified implementation in logger.cxx)
if(QUARISMA_USE_NATIVE_LOGGING)
  list(REMOVE_ITEM sources "${CMAKE_CURRENT_SOURCE_DIR}/logging/logging.cxx")
  list(REMOVE_ITEM headers "${CMAKE_CURRENT_SOURCE_DIR}/logging/logging.h")
  message(
    STATUS
      "NATIVE logging backend: Excluding logging.cxx and logging.h (using simplified fmt-based implementation)"
  )
endif()

if(QUARISMA_ENABLE_TBB)
  # If TBB is enabled, remove STDThread files
  file(GLOB_RECURSE TBB_FILES smp/STDThread/*.cxx)
  list(REMOVE_ITEM sources ${TBB_FILES})
  file(GLOB_RECURSE TBB_HEADER_FILES smp/STDThread/*.h smp/STDThread/*.hxx)
  list(REMOVE_ITEM headers ${TBB_HEADER_FILES})
  list(REMOVE_ITEM templates ${TBB_HEADER_FILES})
else()
  file(GLOB_RECURSE TBB_FILES smp/TBB/*.cxx)
  list(REMOVE_ITEM sources ${TBB_FILES})
  file(GLOB_RECURSE TBB_HEADER_FILES smp/TBB/*.h smp/TBB/*.hxx)
  list(REMOVE_ITEM headers ${TBB_HEADER_FILES})
  list(REMOVE_ITEM templates ${TBB_HEADER_FILES})
endif()

# Remove Kineto files if Kineto is disabled
if(NOT QUARISMA_ENABLE_KINETO)
  file(GLOB_RECURSE KINETO_SOURCE_FILES profiler/kineto_*.cxx)
  file(GLOB_RECURSE KINETO_HEADER_FILES profiler/kineto_*.h)
  list(REMOVE_ITEM sources ${KINETO_SOURCE_FILES})
  list(REMOVE_ITEM headers ${KINETO_HEADER_FILES})
  message(STATUS "Kineto disabled: Excluding Kineto source files from build")
endif()

# Create the Core library Respect BUILD_SHARED_LIBS setting for library type
add_library(Core ${sources} ${headers} ${templates})

# Set appropriate compile definitions based on library type
if(BUILD_SHARED_LIBS)
  target_compile_definitions(Core PUBLIC QUARISMA_SHARED_DEFINE PRIVATE QUARISMA_BUILDING_DLL)
  # Set output directories using Quarisma's standardized approach
  if(COMMAND quarisma_set_target_output_directories)
    quarisma_set_target_output_directories(Core)
  endif()
else()
  target_compile_definitions(Core PUBLIC QUARISMA_STATIC_DEFINE)
endif()

# Create alias for consistent naming
add_library(Quarisma::Core ALIAS Core)

# Enable Link Time Optimization (LTO) for Core library if enabled LTO is disabled for third-party
# libraries via FILTER_FLAGS to avoid conflicts
if(QUARISMA_ENABLE_LTO)
  set_target_properties(Core PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
  message(STATUS "Core library: Link Time Optimization (LTO) enabled")
else()
  message(STATUS "Core library: Link Time Optimization (LTO) disabled")
endif()

# Apply IWYU to the Core target (if available)
if(COMMAND quarisma_apply_iwyu)
  quarisma_apply_iwyu(Core)
endif()

# Configure Visual Studio source groups to preserve directory structure This ensures the Solution
# Explorer shows the actual filesystem hierarchy
quarisma_module_create_filters("${CMAKE_CURRENT_SOURCE_DIR}")

message(STATUS "Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Compiler ID: ${CMAKE_CXX_COMPILER_ID}, std++=${QUARISMA_CXX_STANDARD}")

# Set target properties
set_target_properties(
  Core PROPERTIES CXX_STANDARD ${QUARISMA_CXX_STANDARD} CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
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
# (e.g., test executables that link to Quarisma::Core)
if(QUARISMA_DEPENDENCY_INCLUDE_DIRS)
  target_include_directories(Core PUBLIC ${QUARISMA_DEPENDENCY_INCLUDE_DIRS})
  message(STATUS "Core library include directories: ${QUARISMA_DEPENDENCY_INCLUDE_DIRS}")
endif()

# Link with build interface target
target_link_libraries(Core PUBLIC Quarisma::build)
if(COMMAND quarisma_target_clang_tidy)
  quarisma_target_clang_tidy(Core)
endif()

# ============================================================================= Link with
# Centralized Dependencies
# =============================================================================
# All third-party and system library dependencies are now managed centrally via
# QUARISMA_DEPENDENCY_LIBS, which is populated by Cmake/tools/dependencies.cmake based on enabled
# feature flags. This ensures consistent linking across all targets.
#
# Dependencies are linked as PUBLIC so they propagate to dependent targets (e.g., test executables)
# that link to Quarisma::Core. This is necessary because Core's public headers include headers from
# these dependencies (e.g., fmt/format.h in logger.h), so consumers of Core must also link with
# them.

# Link with centralized dependency list
if(QUARISMA_DEPENDENCY_LIBS)
  target_link_libraries(Core PUBLIC ${QUARISMA_DEPENDENCY_LIBS})
  message(STATUS "Core library linked with centralized dependencies: ${QUARISMA_DEPENDENCY_LIBS}")
endif()

# ============================================================================= Compile Definitions
# for Optional Features
# =============================================================================
# Feature-related compile definitions are now centrally managed via
# QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS, which is populated by Cmake/tools/dependencies.cmake based
# on enabled feature flags. These definitions are PUBLIC to propagate to dependent targets, ensuring
# feature macros are available in test files and other dependent code.

# Apply centralized compile definitions
if(QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS)
  target_compile_definitions(Core PUBLIC ${QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS})
  message(STATUS "Core library compile definitions: ${QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS}")
endif()

# Handle Mimalloc dependency tracking
if(QUARISMA_ENABLE_MIMALLOC AND TARGET Quarisma::mimalloc)
  add_dependencies(Core Quarisma::mimalloc)
endif()

# Install targets
install(
  TARGETS Core
  EXPORT QuarismaTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES
  DESTINATION include
)

# Install headers
install(FILES ${headers} DESTINATION include/Quarisma/Core COMPONENT Development)

# Install configured headers
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/quarisma_version_macros.h"
              "${CMAKE_CURRENT_BINARY_DIR}/quarisma_threads.h" DESTINATION include/Quarisma/Core
        COMPONENT Development
)

# Configure testing
if(QUARISMA_BUILD_TESTING STREQUAL "ON" OR QUARISMA_BUILD_TESTING STREQUAL "WANT")
  add_subdirectory(Testing/Cxx)
endif()
