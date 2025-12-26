# ============================================================================= Security Library C++
# Tests
# =============================================================================
# Following Core's simple pattern for test integration

# ============================================================================= Test Source Files
# Discovery
# =============================================================================
file(GLOB_RECURSE test_sources LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/Test*.cxx")

set(TestFiles)
foreach(_test_source IN ITEMS ${test_sources})
  get_filename_component(file_name "${_test_source}" NAME)
  list(APPEND TestFiles "${_test_source}")
endforeach()

# ============================================================================= SecurityCxxTests
# Executable
# =============================================================================
add_executable(SecurityCxxTests ${TestFiles})

# Compile definitions
target_compile_definitions(SecurityCxxTests PRIVATE QUARISMA_GOOGLE_TEST USE_GTEST _VARIADIC_MAX=10)

# Link libraries Security library is linked which transitively brings in all centralized
# dependencies
target_link_libraries(SecurityCxxTests PRIVATE gtest gtest_main Quarisma::Security Quarisma::Core)

# Set target properties
set_target_properties(
  SecurityCxxTests PROPERTIES FOLDER "Tests" VS_DEBUGGER_WORKING_DIRECTORY
                                             "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

# ============================================================================= Google Test
# Discovery for CTest and Visual Studio Test Explorer
# =============================================================================

# Register test with CTest
add_test(NAME SecurityCxxTests COMMAND SecurityCxxTests
         WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

# Set test properties
set_tests_properties(SecurityCxxTests PROPERTIES TIMEOUT 300 LABELS "Security;Unit")

# Set coverage environment variable if enabled
if(QUARISMA_ENABLE_COVERAGE AND DEFINED QUARISMA_LLVM_PROFILE_FILE)
  set_tests_properties(
    SecurityCxxTests PROPERTIES ENVIRONMENT "LLVM_PROFILE_FILE=${QUARISMA_LLVM_PROFILE_FILE}"
  )
endif()

message(STATUS "✓ SecurityCxxTests registered with CTest")
message(STATUS "  - Working directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# ============================================================================= Output Directory
# Configuration
# =============================================================================
if(COMMAND quarisma_set_target_output_directories)
  quarisma_set_target_output_directories(SecurityCxxTests)
endif()
