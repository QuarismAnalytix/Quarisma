# XSigma CI Compiler Cache Configuration This initial-cache file standardises compiler launcher
# settings across runners while still allowing manual overrides via the CMAKE_COMPILER_LAUNCHER
# environment variable.

if(NOT "$ENV{CMAKE_COMPILER_LAUNCHER}" STREQUAL "")
  set(CMAKE_C_COMPILER_LAUNCHER "$ENV{CMAKE_COMPILER_LAUNCHER}" CACHE STRING "" FORCE)
  set(CMAKE_CXX_COMPILER_LAUNCHER "$ENV{CMAKE_COMPILER_LAUNCHER}" CACHE STRING "" FORCE)
  set(CMAKE_CUDA_COMPILER_LAUNCHER "$ENV{CMAKE_COMPILER_LAUNCHER}" CACHE STRING "" FORCE)
elseif("$ENV{RUNNER_OS}" STREQUAL "Windows")
  set(CMAKE_C_COMPILER_LAUNCHER "buildcache" CACHE STRING "" FORCE)
  set(CMAKE_CXX_COMPILER_LAUNCHER "buildcache" CACHE STRING "" FORCE)
  set(CMAKE_CUDA_COMPILER_LAUNCHER "buildcache" CACHE STRING "" FORCE)
  set(xsigma_replace_uncacheable_flags ON CACHE BOOL "" FORCE)
else()
  set(CMAKE_C_COMPILER_LAUNCHER "sccache" CACHE STRING "" FORCE)
  set(CMAKE_CXX_COMPILER_LAUNCHER "sccache" CACHE STRING "" FORCE)
  set(CMAKE_CUDA_COMPILER_LAUNCHER "sccache" CACHE STRING "" FORCE)
endif()

# Do not wrap HIP with sccache. Ubuntu's packaged sccache (0.7.7) does not
# reliably handle ROCm clang++/hipcc; CMake may also inherit the CXX launcher
# onto HIP because the HIP compiler is Clang. A failed HIP probe then kills
# the daemon, and the next CXX object fails with "Connection refused"
# (CMake GPU HIP job, run 34146182881). HIP compiles stay uncached.
set(CMAKE_HIP_COMPILER_LAUNCHER "" CACHE STRING "" FORCE)
