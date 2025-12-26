# Third-party dependencies for Quarisma
cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0069 NEW)
message(STATUS "DEBUG: Processing ThirdParty/CMakeLists.txt")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Option to use external libraries instead of bundled ones
option(QUARISMA_ENABLE_EXTERNAL "Use external copies of third party libraries by default" OFF)

# Global option to control third-party library type (STATIC, SHARED, or DEFAULT)
# DEFAULT means respect each library's own settings or BUILD_SHARED_LIBS
set(QUARISMA_THIRDPARTY_TYPE "DEFAULT" CACHE STRING "Default library type for third-party libraries (STATIC, SHARED, or DEFAULT)")
set_property(CACHE QUARISMA_THIRDPARTY_TYPE PROPERTY STRINGS "STATIC" "SHARED" "DEFAULT")

# Configure Visual Studio folder structure to match file system
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Disable tests, examples, and benchmarks for all third-party libraries
set(BUILD_TESTING OFF CACHE BOOL "Disable testing for third-party libraries" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "Disable examples for third-party libraries" FORCE)
set(BUILD_BENCHMARKS OFF CACHE BOOL "Disable benchmarks for third-party libraries" FORCE)


# fmt-specific options
set(FMT_TEST OFF CACHE BOOL "Disable fmt tests" FORCE)
set(FMT_DOC OFF CACHE BOOL "Disable fmt documentation" FORCE)
set(FMT_INSTALL OFF CACHE BOOL "Disable fmt install" FORCE)
set(FMT_HEADER_ONLY ON CACHE BOOL "Use header-only fmt library" FORCE)

# cpuinfo-specific options
set(CPUINFO_BUILD_TOOLS OFF CACHE BOOL "Disable cpuinfo tools" FORCE)
set(CPUINFO_BUILD_UNIT_TESTS OFF CACHE BOOL "Disable cpuinfo unit tests" FORCE)
set(CPUINFO_BUILD_MOCK_TESTS OFF CACHE BOOL "Disable cpuinfo mock tests" FORCE)
set(CPUINFO_BUILD_BENCHMARKS OFF CACHE BOOL "Disable cpuinfo benchmarks" FORCE)
set(CPUINFO_LIBRARY_TYPE "static" CACHE STRING "Type of cpuinfo library (shared, static, or default) to build")

# magic_enum-specific options
set(MAGIC_ENUM_OPT_BUILD_EXAMPLES OFF CACHE BOOL "Disable magic_enum examples" FORCE)
set(MAGIC_ENUM_OPT_BUILD_TESTS OFF CACHE BOOL "Disable magic_enum tests" FORCE)

# loguru-specific options
set(LOGURU_BUILD_EXAMPLES OFF CACHE BOOL "Disable loguru examples" FORCE)
#set(LOGURU_USE_FMTLIB ON CACHE BOOL "Enable fmtlib formatting in loguru" FORCE)
set(LOGURU_WITH_STREAMS ON CACHE BOOL "Enable streams in loguru" FORCE)

# glog-specific options
set(BUILD_EXAMPLES OFF CACHE BOOL "Disable glog examples" FORCE)
set(WITH_GFLAGS OFF CACHE BOOL "Disable gflags dependency for glog" FORCE)
set(WITH_GTEST OFF CACHE BOOL "Disable gtest for glog" FORCE)
set(WITH_GMOCK OFF CACHE BOOL "Disable gmock for glog" FORCE)

# googletest-specific options
set(BUILD_GMOCK OFF CACHE BOOL "Disable Google Mock" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "Disable Google Test install" FORCE)
set(gtest_build_samples OFF CACHE BOOL "Disable Google Test samples" FORCE)
set(gtest_build_tests OFF CACHE BOOL "Disable Google Test tests" FORCE)



# Google Benchmark-specific options
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark tests" FORCE)
set(BENCHMARK_ENABLE_EXCEPTIONS ON CACHE BOOL "Enable benchmark exceptions" FORCE)
set(BENCHMARK_ENABLE_LTO OFF CACHE BOOL "Disable benchmark LTO" FORCE)
set(BENCHMARK_USE_LIBCXX OFF CACHE BOOL "Disable benchmark libcxx" FORCE)
set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "Disable benchmark werror" FORCE)
set(BENCHMARK_FORCE_WERROR OFF CACHE BOOL "Disable benchmark force werror" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "Disable benchmark install" FORCE)
set(BENCHMARK_INSTALL_DOCS OFF CACHE BOOL "Disable benchmark docs install" FORCE)
set(BENCHMARK_ENABLE_DOXYGEN OFF CACHE BOOL "Disable benchmark doxygen" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "Disable benchmark gtest tests" FORCE)
set(BENCHMARK_USE_BUNDLED_GTEST OFF CACHE BOOL "Don't use bundled gtest for benchmark" FORCE)
set(BENCHMARK_DOWNLOAD_DEPENDENCIES OFF CACHE BOOL "Don't download dependencies" FORCE)

# mimalloc-specific options
set(MI_BUILD_TESTS OFF CACHE BOOL "Disable mimalloc tests" FORCE)
set(MI_BUILD_OBJECT OFF CACHE BOOL "Disable mimalloc object library" FORCE)
set(MI_INSTALL_TOPLEVEL OFF CACHE BOOL "Disable mimalloc top-level install" FORCE)
set(MI_OVERRIDE OFF CACHE BOOL "Disable mimalloc malloc override" FORCE)
set(MI_BUILD_SHARED OFF CACHE BOOL "Build mimalloc as static library" FORCE)
set(MI_BUILD_STATIC ON CACHE BOOL "Build mimalloc as static library" FORCE)
# Note: MI_BUILD_STATIC is now controlled by add_third_party_library LIBRARY_TYPE parameter

# =============================================================================
# Helper Functions for Third-Party Library Management
# =============================================================================

# -----------------------------------------------------------------------------
# _filter_flags_internal: Remove specified flags from a flag string
# -----------------------------------------------------------------------------
# Internal helper to filter out specific flags from a string of compiler/linker flags
#
# Parameters:
#   input_flags - The flag string to filter
#   flags_to_remove - List of flags to remove (can be regex patterns)
#   output_var - Variable name to store the filtered result
#
function(_filter_flags_internal input_flags flags_to_remove output_var)
    set(result "${input_flags}")

    foreach(flag_pattern ${flags_to_remove})
        # Remove the flag pattern from the string
        string(REGEX REPLACE "${flag_pattern}( |$)" "" result "${result}")
    endforeach()

    # Clean up multiple spaces
    string(REGEX REPLACE "  +" " " result "${result}")
    string(STRIP "${result}" result)

    set(${output_var} "${result}" PARENT_SCOPE)
endfunction()

# -----------------------------------------------------------------------------
# _save_third_party_flags: Save and optionally filter compiler/linker flags
# -----------------------------------------------------------------------------
# Saves current CMAKE_CXX_FLAGS, CMAKE_C_FLAGS, and linker flags, then optionally
# filters out problematic flags that shouldn't be applied to third-party libraries
#
# Parameters:
#   name - Library name (for namespacing saved variables)
#   filter_flags - List of flag patterns to remove (optional)
#
function(_save_third_party_flags name filter_flags)
    # Save original flags to parent scope
    set(_SAVED_CMAKE_CXX_FLAGS_${name} "${CMAKE_CXX_FLAGS}" PARENT_SCOPE)
    set(_SAVED_CMAKE_C_FLAGS_${name} "${CMAKE_C_FLAGS}" PARENT_SCOPE)
    set(_SAVED_CMAKE_EXE_LINKER_FLAGS_${name} "${CMAKE_EXE_LINKER_FLAGS}" PARENT_SCOPE)
    set(_SAVED_CMAKE_SHARED_LINKER_FLAGS_${name} "${CMAKE_SHARED_LINKER_FLAGS}" PARENT_SCOPE)
    set(_SAVED_CMAKE_MODULE_LINKER_FLAGS_${name} "${CMAKE_MODULE_LINKER_FLAGS}" PARENT_SCOPE)
    set(_SAVED_CMAKE_STATIC_LINKER_FLAGS_${name} "${CMAKE_STATIC_LINKER_FLAGS}" PARENT_SCOPE)

    # If filter flags are provided, filter them out
    if(filter_flags)
        _filter_flags_internal("${CMAKE_CXX_FLAGS}" "${filter_flags}" filtered_cxx_flags)
        _filter_flags_internal("${CMAKE_C_FLAGS}" "${filter_flags}" filtered_c_flags)
        _filter_flags_internal("${CMAKE_EXE_LINKER_FLAGS}" "${filter_flags}" filtered_exe_linker_flags)
        _filter_flags_internal("${CMAKE_SHARED_LINKER_FLAGS}" "${filter_flags}" filtered_shared_linker_flags)
        _filter_flags_internal("${CMAKE_MODULE_LINKER_FLAGS}" "${filter_flags}" filtered_module_linker_flags)
        _filter_flags_internal("${CMAKE_STATIC_LINKER_FLAGS}" "${filter_flags}" filtered_static_linker_flags)

        # Apply filtered flags to parent scope
        set(CMAKE_CXX_FLAGS "${filtered_cxx_flags}" PARENT_SCOPE)
        set(CMAKE_C_FLAGS "${filtered_c_flags}" PARENT_SCOPE)
        set(CMAKE_EXE_LINKER_FLAGS "${filtered_exe_linker_flags}" PARENT_SCOPE)
        set(CMAKE_SHARED_LINKER_FLAGS "${filtered_shared_linker_flags}" PARENT_SCOPE)
        set(CMAKE_MODULE_LINKER_FLAGS "${filtered_module_linker_flags}" PARENT_SCOPE)
        set(CMAKE_STATIC_LINKER_FLAGS "${filtered_static_linker_flags}" PARENT_SCOPE)

        message(STATUS "Filtered flags for ${name}: removed ${filter_flags}")
    endif()
endfunction()

# -----------------------------------------------------------------------------
# _restore_third_party_flags: Restore original compiler/linker flags
# -----------------------------------------------------------------------------
# Restores the compiler and linker flags that were saved before building
# a third-party library
#
# Parameters:
#   name - Library name (for namespacing saved variables)
#
function(_restore_third_party_flags name)
    # Restore original flags to parent scope
    set(CMAKE_CXX_FLAGS "${_SAVED_CMAKE_CXX_FLAGS_${name}}" PARENT_SCOPE)
    set(CMAKE_C_FLAGS "${_SAVED_CMAKE_C_FLAGS_${name}}" PARENT_SCOPE)
    set(CMAKE_EXE_LINKER_FLAGS "${_SAVED_CMAKE_EXE_LINKER_FLAGS_${name}}" PARENT_SCOPE)
    set(CMAKE_SHARED_LINKER_FLAGS "${_SAVED_CMAKE_SHARED_LINKER_FLAGS_${name}}" PARENT_SCOPE)
    set(CMAKE_MODULE_LINKER_FLAGS "${_SAVED_CMAKE_MODULE_LINKER_FLAGS_${name}}" PARENT_SCOPE)
    set(CMAKE_STATIC_LINKER_FLAGS "${_SAVED_CMAKE_STATIC_LINKER_FLAGS_${name}}" PARENT_SCOPE)
endfunction()

# -----------------------------------------------------------------------------
# _set_third_party_folder_properties: Set FOLDER property for all targets
# -----------------------------------------------------------------------------
# Automatically discovers all targets created by a third-party library and
# sets their FOLDER property for IDE organization. This function recursively
# traverses all subdirectories to find targets at any nesting level.
#
# IMPORTANT: CMake's BUILDSYSTEM_TARGETS property only returns targets defined
# directly in a directory, not in subdirectories. This function uses recursive
# traversal via the SUBDIRECTORIES property to discover ALL targets.
#
# Parameters:
#   name - Library name
#   binary_dir - Binary directory where the library was built
#
function(_set_third_party_folder_properties name binary_dir)
    # Helper function to recursively collect targets from a directory tree
    # This is necessary because BUILDSYSTEM_TARGETS only returns targets
    # defined directly in the specified directory, not in subdirectories
    function(_collect_targets_recursive dir collected_targets_var)
        # Get targets defined directly in this directory
        get_property(dir_targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)

        if(dir_targets)
            list(APPEND local_targets ${dir_targets})
        endif()

        # Get all subdirectories in this directory
        get_property(subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)

        # Recursively process each subdirectory
        foreach(subdir ${subdirs})
            _collect_targets_recursive("${subdir}" local_targets)
        endforeach()

        # Return the collected targets to parent scope
        set(${collected_targets_var} "${local_targets}" PARENT_SCOPE)
    endfunction()

    # Collect all targets recursively starting from the binary directory
    _collect_targets_recursive("${binary_dir}" all_targets)

    if(all_targets)
        message(STATUS "Setting FOLDER property for ${name} targets: ${all_targets}")
        foreach(target ${all_targets})
            if(TARGET ${target})
                set_target_properties(${target} PROPERTIES FOLDER "ThirdParty/${name}")
            endif()
        endforeach()
    else()
        message(STATUS "No targets found for ${name} in ${binary_dir} (this may be normal for header-only libraries)")
    endif()
endfunction()

# -----------------------------------------------------------------------------
# _create_third_party_aliases: Create library aliases
# -----------------------------------------------------------------------------
# Creates alias targets for a third-party library to provide consistent
# naming conventions (e.g., fmt::fmt as an alias for fmt)
#
# Parameters:
#   name - Library name
#   aliases - List of alias names to create
#
function(_create_third_party_aliases name aliases)
    if(NOT aliases)
        return()
    endif()

    foreach(alias ${aliases})
        # Check if the alias already exists
        if(NOT TARGET ${alias})
            # Check if the base target exists
            if(TARGET ${name})
                add_library(${alias} ALIAS ${name})
                message(STATUS "Created alias ${alias} for ${name}")
            else()
                message(WARNING "Cannot create alias ${alias}: target ${name} does not exist")
            endif()
        else()
            message(STATUS "Alias ${alias} already exists, skipping")
        endif()
    endforeach()
endfunction()

# -----------------------------------------------------------------------------
# _create_third_party_interface_targets: Create Quarisma:: namespace aliases
# -----------------------------------------------------------------------------
# Creates Quarisma:: namespace aliases for third-party library targets.
# This provides a consistent interface for linking against third-party libraries
# throughout the Quarisma project.
#
# The function handles multiple scenarios:
# 1. Target with namespace alias (e.g., fmt::fmt) - resolves the actual target
# 2. Plain target (e.g., fmt) - creates alias directly
# 3. Multiple possible target names - tries each in order
#
# Parameters:
#   quarisma_targets - List of Quarisma target specifications in format:
#                    "Quarisma::name=target1|target2|..."
#                    where target1, target2, etc. are tried in order
#
# Example:
#   _create_third_party_interface_targets(
#       "Quarisma::fmt=fmt::fmt|fmt"
#       "Quarisma::gtest=GTest::gtest|gtest;Quarisma::gtest_main=GTest::gtest_main|gtest_main"
#   )
#
function(_create_third_party_interface_targets quarisma_targets)
    if(NOT quarisma_targets)
        return()
    endif()

    foreach(target_spec ${quarisma_targets})
        # Parse the specification: "Quarisma::name=target1|target2|..."
        string(REPLACE "=" ";" spec_parts "${target_spec}")
        list(LENGTH spec_parts spec_parts_len)

        if(NOT spec_parts_len EQUAL 2)
            message(WARNING "Invalid Quarisma target specification: ${target_spec}")
            continue()
        endif()

        list(GET spec_parts 0 quarisma_name)
        list(GET spec_parts 1 source_targets)

        # Check if Quarisma target already exists
        if(TARGET ${quarisma_name})
            message(STATUS "Quarisma target ${quarisma_name} already exists, skipping")
            continue()
        endif()

        # Split source targets by pipe (|) to get alternatives
        string(REPLACE "|" ";" source_target_list "${source_targets}")

        set(target_created FALSE)
        foreach(source_target ${source_target_list})
            if(TARGET ${source_target})
                # Check if source target is an alias and resolve it
                get_target_property(aliased_target ${source_target} ALIASED_TARGET)

                if(aliased_target)
                    # Source is an alias, create Quarisma alias to the actual target
                    add_library(${quarisma_name} ALIAS ${aliased_target})
                    message(STATUS "Created ${quarisma_name} -> ${source_target} (resolved to ${aliased_target})")
                else()
                    # Source is a real target, create alias directly
                    add_library(${quarisma_name} ALIAS ${source_target})
                    message(STATUS "Created ${quarisma_name} -> ${source_target}")
                endif()

                set(target_created TRUE)
                break()
            endif()
        endforeach()

        if(NOT target_created)
            message(STATUS "Could not create ${quarisma_name}: none of the source targets exist (${source_targets})")
        endif()
    endforeach()
endfunction()

# =============================================================================
# Main Function: add_third_party_library
# =============================================================================
# Enhanced function to add third-party libraries with comprehensive configuration
# options including flag filtering, automatic target organization, and alias creation
#
# Parameters:
#   name - Name of the third-party library (required)
#
# Optional Parameters:
#   SOURCE_DIR <path>           - Custom source directory path relative to ThirdParty/
#                                 If not specified, uses the library name as the path
#                                 Example: "kineto/libkineto" for nested structures
#   EXTERNAL_NAME <name>        - Name to use when searching for external package
#   CONDITION <variable>        - CMake variable that must be true to add the library
#   LIBRARY_TYPE <type>         - Type of library: STATIC, SHARED, or DEFAULT
#                                 If not specified, uses QUARISMA_THIRDPARTY_TYPE
#   LIBRARY_TYPE_VAR <var>      - CMake variable name for library-specific control
#                                 Example: MI_BUILD_STATIC for mimalloc
#   FILTER_FLAGS <flags...>     - List of compiler/linker flags to filter out
#                                 Example: "-fno-exceptions" "-Werror"
#   DISABLE_WARNINGS            - Boolean flag to disable all compiler warnings
#                                 Adds -w for Clang/GCC and /w for MSVC
#   EXTRA_FLAGS <flags...>      - List of additional compiler flags to add
#                                 Example: "-flto=thin" "-march=native"
#   ALIASES <aliases...>        - List of alias names to create for the library
#                                 Example: "fmt::fmt" for fmt library
#   QUARISMA_TARGETS <specs...>   - List of Quarisma:: namespace target specifications
#                                 Format: "Quarisma::name=target1|target2|..."
#                                 Example: "Quarisma::fmt=fmt::fmt|fmt"
#
# Example Usage:
#   add_third_party_library(fmt
#       LIBRARY_TYPE STATIC
#       FILTER_FLAGS "-fno-exceptions" "-Werror"
#       ALIASES "fmt::fmt"
#       QUARISMA_TARGETS "Quarisma::fmt=fmt::fmt|fmt"
#   )
#
#   add_third_party_library(kineto
#       CONDITION QUARISMA_ENABLE_KINETO
#       LIBRARY_TYPE STATIC
#       SOURCE_DIR kineto/libkineto
#       DISABLE_WARNINGS
#       EXTRA_FLAGS "-flto=thin"
#   )
#
function(add_third_party_library name)
    # Parse function arguments
    cmake_parse_arguments(
        TPL                                                           # Prefix
        "DISABLE_WARNINGS"                                # Options (boolean flags)
        "SOURCE_DIR;EXTERNAL_NAME;CONDITION;LIBRARY_TYPE;LIBRARY_TYPE_VAR" # One-value keywords
        "FILTER_FLAGS;EXTRA_FLAGS;ALIASES;QUARISMA_TARGETS"             # Multi-value keywords
        ${ARGN}                                                       # Arguments to parse
    )

    # Set default values for optional parameters
    if(NOT DEFINED TPL_EXTERNAL_NAME)
        set(TPL_EXTERNAL_NAME ${name})
    endif()

    # SOURCE_DIR defaults to the library name if not specified
    if(NOT DEFINED TPL_SOURCE_DIR)
        set(TPL_SOURCE_DIR ${name})
    endif()

    # -------------------------------------------------------------------------
    # Step 1: Check condition (early return pattern)
    # -------------------------------------------------------------------------
    if(DEFINED TPL_CONDITION)
        if(NOT ${TPL_CONDITION})
            message(STATUS "Skipping ${name}: condition ${TPL_CONDITION} is not met")
            return()
        endif()
    endif()

    # -------------------------------------------------------------------------
    # Step 2: Check if already processed to avoid duplicates
    # -------------------------------------------------------------------------
    get_property(QUARISMA_${name}_PROCESSED GLOBAL PROPERTY QUARISMA_${name}_PROCESSED)
    if(QUARISMA_${name}_PROCESSED)
        message(STATUS "Third-party library ${name} already processed, skipping")
        return()
    endif()
    set_property(GLOBAL PROPERTY QUARISMA_${name}_PROCESSED TRUE)

    # -------------------------------------------------------------------------
    # Step 3: Try to find external library first if requested
    # -------------------------------------------------------------------------
    if(QUARISMA_ENABLE_EXTERNAL)
        find_package(${TPL_EXTERNAL_NAME} QUIET)
        if(${TPL_EXTERNAL_NAME}_FOUND OR ${name}_FOUND)
            message(STATUS "Using external ${name}")
            if(TPL_ALIASES AND TARGET ${name})
                _create_third_party_aliases(${name} "${TPL_ALIASES}")
            endif()
            if(TPL_QUARISMA_TARGETS)
                _create_third_party_interface_targets("${TPL_QUARISMA_TARGETS}")
            endif()
            return()
        endif()
    endif()

    # -------------------------------------------------------------------------
    # Step 4: Determine library type
    # -------------------------------------------------------------------------
    # Priority: LIBRARY_TYPE parameter > QUARISMA_THIRDPARTY_TYPE global > DEFAULT
    set(lib_type "DEFAULT")
    if(DEFINED TPL_LIBRARY_TYPE)
        set(lib_type ${TPL_LIBRARY_TYPE})
    elseif(NOT QUARISMA_THIRDPARTY_TYPE STREQUAL "DEFAULT")
        set(lib_type ${QUARISMA_THIRDPARTY_TYPE})
    endif()

    # -------------------------------------------------------------------------
    # Step 5: Save current state (BUILD_SHARED_LIBS and flags)
    # -------------------------------------------------------------------------
    set(_saved_build_shared_libs ${BUILD_SHARED_LIBS})

    # Save and optionally filter compiler/linker flags when we plan to mutate them
    set(_TPL_SAVED_FLAGS FALSE)
    set(_tpl_filter_flags "")
    if(TPL_FILTER_FLAGS)
        set(_tpl_filter_flags "${TPL_FILTER_FLAGS}")
    endif()
    if(TPL_FILTER_FLAGS OR TPL_DISABLE_WARNINGS OR TPL_EXTRA_FLAGS)
        _save_third_party_flags(${name} "${_tpl_filter_flags}")
        set(_TPL_SAVED_FLAGS TRUE)
    endif()

    # -------------------------------------------------------------------------
    # Step 5b: Handle DISABLE_WARNINGS and EXTRA_FLAGS
    # -------------------------------------------------------------------------
    if(TPL_DISABLE_WARNINGS)
        # Remove all existing warning-related flags
        # Patterns: -W*, -Wno-*, /wd* (but keep /W* for MSVC warning level)
        if(MSVC)
            # For MSVC: remove only /wd* (specific warning disables) and -W* flags
            _filter_flags_internal("${CMAKE_CXX_FLAGS}" "-W.*;/wd.*" filtered_cxx_flags_no_warnings)
            _filter_flags_internal("${CMAKE_C_FLAGS}" "-W.*;/wd.*" filtered_c_flags_no_warnings)

            set(CMAKE_CXX_FLAGS "${filtered_cxx_flags_no_warnings}")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" PARENT_SCOPE)
            set(CMAKE_C_FLAGS "${filtered_c_flags_no_warnings}")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" PARENT_SCOPE)

            # Add /W0 to disable all warnings while keeping the warning level infrastructure
            string(APPEND CMAKE_CXX_FLAGS " /W0")
            string(APPEND CMAKE_C_FLAGS " /W0")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" PARENT_SCOPE)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" PARENT_SCOPE)
        else()
            # For Clang/GCC: remove all -W* and -Wno-* flags
            _filter_flags_internal("${CMAKE_CXX_FLAGS}" "-W.*;-Wno-.*" filtered_cxx_flags_no_warnings)
            _filter_flags_internal("${CMAKE_C_FLAGS}" "-W.*;-Wno-.*" filtered_c_flags_no_warnings)

            set(CMAKE_CXX_FLAGS "${filtered_cxx_flags_no_warnings}")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" PARENT_SCOPE)
            set(CMAKE_C_FLAGS "${filtered_c_flags_no_warnings}")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" PARENT_SCOPE)

            # Add -w to disable all warnings
            string(APPEND CMAKE_CXX_FLAGS " -w")
            string(APPEND CMAKE_C_FLAGS " -w")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" PARENT_SCOPE)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" PARENT_SCOPE)
        endif()
        message(STATUS "Disabled all warnings for ${name} (removed existing warning flags and added suppression)")
    endif()

    # Add extra compiler flags if provided
    if(TPL_EXTRA_FLAGS)
        foreach(flag ${TPL_EXTRA_FLAGS})
            string(APPEND CMAKE_CXX_FLAGS " ${flag}")
            string(APPEND CMAKE_C_FLAGS " ${flag}")
        endforeach()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" PARENT_SCOPE)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" PARENT_SCOPE)
        message(STATUS "Added extra flags for ${name}: ${TPL_EXTRA_FLAGS}")
    endif()

    # -------------------------------------------------------------------------
    # Step 6: Configure library type
    # -------------------------------------------------------------------------
    if(lib_type STREQUAL "STATIC")
        set(BUILD_SHARED_LIBS OFF)
        message(STATUS "Configuring ${name} as STATIC library")

        # Set library-specific variable if provided
        if(DEFINED TPL_LIBRARY_TYPE_VAR)
            set(${TPL_LIBRARY_TYPE_VAR} ON CACHE BOOL "Build ${name} as static library" FORCE)
        endif()
    elseif(lib_type STREQUAL "SHARED")
        set(BUILD_SHARED_LIBS ON)
        message(STATUS "Configuring ${name} as SHARED library")

        # Set library-specific variable if provided
        if(DEFINED TPL_LIBRARY_TYPE_VAR)
            set(${TPL_LIBRARY_TYPE_VAR} OFF CACHE BOOL "Build ${name} as shared library" FORCE)
        endif()
    else()
        message(STATUS "Configuring ${name} with DEFAULT library type")
    endif()

    # -------------------------------------------------------------------------
    # Step 7: Add the third-party library subdirectory
    # -------------------------------------------------------------------------
    # Support both standard (SOURCE_DIR/CMakeLists.txt) and nested (SOURCE_DIR/subdir/CMakeLists.txt) structures
    set(lib_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/${TPL_SOURCE_DIR}")
    set(lib_cmakelists_path "${lib_source_dir}/CMakeLists.txt")

    # If CMakeLists.txt doesn't exist at the top level, check if it's in a subdirectory
    # This handles cases like kineto/libkineto where the actual CMakeLists.txt is nested
    if(NOT EXISTS "${lib_cmakelists_path}")
        # Try to find CMakeLists.txt in immediate subdirectories
        file(GLOB subdirs LIST_DIRECTORIES true "${lib_source_dir}/*")
        foreach(subdir ${subdirs})
            if(IS_DIRECTORY "${subdir}")
                get_filename_component(subdir_name "${subdir}" NAME)
                set(potential_cmakelists "${subdir}/CMakeLists.txt")
                if(EXISTS "${potential_cmakelists}")
                    set(lib_source_dir "${subdir}")
                    set(lib_cmakelists_path "${potential_cmakelists}")
                    message(STATUS "Found CMakeLists.txt in nested directory: ${subdir_name}")
                    break()
                endif()
            endif()
        endforeach()
    endif()

    if(EXISTS "${lib_cmakelists_path}")
        message(STATUS "Using bundled ${name}")

        # Store the binary directory for later use
        # Use the library name (not the source path) for the binary directory
        set(lib_binary_dir "${CMAKE_CURRENT_BINARY_DIR}/${name}_build")

        # Add the library
        add_subdirectory(${lib_source_dir} ${lib_binary_dir})

        # -------------------------------------------------------------------------
        # Step 8: Automatically set FOLDER properties for IDE organization
        # -------------------------------------------------------------------------
        _set_third_party_folder_properties(${name} "${lib_binary_dir}")
        
        # -------------------------------------------------------------------------
        # Step 9: Create library aliases if requested
        # -------------------------------------------------------------------------
        if(TPL_ALIASES)
            _create_third_party_aliases(${name} "${TPL_ALIASES}")
        endif()

        # -------------------------------------------------------------------------
        # Step 10: Create Quarisma:: namespace interface targets if requested
        # -------------------------------------------------------------------------
        if(TPL_QUARISMA_TARGETS)
            _create_third_party_interface_targets("${TPL_QUARISMA_TARGETS}")
        endif()
    else()
        message(WARNING "Third-party library ${name} not found in ${CMAKE_CURRENT_SOURCE_DIR}/${TPL_SOURCE_DIR}")
    endif()

    # -------------------------------------------------------------------------
    # Step 11: Restore original state
    # -------------------------------------------------------------------------
    # Restore BUILD_SHARED_LIBS
    set(BUILD_SHARED_LIBS ${_saved_build_shared_libs})

    # Restore compiler/linker flags if they were filtered
    if(_TPL_SAVED_FLAGS)
        _restore_third_party_flags(${name})
    endif()
endfunction()

# =============================================================================
# Third-Party Library Declarations
# =============================================================================

# Add third-party libraries conditionally based on QUARISMA_ENABLE_XXX options

# fmt - Modern formatting library (header-only mode)
# Using header-only version to avoid shared library issues and ensure compatibility with Kineto
add_third_party_library(fmt
    LIBRARY_TYPE STATIC
    FILTER_FLAGS "-flto=thin" "-flto=full"
    QUARISMA_TARGETS "Quarisma::fmt=fmt::fmt-header-only|fmt::fmt|fmt"
)

# cpuinfo - CPU feature detection library (mandatory dependency)
add_third_party_library(cpuinfo
    FILTER_FLAGS "-flto=thin" "-flto=full"
    QUARISMA_TARGETS "Quarisma::cpuinfo=cpuinfo"
)

# magic_enum - Static reflection for enums in C++
add_third_party_library(magic_enum
    CONDITION QUARISMA_ENABLE_MAGICENUM
    FILTER_FLAGS "-flto=thin" "-flto=full"
    QUARISMA_TARGETS "Quarisma::magic_enum=magic_enum::magic_enum|magic_enum"
)

# loguru - Lightweight C++ logging library (forced to static)
add_third_party_library(loguru
    CONDITION QUARISMA_ENABLE_LOGURU
    LIBRARY_TYPE STATIC
    FILTER_FLAGS "-flto=thin" "-flto=full"
    QUARISMA_TARGETS "Quarisma::loguru=loguru::loguru|loguru"
)

# glog - Google logging library
add_third_party_library(glog
    CONDITION QUARISMA_ENABLE_GLOG
    FILTER_FLAGS "-flto=thin" "-flto=full"
    QUARISMA_TARGETS "Quarisma::glog=glog::glog|glog"
)

# mimalloc - Microsoft's high-performance memory allocator (conditional)
# By default builds as static library, controlled by LIBRARY_TYPE parameter
add_third_party_library(mimalloc
    CONDITION QUARISMA_ENABLE_MIMALLOC
    LIBRARY_TYPE STATIC
    LIBRARY_TYPE_VAR MI_BUILD_STATIC
    FILTER_FLAGS "-Wno-strict-prototypes" "-flto=thin" "-flto=full"
    QUARISMA_TARGETS "Quarisma::mimalloc=mimalloc|mimalloc-static"
)

# Kineto - PyTorch profiling library
# Aligned with PyTorch's QUARISMA_ENABLE_KINETO configuration for feature parity.
# Supports GPU backends: CUPTI (NVIDIA), ROCtracer (AMD), XPUPTI (Intel XPU), AIUPTI (AI accelerators)

# Configure GPU backend support based on enabled platforms
set(LIBKINETO_NOAIUPTI ON CACHE STRING "" FORCE)
if(NOT QUARISMA_ENABLE_CUDA)
    set(LIBKINETO_NOCUPTI ON CACHE STRING "" FORCE)
else()
    set(LIBKINETO_NOCUPTI OFF CACHE STRING "" FORCE)
    message(STATUS "Using Kineto with CUPTI support")
endif()

if(NOT USE_ROCM)
    set(LIBKINETO_NOROCTRACER ON CACHE STRING "" FORCE)
else()
    set(LIBKINETO_NOROCTRACER OFF CACHE STRING "" FORCE)
    message(STATUS "Using Kineto with Roctracer support")
endif()

if(LIBKINETO_NOCUPTI AND LIBKINETO_NOROCTRACER AND LIBKINETO_NOXPUPTI)
    message(STATUS "Using CPU-only version of Kineto")
endif()

# Configure Kineto library
set(KINETO_BUILD_TESTS OFF CACHE BOOL "")
set(KINETO_LIBRARY_TYPE "static" CACHE STRING "")

# Tell Kineto to use Quarisma's fmt library instead of its embedded one
# This prevents duplicate symbol errors when linking
# Set FMT_SOURCE_DIR to a marker value that tells Kineto to skip fmt build
set(FMT_SOURCE_DIR "QUARISMA_EXTERNAL_FMT" CACHE STRING "Use external fmt library" FORCE)

message(STATUS "Configuring Kineto dependency:")
message(STATUS "  KINETO_BUILD_TESTS = ${KINETO_BUILD_TESTS}")
message(STATUS "  KINETO_LIBRARY_TYPE = ${KINETO_LIBRARY_TYPE}")
message(STATUS "  FMT_SOURCE_DIR = ${FMT_SOURCE_DIR} (QUARISMA_EXTERNAL_FMT means use external fmt)")

add_third_party_library(kineto
    CONDITION QUARISMA_ENABLE_KINETO
    LIBRARY_TYPE STATIC
    SOURCE_DIR kineto/libkineto
    DISABLE_WARNINGS
    FILTER_FLAGS "-flto=thin" "-flto=full"
    QUARISMA_TARGETS "Quarisma::kineto=kineto"
)

# Disable Kineto if the library was not found
if(QUARISMA_ENABLE_KINETO AND NOT TARGET Quarisma::kineto)
    message(STATUS "Kineto library not found - disabling QUARISMA_ENABLE_KINETO")
    set(QUARISMA_ENABLE_KINETO OFF CACHE BOOL "Enable PyTorch Kineto profiling library." FORCE)
endif()

# =============================================================================
# Google Test and Google Benchmark Integration
# =============================================================================

# Note: googletest and benchmark require special handling due to EXCLUDE_FROM_ALL
# and MSVC-specific settings, so they use manual add_subdirectory calls

# Google Test - Unit testing framework
if(QUARISMA_ENABLE_GTEST)
    # Check if gtest target already exists (avoid duplicate add_subdirectory)
    if(NOT TARGET gtest)
        message(STATUS "Adding Google Test from ThirdParty/googletest")

        # Set options before adding subdirectory
        # Force shared CRT on MSVC to match Quarisma's runtime library settings
        if(MSVC)
            set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        endif()

        # Add googletest subdirectory
        # This creates targets: gtest, gtest_main, gmock, gmock_main
        add_subdirectory(
            "${CMAKE_CURRENT_SOURCE_DIR}/googletest"
            "${CMAKE_CURRENT_BINARY_DIR}/googletest"
            EXCLUDE_FROM_ALL
        )

        # Set folder properties for all googletest targets
        _set_third_party_folder_properties("googletest" "${CMAKE_CURRENT_BINARY_DIR}/googletest")

        message(STATUS "Google Test targets created: gtest, gtest_main")
    else()
        message(STATUS "Google Test targets already exist, skipping add_subdirectory")
    endif()

    # Create Quarisma:: namespace aliases for googletest
    _create_third_party_interface_targets(
        "Quarisma::gtest=GTest::gtest|gtest"
        "Quarisma::gtest_main=GTest::gtest_main|gtest_main"
    )
endif()

# Google Benchmark - Microbenchmarking framework
if(QUARISMA_ENABLE_BENCHMARK)
    # Check if benchmark target already exists (avoid duplicate add_subdirectory)
    if(NOT TARGET benchmark)
        message(STATUS "Adding Google Benchmark from ThirdParty/benchmark")

        # Add benchmark subdirectory
        # This creates targets: benchmark, benchmark_main
        add_subdirectory(
            "${CMAKE_CURRENT_SOURCE_DIR}/benchmark"
            "${CMAKE_CURRENT_BINARY_DIR}/benchmark"
            EXCLUDE_FROM_ALL
        )

        # Set folder properties for all benchmark targets
        _set_third_party_folder_properties("benchmark" "${CMAKE_CURRENT_BINARY_DIR}/benchmark")

        message(STATUS "Google Benchmark targets created: benchmark, benchmark_main")
    else()
        message(STATUS "Google Benchmark targets already exist, skipping add_subdirectory")
    endif()

    # Create Quarisma:: namespace alias for benchmark
    _create_third_party_interface_targets(
        "Quarisma::benchmark=benchmark::benchmark|benchmark"
    )
endif()

# =============================================================================
# Quarisma Interface Targets
# =============================================================================
# All Quarisma:: namespace interface targets are now created automatically
# by the add_third_party_library() function using the QUARISMA_TARGETS parameter.
# This eliminates code duplication and provides a consistent interface for
# linking against third-party libraries throughout the Quarisma project.
#
# See the library declarations above for QUARISMA_TARGETS specifications.
# =============================================================================
