#!/usr/bin/env bash
# Comprehensive XSigma Configuration Testing Script
# Tests all major CMake configuration flags systematically

set -e  # Exit on error

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Results tracking
PASSED_CONFIGS=()
FAILED_CONFIGS=()
SKIPPED_CONFIGS=()

# Log file
LOG_FILE="/tmp/xsigma_config_test_$(date +%Y%m%d_%H%M%S).log"
RESULTS_FILE="/tmp/xsigma_config_results_$(date +%Y%m%d_%H%M%S).txt"

echo "XSigma Configuration Testing" | tee -a "$LOG_FILE"
echo "=============================" | tee -a "$LOG_FILE"
echo "Log file: $LOG_FILE" | tee -a "$LOG_FILE"
echo "Results file: $RESULTS_FILE" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# Function to print colored output
print_status() {
    local color=$1
    local message=$2
    echo -e "${color}${message}${NC}" | tee -a "$LOG_FILE"
}

# Function to test a configuration
test_configuration() {
    local config_name=$1
    local config_args=$2
    local skip_reason=$3
    
    print_status "$BLUE" "\n=========================================="
    print_status "$BLUE" "Testing: $config_name"
    print_status "$BLUE" "Args: $config_args"
    print_status "$BLUE" "=========================================="
    
    if [ -n "$skip_reason" ]; then
        print_status "$YELLOW" "SKIPPED: $skip_reason"
        SKIPPED_CONFIGS+=("$config_name")
        echo "$config_name|SKIPPED|$skip_reason|0|0" >> "$RESULTS_FILE"
        return 0
    fi
    
    local start_time=$(date +%s)
    
    # Run the build
    if python3 setup.py $config_args >> "$LOG_FILE" 2>&1; then
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        
        # Extract build and test times from log
        local build_time=$(tail -100 "$LOG_FILE" | grep "Build time:" | tail -1 | awk '{print $3}')
        local test_time=$(tail -100 "$LOG_FILE" | grep "Test time:" | tail -1 | awk '{print $3}')

        print_status "$GREEN" "✅ PASSED (${duration}s, build: ${build_time}s, test: ${test_time}s)"
        PASSED_CONFIGS+=("$config_name:$build_time:$test_time")
        echo "$config_name|PASSED||$build_time|$test_time" >> "$RESULTS_FILE"
    else
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        
        print_status "$RED" "❌ FAILED (${duration}s)"
        FAILED_CONFIGS+=("$config_name")
        echo "$config_name|FAILED|Build or test failed|0|0" >> "$RESULTS_FILE"
        
        # Extract error from log
        print_status "$RED" "Last 20 lines of error:"
        tail -20 "$LOG_FILE"
    fi
}

# Change to Scripts directory
cd "$(dirname "$0")"

# Initialize results file
echo "Configuration|Status|Notes|BuildTime|TestTime" > "$RESULTS_FILE"

print_status "$BLUE" "Starting systematic configuration testing..."
echo ""

# ============================================================================
# 1. BASELINE CONFIGURATION (already tested, but include for completeness)
# ============================================================================
print_status "$BLUE" "=== BASELINE TESTS ==="
test_configuration "baseline" "config.build.test.ninja.clang" ""

# ============================================================================
# 2. INDIVIDUAL FEATURE FLAGS - ENABLE
# ============================================================================
print_status "$BLUE" "\n=== INDIVIDUAL FEATURE FLAGS (ENABLE) ==="

# TBB (Threading Building Blocks)
test_configuration "tbb_enabled" "config.build.test.ninja.clang.tbb" ""

# MKL (Math Kernel Library) - requires MKL to be installed
test_configuration "mkl_enabled" "config.build.test.ninja.clang.mkl" "MKL not installed on macOS ARM64"

# NUMA - not supported on macOS
test_configuration "numa_enabled" "config.build.test.ninja.clang.numa" "NUMA not supported on macOS"

# MEMKIND - not supported on macOS
test_configuration "memkind_enabled" "config.build.test.ninja.clang.memkind" "MEMKIND not supported on macOS"

# Benchmark
test_configuration "benchmark_enabled" "config.build.test.ninja.clang.benchmark" ""

# ============================================================================
# 3. INDIVIDUAL FEATURE FLAGS - DISABLE (for flags that default to ON)
# ============================================================================
print_status "$BLUE" "\n=== INDIVIDUAL FEATURE FLAGS (DISABLE) ==="

# Disable LTO (defaults to ON)
test_configuration "lto_disabled" "config.build.test.ninja.clang.lto" ""

# Disable Loguru (defaults to ON)
test_configuration "loguru_disabled" "config.build.test.ninja.clang.loguru" ""

# Disable Mimalloc (defaults to ON)
test_configuration "mimalloc_disabled" "config.build.test.ninja.clang.mimalloc" ""

# ============================================================================
# 4. VECTORIZATION OPTIONS
# ============================================================================
print_status "$BLUE" "\n=== VECTORIZATION OPTIONS ==="

test_configuration "vectorization_none" "config.build.test.ninja.clang --vectorization=no" ""
test_configuration "vectorization_sse" "config.build.test.ninja.clang --vectorization=sse" ""
test_configuration "vectorization_avx" "config.build.test.ninja.clang --vectorization=avx" ""
test_configuration "vectorization_avx2" "config.build.test.ninja.clang --vectorization=avx2" ""
test_configuration "vectorization_avx512" "config.build.test.ninja.clang --vectorization=avx512" "AVX512 not available on Apple Silicon"

# ============================================================================
# 5. STATIC VS SHARED LIBRARIES
# ============================================================================
print_status "$BLUE" "\n=== LIBRARY TYPE ==="

# Static libraries (static flag inverts BUILD_SHARED_LIBS)
test_configuration "static_libs" "config.build.test.ninja.clang.static" ""

# ============================================================================
# 6. BUILD TYPE VARIATIONS
# ============================================================================
print_status "$BLUE" "\n=== BUILD TYPES ==="

test_configuration "debug_build" "config.build.test.ninja.clang.debug" ""
test_configuration "release_build" "config.build.test.ninja.clang.release" ""
test_configuration "relwithdebinfo_build" "config.build.test.ninja.clang.relwithdebinfo" ""

# ============================================================================
# 7. CRITICAL FLAG COMBINATIONS
# ============================================================================
print_status "$BLUE" "\n=== CRITICAL FLAG COMBINATIONS ==="

# Minimal build (all optional features disabled)
test_configuration "minimal_build" "config.build.test.ninja.clang.lto.loguru.mimalloc" ""

# Production build (Release + LTO + Mimalloc)
test_configuration "production_build" "config.build.test.ninja.clang.release" ""

# Performance build (Release + LTO + AVX2 + Mimalloc)
test_configuration "performance_build" "config.build.test.ninja.clang.release --vectorization=avx2" ""

# TBB + Benchmark
test_configuration "tbb_benchmark" "config.build.test.ninja.clang.tbb.benchmark" ""

# Debug + No LTO
test_configuration "debug_no_lto" "config.build.test.ninja.clang.debug.lto" ""

# Static + Release + LTO
test_configuration "static_release_lto" "config.build.test.ninja.clang.static.release" ""

# ============================================================================
# SUMMARY
# ============================================================================
print_status "$BLUE" "\n=========================================="
print_status "$BLUE" "TESTING COMPLETE"
print_status "$BLUE" "=========================================="

echo "" | tee -a "$LOG_FILE"
print_status "$GREEN" "Passed: ${#PASSED_CONFIGS[@]}"
print_status "$RED" "Failed: ${#FAILED_CONFIGS[@]}"
print_status "$YELLOW" "Skipped: ${#SKIPPED_CONFIGS[@]}"
echo "" | tee -a "$LOG_FILE"

if [ ${#PASSED_CONFIGS[@]} -gt 0 ]; then
    print_status "$GREEN" "Passed configurations:"
    for config_data in "${PASSED_CONFIGS[@]}"; do
        IFS=':' read -r config build_time test_time <<< "$config_data"
        echo "  ✅ $config (build: ${build_time}s, test: ${test_time}s)" | tee -a "$LOG_FILE"
    done
    echo "" | tee -a "$LOG_FILE"
fi

if [ ${#FAILED_CONFIGS[@]} -gt 0 ]; then
    print_status "$RED" "Failed configurations:"
    for config in "${FAILED_CONFIGS[@]}"; do
        echo "  ❌ $config" | tee -a "$LOG_FILE"
    done
    echo "" | tee -a "$LOG_FILE"
fi

if [ ${#SKIPPED_CONFIGS[@]} -gt 0 ]; then
    print_status "$YELLOW" "Skipped configurations:"
    for config in "${SKIPPED_CONFIGS[@]}"; do
        echo "  ⏭️  $config" | tee -a "$LOG_FILE"
    done
    echo "" | tee -a "$LOG_FILE"
fi

print_status "$BLUE" "Detailed results saved to: $RESULTS_FILE"
print_status "$BLUE" "Full log saved to: $LOG_FILE"

# Exit with error if any tests failed
if [ ${#FAILED_CONFIGS[@]} -gt 0 ]; then
    exit 1
fi

exit 0

