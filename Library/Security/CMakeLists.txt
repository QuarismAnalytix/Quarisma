# Security module CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

set(quarisma_security_headers)
set(quarisma_security_sources)

# Collect all header and source files
file(GLOB_RECURSE headers LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

file(GLOB_RECURSE sources LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx")

# Remove test files from the library sources
list(FILTER sources EXCLUDE REGEX ".*/Testing/.*")
list(FILTER headers EXCLUDE REGEX ".*/Testing/.*")

# Create the Security library Respect BUILD_SHARED_LIBS setting for library type
add_library(Security ${sources} ${headers})

# Set appropriate compile definitions based on library type
if(BUILD_SHARED_LIBS)
  target_compile_definitions(Security PUBLIC QUARISMA_SHARED_DEFINE PRIVATE QUARISMA_BUILDING_DLL)
  # Set output directories using Quarisma's standardized approach
  if(COMMAND quarisma_set_target_output_directories)
    quarisma_set_target_output_directories(Security)
  endif()
else()
  target_compile_definitions(Security PUBLIC QUARISMA_STATIC_DEFINE)
endif()

# Create alias for consistent naming
add_library(Quarisma::Security ALIAS Security)

# Enable Link Time Optimization (LTO) for Security library if enabled
if(QUARISMA_ENABLE_LTO)
  set_target_properties(Security PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
  message(STATUS "Security library: Link Time Optimization (LTO) enabled")
else()
  message(STATUS "Security library: Link Time Optimization (LTO) disabled")
endif()

# Apply IWYU to the Security target (if available)
if(COMMAND quarisma_apply_iwyu)
  quarisma_apply_iwyu(Security)
endif()

# Configure Visual Studio source groups to preserve directory structure
quarisma_module_create_filters("${CMAKE_CURRENT_SOURCE_DIR}")

# Set target properties
set_target_properties(
  Security PROPERTIES CXX_STANDARD ${QUARISMA_CXX_STANDARD} CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS
                                                                                   OFF
)

if(MSVC)
  target_compile_options(Security PRIVATE /WX)
endif()

# ============================================================================= Include Directories
# =============================================================================
# Security library include directories are split into: 1. Security's own directories (PUBLIC for
# external consumers) 2. Centralized third-party include directories (PUBLIC to propagate to
# dependent targets)

target_include_directories(
  Security
  PUBLIC $<BUILD_INTERFACE:${QUARISMA_SOURCE_DIR}/Library>
         $<BUILD_INTERFACE:${QUARISMA_SOURCE_DIR}/Library/Core>
         $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/Library/Core>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
         $<INSTALL_INTERFACE:include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add centralized third-party include directories as PUBLIC
if(QUARISMA_DEPENDENCY_INCLUDE_DIRS)
  target_include_directories(Security PUBLIC ${QUARISMA_DEPENDENCY_INCLUDE_DIRS})
  message(STATUS "Security library include directories: ${QUARISMA_DEPENDENCY_INCLUDE_DIRS}")
endif()

# Link with build interface target
target_link_libraries(Security PUBLIC Quarisma::build)

if(COMMAND quarisma_target_clang_tidy)
  quarisma_target_clang_tidy(Security)
endif()

# ============================================================================= Link with
# Centralized Dependencies
# =============================================================================
# Link with centralized dependency list (if needed)
if(QUARISMA_DEPENDENCY_LIBS)
  target_link_libraries(Security PUBLIC ${QUARISMA_DEPENDENCY_LIBS})
  message(STATUS "Security library linked with centralized dependencies: ${QUARISMA_DEPENDENCY_LIBS}")
endif()

# Platform-specific libraries for security module
if(WIN32)
  target_link_libraries(Security PRIVATE bcrypt)
  message(STATUS "Security library: Linked with bcrypt (Windows)")
elseif(APPLE)
  target_link_libraries(Security PRIVATE "-framework Security")
  message(STATUS "Security library: Linked with Security framework (macOS)")
endif()

# ============================================================================= Compile Definitions
# for Optional Features
# =============================================================================
# Apply centralized compile definitions
if(QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS)
  target_compile_definitions(Security PUBLIC ${QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS})
  message(STATUS "Security library compile definitions: ${QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS}")
endif()

# Install targets
install(
  TARGETS Security
  EXPORT QuarismaTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES
  DESTINATION include
)

# Install headers
install(FILES ${headers} DESTINATION include/Quarisma/Security COMPONENT Development)

# Configure testing
if(QUARISMA_BUILD_TESTING STREQUAL "ON" OR QUARISMA_BUILD_TESTING STREQUAL "WANT")
  add_subdirectory(Testing/Cxx)
endif()
