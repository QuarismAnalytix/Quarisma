# * Try to find ITT
#
# The following are set after configuration is done: ITT_FOUND          : set to true if ITT is
# found. ITT_INCLUDE_DIR    : path to ITT include dir. ITT_LIBRARIES      : list of libraries for
# ITT

function(Quarisma_create_itt_interface_targets)
  if(TARGET ittnotify AND NOT TARGET "itt::itt")
    add_library(Quarisma::itt INTERFACE IMPORTED)
    target_include_directories(Quarisma::itt INTERFACE ${ITT_INCLUDE_DIR})
    target_link_libraries(Quarisma::itt INTERFACE ittnotify)
  endif()

  if(TARGET jitprofiling AND NOT TARGET Quarisma::jitprofiling)
    add_library(Quarisma::jitprofiling INTERFACE IMPORTED)
    target_include_directories(Quarisma::jitprofiling INTERFACE ${ITT_INCLUDE_DIR})
    target_link_libraries(Quarisma::jitprofiling INTERFACE jitprofiling)
  endif()
endfunction()

if(NOT ITT_FOUND)
  set(ITT_FOUND OFF)

  set(ITT_INCLUDE_DIR)
  set(ITT_LIBRARIES)

  set(ITT_ROOT "${PROJECT_SOURCE_DIR}/ThirdParty/ittapi")
  find_path(ITT_INCLUDE_DIR ittnotify.h PATHS ${ITT_ROOT} PATH_SUFFIXES include)
  if(ITT_INCLUDE_DIR)
    add_subdirectory(${ITT_ROOT})
    set(ITT_LIBRARIES ittnotify)
    set(ITT_FOUND ON)

    # Set FOLDER property for all ITT API targets
    # The ITT API library creates multiple targets: ittnotify, jitprofiling, and optionally advisor
    if(TARGET ittnotify)
      set_target_properties(ittnotify PROPERTIES FOLDER "ThirdParty/ittapi")
    endif()

    if(TARGET jitprofiling)
      set_target_properties(jitprofiling PROPERTIES FOLDER "ThirdParty/ittapi")
    endif()

    if(TARGET advisor)
      set_target_properties(advisor PROPERTIES FOLDER "ThirdParty/ittapi")
    endif()

    Quarisma_create_itt_interface_targets()
  endif(ITT_INCLUDE_DIR)
endif(NOT ITT_FOUND)
