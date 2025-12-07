# ============================================================================= Core Library C++
# Tests
# =============================================================================
# Following cpuinfo's simple pattern for test integration

# ============================================================================= Test Source Files
# Discovery
# =============================================================================
file(GLOB_RECURSE test_sources LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/Test*.cpp"
                                                      "${CMAKE_CURRENT_SOURCE_DIR}/Test*.cu"
)

# Exclude special test files that need separate handling
set(TestFiles)
foreach(_test_source IN ITEMS ${test_sources})
  get_filename_component(file_name "${_test_source}" NAME)

  # Always exclude TestLoggerDisableSignalHandler
  if(file_name MATCHES "TestLoggerDisableSignalHandler")
    continue()
  endif()

  # Exclude GPU-related test files based on compile flags When both CUDA and HIP are disabled,
  # exclude all GPU tests
  if(NOT XSIGMA_ENABLE_CUDA AND NOT XSIGMA_ENABLE_HIP)
    if(file_name MATCHES "TestGpu" OR file_name MATCHES "TestGPU")
      message(STATUS "Excluding GPU test (no GPU support): ${file_name}")
      continue()
    endif()
  endif()

  # When CUDA is disabled, exclude CUDA-specific tests
  if(NOT XSIGMA_ENABLE_CUDA)
    if(file_name MATCHES "TestCuda")
      message(STATUS "Excluding CUDA test (CUDA disabled): ${file_name}")
      continue()
    endif()
  endif()

  # When HIP is disabled, exclude HIP-specific tests
  if(NOT XSIGMA_ENABLE_HIP)
    if(file_name MATCHES "TestHip")
      message(STATUS "Excluding HIP test (HIP disabled): ${file_name}")
      continue()
    endif()
  endif()

  # When Kineto is disabled, exclude Kineto-specific tests
  if(NOT XSIGMA_ENABLE_KINETO)
    if(file_name MATCHES "TestKineto")
      message(STATUS "Excluding Kineto test (Kineto disabled): ${file_name}")
      continue()
    endif()
  endif()

  # When Enzyme is disabled, exclude Enzyme-specific tests
  if(NOT XSIGMA_ENABLE_ENZYME)
    if(file_name MATCHES "TestEnzyme")
      message(STATUS "Excluding Enzyme test (Enzyme disabled): ${file_name}")
      continue()
    endif()
  endif()

  list(APPEND TestFiles "${_test_source}")
endforeach()

set(XSIGMA_NATIVE_PROFILER_TESTS
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerBackendIntegration.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerPlatform.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerXPlane.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerTimespan.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestEnhancedProfiler.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerAnnotationParsing.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerExporters.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerAnnotationStack.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerXPlaneVisitor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerFormatUtils.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerChromeTraceHierarchical.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestXPlaneSchema.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerAnalysis.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestXPlaneUtils.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerHeavyFunction.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestTraceme.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfiler.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerSessionEdgeCases.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestScopedMemoryDebugAnnotation.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerStatsCalculator.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerUtils.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerMemoryAndStats.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestXPlaneBuilder.cpp")

if(NOT XSIGMA_ENABLE_NATIVE_PROFILER)
  list(REMOVE_ITEM TestFiles ${XSIGMA_NATIVE_PROFILER_TESTS})
  message(STATUS "Native profiler disabled: excluding native-only Core tests")
endif()

# ============================================================================= CoreCxxTests
# Executable
# =============================================================================
add_executable(CoreCxxTests ${TestFiles})

# Compile definitions
target_compile_definitions(CoreCxxTests PRIVATE XSIGMA_GOOGLE_TEST USE_GTEST _VARIADIC_MAX=10)

# Link libraries Core library is linked which transitively brings in all centralized dependencies
# via XSIGMA_DEPENDENCY_LIBS
target_link_libraries(CoreCxxTests PRIVATE XSigma::gtest XSigma::gtest_main XSigma::Core)

# Set target properties
set_target_properties(
  CoreCxxTests PROPERTIES FOLDER "Tests" VS_DEBUGGER_WORKING_DIRECTORY
                                         "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

# ============================================================================= Google Test
# Discovery for CTest and Visual Studio Test Explorer
# =============================================================================

# Register test with CTest
add_test(NAME CoreCxxTests COMMAND CoreCxxTests
         WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

# Set test properties
set_tests_properties(CoreCxxTests PROPERTIES TIMEOUT 300 LABELS "Core;Unit")

# Set coverage environment variable if enabled
if(XSIGMA_ENABLE_COVERAGE AND DEFINED XSIGMA_LLVM_PROFILE_FILE)
  set_tests_properties(
    CoreCxxTests PROPERTIES ENVIRONMENT "LLVM_PROFILE_FILE=${XSIGMA_LLVM_PROFILE_FILE}"
  )
endif()

message(STATUS "✓ CoreCxxTests registered with CTest")
message(STATUS "  - Working directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# ============================================================================= Output Directory
# Configuration
# =============================================================================
if(COMMAND xsigma_set_target_output_directories)
  xsigma_set_target_output_directories(CoreCxxTests)
endif()

# ============================================================================= Individual Benchmark
# Executables
# =============================================================================
# Each benchmark is built as a separate executable for:
# - Parallel compilation (faster builds)
# - Selective rebuilding (only rebuild changed benchmarks)
# - Independent execution (run specific benchmarks in isolation)
# - Better CTest integration (individual test reporting)
# =============================================================================
if(XSIGMA_ENABLE_BENCHMARK)

  # Discover all benchmark source files
  file(GLOB_RECURSE bench_sources LIST_DIRECTORIES false
       "${CMAKE_CURRENT_SOURCE_DIR}/Benchmark*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Benchmark*.cu"
  )

  # Exclude CUDA-related benchmarks when GPU support is disabled
  if(NOT XSIGMA_ENABLE_CUDA AND NOT XSIGMA_ENABLE_HIP)
    set(filtered_bench_sources)
    foreach(_bench_source IN ITEMS ${bench_sources})
      get_filename_component(file_name "${_bench_source}" NAME)
      # Exclude CUDA-related benchmarks
      if(NOT file_name MATCHES ".*[Cc]uda.*")
        list(APPEND filtered_bench_sources "${_bench_source}")
      else()
        message(STATUS "Excluding CUDA benchmark (no GPU support): ${file_name}")
      endif()
    endforeach()
    set(bench_sources ${filtered_bench_sources})
  endif()

  # Create a separate executable target for each benchmark file
  foreach(_bench_source IN ITEMS ${bench_sources})
    # Extract benchmark name from filename (remove .cpp/.cu extension)
    get_filename_component(bench_name "${_bench_source}" NAME_WE)

    # Create executable target with consistent naming: benchmark_<name>
    # Example: BenchmarkSMP.cpp -> benchmark_smp
    string(REPLACE "Benchmark" "" bench_suffix "${bench_name}")
    string(TOLOWER "${bench_suffix}" bench_suffix_lower)
    set(target_name "benchmark_${bench_suffix_lower}")

    # Create the executable
    add_executable(${target_name} "${_bench_source}")

    # Link libraries
    # - XSigma::benchmark: Google Benchmark library
    # - XSigma::benchmark_main: Provides main() function
    # - XSigma::Core: Core library with all dependencies
    target_link_libraries(${target_name} PRIVATE
      XSigma::benchmark
      XSigma::benchmark_main
      XSigma::Core
    )

    # Set target properties
    set_target_properties(${target_name} PROPERTIES
      FOLDER "Benchmarks"
      OUTPUT_NAME "${target_name}"
    )

    # Apply XSigma output directory configuration
    # This ensures benchmarks are placed in ${XSIGMA_BINARY_DIR}/bin
    if(COMMAND xsigma_set_target_output_directories)
      xsigma_set_target_output_directories(${target_name})
    else()
      # Fallback: use standard output directories
      set_target_properties(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
      )
    endif()

    # Register with CTest for integrated test execution
    # This allows running benchmarks via: ctest -R benchmark_
    # Or specific benchmark: ctest -R benchmark_smp
    add_test(
      NAME ${target_name}
      COMMAND ${target_name} --benchmark_min_time=0.01s
      WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    )

    # Set test properties
    set_tests_properties(${target_name} PROPERTIES
      TIMEOUT 600  # 10 minutes timeout for benchmarks
      LABELS "Benchmark;Performance;Core"
    )

    message(STATUS "✓ Benchmark target configured: ${target_name} (from ${bench_name}.cpp)")
  endforeach()

  # Report total number of benchmarks configured
  list(LENGTH bench_sources bench_count)
  message(STATUS "✓ Individual benchmark executables configured (${bench_count} total)")
endif()

# ============================================================================= ProfileParallelBackends
# Executable (Profiling Tool)
# =============================================================================
# This executable profiles the parallel backends (OpenMP, TBB, Native) using Kineto
# Build separately for each backend to compare performance

add_executable(ProfileParallelBackends ProfileParallelBackends.cpp)

# Link libraries
target_link_libraries(ProfileParallelBackends PRIVATE XSigma::Core)

# Set target properties
set_target_properties(ProfileParallelBackends PROPERTIES FOLDER "Profiling")

# Apply XSigma output directory configuration
if(COMMAND xsigma_set_target_output_directories)
  xsigma_set_target_output_directories(ProfileParallelBackends)
endif()

message(STATUS "✓ ProfileParallelBackends executable configured")
message(STATUS "  - Backend: ${XSIGMA_PARALLEL_BACKEND}")
message(STATUS "  - OpenMP: ${XSIGMA_ENABLE_OPENMP}")
message(STATUS "  - TBB: ${XSIGMA_ENABLE_TBB}")
