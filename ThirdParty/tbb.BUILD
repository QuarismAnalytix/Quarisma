# =============================================================================
# Intel TBB (Threading Building Blocks) BUILD Configuration
# =============================================================================
# High-performance task scheduling and memory allocation library
# Built from source (oneTBB v2021.10.0)
# =============================================================================

package(default_visibility = ["//visibility:public"])

# =============================================================================
# TBB Core Library
# =============================================================================
# Builds TBB from source instead of linking against system library

cc_library(
    name = "tbb",
    srcs = glob(
        [
            "src/tbb/*.cpp",
        ],
        exclude = [
            "src/tbb/tbbbind.cpp",  # Requires hwloc
        ],
    ),
    hdrs = glob([
        "include/tbb/**/*.h",
        "include/oneapi/**/*.h",
        "src/tbb/*.h",
    ], allow_empty = True),
    includes = [
        "include",
        "src",
    ],
    copts = [
        "-DTBB_USE_DEBUG=0",
        "-D__TBB_BUILD=1",
        "-D__TBB_DYNAMIC_LOAD_ENABLED=0",
        "-D__TBB_SOURCE_DIRECTLY_INCLUDED=1",
        "-Wno-attributes",
        "-Wno-error",
    ] + select({
        "@platforms//os:windows": [
            "/DUSE_WINTHREAD",
            "/D_WIN32_WINNT=0x0A00",
        ],
        "//conditions:default": [
            "-DUSE_PTHREAD",
            "-fPIC",
            "-mwaitpkg",  # Enable WAITPKG instructions (_tpause, _umonitor, _umwait)
            "-mrtm",      # Enable RTM (Restricted Transactional Memory)
        ],
    }),
    linkopts = select({
        "@platforms//os:windows": [],
        "@platforms//os:macos": ["-lpthread"],
        "//conditions:default": ["-lpthread", "-ldl"],
    }),
    defines = ["__TBB_NO_IMPLICIT_LINKAGE=1"],
)

# =============================================================================
# TBB Malloc Library
# =============================================================================

cc_library(
    name = "tbbmalloc",
    srcs = glob([
        "src/tbbmalloc/*.cpp",
    ], exclude = [
        "src/tbbmalloc/proxy.cpp",
        "src/tbbmalloc/tbbmalloc_proxy.cpp",
    ]),
    hdrs = glob([
        "include/tbb/tbbmalloc.h",
        "include/oneapi/tbb/tbbmalloc.h",
        "src/tbbmalloc/*.h",
        "src/tbbmalloc_proxy/*.h",
    ], allow_empty = True),
    includes = [
        "include",
        "src",
    ],
    copts = [
        "-DTBB_USE_DEBUG=0",
        "-D__TBBMALLOC_BUILD=1",
    ] + select({
        "@platforms//os:windows": [
            "/DUSE_WINTHREAD",
        ],
        "//conditions:default": [
            "-DUSE_PTHREAD",
            "-fPIC",
        ],
    }),
    linkopts = select({
        "@platforms//os:windows": [],
        "@platforms//os:macos": ["-lpthread"],
        "//conditions:default": ["-lpthread", "-ldl"],
    }),
    deps = [":tbb"],  # tbbmalloc needs TBB headers
)

# =============================================================================
# TBB Malloc Proxy Library
# =============================================================================

cc_library(
    name = "tbbmalloc_proxy",
    srcs = glob([
        "src/tbbmalloc/proxy.cpp",
        "src/tbbmalloc/tbbmalloc_proxy.cpp",
    ], allow_empty = True),
    hdrs = glob([
        "include/tbb/tbbmalloc_proxy.h",
        "include/oneapi/tbb/tbbmalloc_proxy.h",
    ], allow_empty = True),
    includes = [
        "include",
        "src",
    ],
    copts = [
        "-DTBB_USE_DEBUG=0",
        "-D__TBBMALLOC_BUILD=1",
    ] + select({
        "@platforms//os:windows": [
            "/DUSE_WINTHREAD",
        ],
        "//conditions:default": [
            "-DUSE_PTHREAD",
            "-fPIC",
        ],
    }),
    deps = [":tbbmalloc"],
)

