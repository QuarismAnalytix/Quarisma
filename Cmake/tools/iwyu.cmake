# ============================================================================= Quarisma
# Include-What-You-Use (IWYU) Configuration Module
# =============================================================================
# This module configures include-what-you-use for analyzing include dependencies. It detects
# unnecessary includes and suggests improvements to include structure.
# =============================================================================

# Include guard to prevent multiple inclusions
include_guard(GLOBAL)

# Include-What-You-Use Analysis Flag Controls whether IWYU include analysis is enabled during
# compilation. When enabled, analyzes include dependencies and suggests optimizations. Helps
# maintain clean and efficient include hierarchies.
option(QUARISMA_ENABLE_IWYU "Enable include-what-you-use (iwyu) support." OFF)
mark_as_advanced(QUARISMA_ENABLE_IWYU)

if(NOT QUARISMA_ENABLE_IWYU)
  return()
endif()

find_program(
  QUARISMA_IWYU_EXECUTABLE
  NAMES include-what-you-use iwyu
  PATHS "C:/Program Files (x86)/include-what-you-use/bin"
        "C:/Program Files/include-what-you-use/bin"
  PATH_SUFFIXES bin
)

if(NOT QUARISMA_IWYU_EXECUTABLE)
  message(
    FATAL_ERROR
      "IWYU requested but not found!

Please install include-what-you-use:

  - Ubuntu/Debian: sudo apt-get install iwyu
  - CentOS/RHEL/Fedora: sudo dnf install iwyu
  - macOS: brew install include-what-you-use
  - Windows: Download from https://include-what-you-use.org/
  - Manual: Build from https://github.com/include-what-you-use/include-what-you-use

Or set QUARISMA_ENABLE_IWYU=OFF to disable include analysis"
  )
else()
  message(STATUS "IWYU found: ${QUARISMA_IWYU_EXECUTABLE}")

  # Check if mapping file exists
  set(IWYU_MAPPING_FILE "${CMAKE_SOURCE_DIR}/Scripts/iwyu/iwyu_exclusion.imp")
  if(EXISTS "${IWYU_MAPPING_FILE}")
    message(STATUS "Using IWYU mapping file: ${IWYU_MAPPING_FILE}")
  else()
    message(WARNING "IWYU mapping file not found: ${IWYU_MAPPING_FILE}")
    set(IWYU_MAPPING_FILE "")
  endif()

  # Create IWYU log directory
  set(IWYU_LOG_DIR "${CMAKE_BINARY_DIR}/iwyu_logs")
  file(MAKE_DIRECTORY "${IWYU_LOG_DIR}")

  # Set IWYU log file path
  set(IWYU_LOG_FILE "${CMAKE_BINARY_DIR}/iwyu.log")

  # Prepare IWYU arguments with crash-resistant settings
  set(QUARISMA_IWYU_ARGS
      "-Xiwyu"
      "--cxx17ns"
      "-Xiwyu"
      "--max_line_length=120"
      "-Xiwyu"
      "--verbose=1"
      "-Xiwyu"
      "--comment_style=short"
      "-Xiwyu"
      "--error=0"
      "-Xiwyu"
      "--no_fwd_decls"
      "-Xiwyu"
      "--quoted_includes_first"
  )

  # Add system root for macOS to help IWYU find system headers
  if(APPLE)
    execute_process(
      COMMAND xcrun --show-sdk-path OUTPUT_VARIABLE MACOS_SDK_PATH OUTPUT_STRIP_TRAILING_WHITESPACE
                                                                   ERROR_QUIET
    )
    if(MACOS_SDK_PATH)
      list(APPEND QUARISMA_IWYU_ARGS "-isysroot" "${MACOS_SDK_PATH}")
      message(STATUS "IWYU using macOS SDK: ${MACOS_SDK_PATH}")
    endif()
  endif()

  if(IWYU_MAPPING_FILE)
    list(PREPEND QUARISMA_IWYU_ARGS "-Xiwyu" "--mapping_file=${IWYU_MAPPING_FILE}")
  endif()

  # Create configure detector script path
  set(CONFIGURE_DETECTOR_SCRIPT "${CMAKE_SOURCE_DIR}/Scripts/iwyu/iwyu_configure_detector.py")

  message(STATUS "IWYU will analyze include dependencies for Quarisma targets only")
  message(STATUS "IWYU analysis will be logged to: ${IWYU_LOG_FILE}")

  # Run configure header detection analysis
  if(EXISTS "${CONFIGURE_DETECTOR_SCRIPT}")
    message(STATUS "Running Quarisma configure header detection...")
    execute_process(
      COMMAND
        ${CMAKE_COMMAND} -E env python "${CONFIGURE_DETECTOR_SCRIPT}" "${CMAKE_SOURCE_DIR}/Library"
        --log-file "${CMAKE_BINARY_DIR}/configure_detection.log" --report-file
        "${CMAKE_BINARY_DIR}/configure_analysis_report.txt" --recursive
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      RESULT_VARIABLE CONFIGURE_DETECTION_RESULT
      OUTPUT_VARIABLE CONFIGURE_DETECTION_OUTPUT
      ERROR_VARIABLE CONFIGURE_DETECTION_ERROR
    )

    if(CONFIGURE_DETECTION_RESULT EQUAL 0)
      message(STATUS "Configure header detection completed successfully")
    else()
      message(
        WARNING
          "Configure header detection found issues - check ${CMAKE_BINARY_DIR}/configure_analysis_report.txt"
      )
    endif()

    message(STATUS "Configure analysis report: ${CMAKE_BINARY_DIR}/configure_analysis_report.txt")
  else()
    message(WARNING "Configure detector script not found: ${CONFIGURE_DETECTOR_SCRIPT}")
  endif()
endif()

# Function to create IWYU wrapper script for enhanced logging
function(quarisma_create_iwyu_wrapper target_name)
  set(WRAPPER_SCRIPT "${CMAKE_BINARY_DIR}/iwyu_wrapper_${target_name}.cmake")

  # Create wrapper script that logs IWYU output
  file(
    WRITE "${WRAPPER_SCRIPT}"
    "
# IWYU Wrapper Script for ${target_name}
# This script captures IWYU output and appends it to the main log file

set(IWYU_EXECUTABLE \"${QUARISMA_IWYU_EXECUTABLE}\")
set(IWYU_LOG_FILE \"${IWYU_LOG_FILE}\")
set(TARGET_NAME \"${target_name}\")

# Get the source file being analyzed
set(SOURCE_FILE \"\${CMAKE_ARGV3}\")

# Log header for this analysis
file(APPEND \"\${IWYU_LOG_FILE}\" \"\\n=== IWYU Analysis for \${TARGET_NAME}: \${SOURCE_FILE} ===\\n\")
file(APPEND \"\${IWYU_LOG_FILE}\" \"Timestamp: \")
string(TIMESTAMP CURRENT_TIME \"%Y-%m-%d %H:%M:%S\")
file(APPEND \"\${IWYU_LOG_FILE}\" \"\${CURRENT_TIME}\\n\")

# Execute IWYU and capture output
execute_process(
    COMMAND \"\${IWYU_EXECUTABLE}\" \${CMAKE_ARGV3} \${CMAKE_ARGV4} \${CMAKE_ARGV5} \${CMAKE_ARGV6} \${CMAKE_ARGV7} \${CMAKE_ARGV8} \${CMAKE_ARGV9}
    OUTPUT_VARIABLE IWYU_OUTPUT
    ERROR_VARIABLE IWYU_ERROR
    RESULT_VARIABLE IWYU_RESULT
)

# Log the output
if(IWYU_OUTPUT)
    file(APPEND \"\${IWYU_LOG_FILE}\" \"IWYU Output:\\n\${IWYU_OUTPUT}\\n\")
endif()

if(IWYU_ERROR)
    file(APPEND \"\${IWYU_LOG_FILE}\" \"IWYU Errors/Warnings:\\n\${IWYU_ERROR}\\n\")
endif()

file(APPEND \"\${IWYU_LOG_FILE}\" \"IWYU Exit Code: \${IWYU_RESULT}\\n\")
file(APPEND \"\${IWYU_LOG_FILE}\" \"=== End Analysis ===\\n\\n\")

# Return the same exit code as IWYU
if(IWYU_RESULT)
    message(FATAL_ERROR \"IWYU failed with exit code \${IWYU_RESULT}\")
endif()
"
  )

  return()
endfunction()

# Function to apply IWYU to a target
function(quarisma_apply_iwyu target_name)
  if(NOT QUARISMA_ENABLE_IWYU OR NOT QUARISMA_IWYU_EXECUTABLE)
    return()
  endif()

  # Skip third-party targets
  get_target_property(target_source_dir ${target_name} SOURCE_DIR)
  if(target_source_dir MATCHES ".*/ThirdParty/.*" OR target_source_dir MATCHES ".*/third_party/.*"
     OR target_source_dir MATCHES ".*/3rdparty/.*"
  )
    message(STATUS "Skipping IWYU for third-party target: ${target_name}")
    return()
  endif()

  # Initialize the log file with basic header information
  if(NOT EXISTS "${IWYU_LOG_FILE}")
    file(WRITE "${IWYU_LOG_FILE}" "# IWYU Analysis Log for Quarisma Project\n")
    file(APPEND "${IWYU_LOG_FILE}" "# Generated by CMake IWYU integration\n")
    file(APPEND "${IWYU_LOG_FILE}" "# IWYU executable: ${QUARISMA_IWYU_EXECUTABLE}\n")
    file(APPEND "${IWYU_LOG_FILE}" "# Mapping file: ${IWYU_MAPPING_FILE}\n")
    file(APPEND "${IWYU_LOG_FILE}" "# Use Scripts/run_iwyu_analysis.py for detailed analysis\n")
    file(APPEND "${IWYU_LOG_FILE}" "\n")
  endif()

  # Apply IWYU to the target with crash-resistant settings
  set_target_properties(
    ${target_name}
    PROPERTIES
      CXX_INCLUDE_WHAT_YOU_USE
      "${QUARISMA_IWYU_EXECUTABLE};${QUARISMA_IWYU_ARGS};-D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH;-D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING"
      C_INCLUDE_WHAT_YOU_USE
      "${QUARISMA_IWYU_EXECUTABLE};${QUARISMA_IWYU_ARGS};-D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH"
  )

  message(STATUS "Applied IWYU to target: ${target_name}")
endfunction()
