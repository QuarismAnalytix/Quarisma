# =============================================================================
# XSigma Bazel Helper Functions and Macros
# =============================================================================
# Common functions for compiler flags, defines, and link options
# Equivalent to various CMake modules in Cmake/tools and Cmake/flags
# =============================================================================

def xsigma_copts():
    """Returns common compiler options for XSigma targets."""
    return select({
        "@platforms//os:windows": [
            "/std:c++17",
            "/W4",
            "/EHsc",
        ],
        "//conditions:default": [
            "-std=c++17",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
        ],
    })

def xsigma_defines():
    """Returns common preprocessor defines for XSigma targets."""
    base_defines = []

    # Add feature-specific defines based on build configuration
    return base_defines + select({
        "//bazel:enable_cuda": ["XSIGMA_ENABLE_CUDA", "XSIGMA_HAS_CUDA=1"],
        "//conditions:default": ["XSIGMA_HAS_CUDA=0"],
    }) + select({
        "//bazel:enable_hip": ["XSIGMA_ENABLE_HIP"],
        "//conditions:default": [],
    }) + select({
        "//bazel:enable_tbb": ["XSIGMA_HAS_TBB"],
        "//conditions:default": [],
    }) + select({
        "//bazel:enable_mkl": ["XSIGMA_ENABLE_MKL"],
        "//conditions:default": [],
    }) + select({
        "//bazel:enable_mimalloc": ["XSIGMA_ENABLE_MIMALLOC"],
        "//conditions:default": [],
    }) + select({
        "//bazel:enable_magic_enum": ["XSIGMA_ENABLE_MAGICENUM"],
        "//conditions:default": [],
    }) + select({
        "//bazel:enable_kineto": ["XSIGMA_ENABLE_KINETO", "XSIGMA_HAS_KINETO"],
        "//conditions:default": ["XSIGMA_ENABLE_KINETO=1"],
    }) + select({
        "//bazel:enable_native_profiler": ["XSIGMA_ENABLE_NATIVE_PROFILER"],
        "//conditions:default": [],
    }) + select({
        "//bazel:enable_itt": ["XSIGMA_ENABLE_ITT"],
        "//conditions:default": [],
    }) + select({
        "//bazel:enable_openmp": ["XSIGMA_ENABLE_OPENMP"],
        "//conditions:default": [],
    }) + select({
        "//bazel:lu_pivoting": ["XSIGMA_LU_PIVOTING"],
        "//conditions:default": [],
    }) + select({
        "//bazel:sobol_1111": ["XSIGMA_SOBOL_1111"],
        "//conditions:default": [],
    }) + select({
        "//bazel:logging_glog": ["XSIGMA_USE_GLOG"],
        "//bazel:logging_loguru": ["XSIGMA_USE_LOGURU"],
        "//bazel:logging_native": ["XSIGMA_USE_NATIVE_LOGGING"],
        "//conditions:default": [],
    }) + select({
        "//bazel:gpu_alloc_sync": ["XSIGMA_GPU_ALLOC_SYNC"],
        "//bazel:gpu_alloc_async": ["XSIGMA_GPU_ALLOC_ASYNC"],
        "//bazel:gpu_alloc_pool_async": ["XSIGMA_GPU_ALLOC_POOL_ASYNC"],
        "//conditions:default": ["XSIGMA_GPU_ALLOC_POOL_ASYNC"],  # Default
    })

def xsigma_linkopts():
    """Returns common linker options for XSigma targets."""
    return select({
        "@platforms//os:windows": [],
        "@platforms//os:macos": [
            "-undefined",
            "dynamic_lookup",
        ],
        "//conditions:default": [
            "-lpthread",
            "-ldl",
        ],
    })

def xsigma_test_copts():
    """Returns compiler options for XSigma test targets."""
    return xsigma_copts()

def xsigma_test_linkopts():
    """Returns linker options for XSigma test targets."""
    return xsigma_linkopts()
