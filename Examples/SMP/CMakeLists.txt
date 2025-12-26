# ============================================================================= Quarisma SMP Examples
# CMakeLists.txt
# =============================================================================
# This CMakeLists.txt configures and builds the Quarisma SMP (Symmetric Multi-Processing) example
# programs that demonstrate the usage of parallel APIs.
#
# Examples included: - example_parallelize_1d: Work-stealing based 1D parallelization -
# example_parallel_for: Chunk-based parallel iteration - example_parallel_reduce: Parallel reduction
# operations - example_thread_configuration: Thread pool configuration and backend selection
#
# Build these examples with: python setup.py config.build.examples.ninja.clang python setup.py
# config.build.examples.vs22
#
# Or manually: cmake -DQUARISMA_ENABLE_EXAMPLES=ON .. cmake --build .
# =============================================================================

cmake_minimum_required(VERSION 3.16)

# ============================================================================= Example Source Files
# =============================================================================
set(example_sources example_parallelize_1d.cxx example_parallel_for.cxx example_parallel_reduce.cxx
                    example_thread_configuration.cxx
)

# ============================================================================= Build Examples
# =============================================================================
foreach(example_source ${example_sources})
  # Extract example name from filename (remove .cxx extension)
  string(REPLACE ".cxx" "" example_name ${example_source})

  # Create executable
  add_executable(${example_name} ${example_source})

  # Set C++ standard
  set_target_properties(
    ${example_name} PROPERTIES CXX_STANDARD ${QUARISMA_CXX_STANDARD} CXX_STANDARD_REQUIRED ON
                               CXX_EXTENSIONS OFF
  )

  # Link against Core library (which includes all SMP functionality)
  target_link_libraries(${example_name} PRIVATE Quarisma::Core)

  # Set output directory
  if(COMMAND quarisma_set_target_output_directories)
    quarisma_set_target_output_directories(${example_name})
  else()
    set_target_properties(
      ${example_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
  endif()

  # Add to Examples folder in IDE
  set_target_properties(${example_name} PROPERTIES FOLDER "Examples/SMP")

  # Apply compiler flags if available
  if(MSVC)
    target_compile_options(${example_name} PRIVATE /WX)
  endif()

  message(STATUS "✓ Example configured: ${example_name}")
endforeach()

message(STATUS "✓ SMP Examples configured successfully")
