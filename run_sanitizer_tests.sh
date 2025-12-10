#!/bin/bash

# XSigma Comprehensive Sanitizer Test Suite
# Tests all sanitizer configurations across all parallel backends

set -e

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Results tracking
declare -A results
total_tests=0
passed_tests=0
failed_tests=0
skipped_tests=0

# Log file
LOG_FILE="sanitizer_test_results_$(date +%Y%m%d_%H%M%S).log"

log() {
    echo -e "$@" | tee -a "$LOG_FILE"
}

log_header() {
    log ""
    log "${BLUE}========================================${NC}"
    log "${BLUE}$1${NC}"
    log "${BLUE}========================================${NC}"
}

log_success() {
    log "${GREEN}✓ $1${NC}"
}

log_failure() {
    log "${RED}✗ $1${NC}"
}

log_skip() {
    log "${YELLOW}⊘ $1${NC}"
}

# Test configuration function
run_test_config() {
    local backend=$1
    local sanitizer=$2
    local build_dir="build_${backend}_${sanitizer}"
    local test_name="${backend}+${sanitizer}"

    total_tests=$((total_tests + 1))

    log_header "Testing: $test_name"

    # Check for incompatible combinations
    if [[ "$sanitizer" == "thread" && "$backend" == "openmp" ]]; then
        log_skip "Skipping $test_name (TSan+OpenMP known issues)"
        results["$test_name"]="SKIP"
        skipped_tests=$((skipped_tests + 1))
        return 0
    fi

    # Create build directory
    rm -rf "$build_dir"
    mkdir -p "$build_dir"
    cd "$build_dir"

    log "Configuring build for $test_name..."

    # Set backend flags
    local openmp_flag="OFF"
    local tbb_flag="OFF"

    if [[ "$backend" == "openmp" ]]; then
        openmp_flag="ON"
    elif [[ "$backend" == "tbb" ]]; then
        tbb_flag="ON"
    fi

    # Configure with CMake
    if ! cmake .. \
        -GNinja \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_C_COMPILER=clang \
        -DCMAKE_CXX_COMPILER=clang++ \
        -DXSIGMA_BUILD_TESTS=ON \
        -DXSIGMA_ENABLE_SANITIZER=ON \
        -DXSIGMA_SANITIZER_TYPE="$sanitizer" \
        -DXSIGMA_ENABLE_OPENMP="$openmp_flag" \
        -DXSIGMA_ENABLE_TBB="$tbb_flag" \
        -DXSIGMA_ENABLE_CUDA=OFF \
        -DXSIGMA_ENABLE_MIMALLOC=OFF \
        >> "$LOG_FILE" 2>&1; then
        log_failure "Configuration failed for $test_name"
        results["$test_name"]="FAIL (config)"
        failed_tests=$((failed_tests + 1))
        cd ..
        return 1
    fi

    log "Building $test_name..."

    # Build the CoreCxxTests target (contains all tests)
    if ! ninja CoreCxxTests >> "$LOG_FILE" 2>&1; then
        log_failure "Build failed for $test_name"
        results["$test_name"]="FAIL (build)"
        failed_tests=$((failed_tests + 1))
        cd ..
        return 1
    fi

    log "Running tests for $test_name..."

    # Set sanitizer options
    export ASAN_OPTIONS="detect_leaks=1:check_initialization_order=1:strict_init_order=1"
    export UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1"
    export TSAN_OPTIONS="history_size=7:detect_deadlocks=1:second_deadlock_stack=1"
    export MSAN_OPTIONS="poison_in_dtor=1"

    # Run tests
    local test_failed=0

    # Run CoreCxxTests with filters for key parallel tests
    # Using gtest filter to run specific tests that are critical for parallel correctness
    log "  Running parallel tests..."

    # Run all async parallel tests
    if ./Library/Core/Testing/Cxx/CoreCxxTests --gtest_filter="TestAsyncParallel*" >> "$LOG_FILE" 2>&1; then
        log_success "  Async parallel tests passed"
    else
        log_failure "  Async parallel tests failed"
        test_failed=1
    fi

    # Run TestParallelFor
    if ./Library/Core/Testing/Cxx/CoreCxxTests --gtest_filter="TestParallelFor*" >> "$LOG_FILE" 2>&1; then
        log_success "  TestParallelFor passed"
    else
        log_failure "  TestParallelFor failed"
        test_failed=1
    fi

    # Run TestParallelReduce
    if ./Library/Core/Testing/Cxx/CoreCxxTests --gtest_filter="TestParallelReduce*" >> "$LOG_FILE" 2>&1; then
        log_success "  TestParallelReduce passed"
    else
        log_failure "  TestParallelReduce failed"
        test_failed=1
    fi

    # Run TestNestedParallelism (critical for thread safety)
    if ./Library/Core/Testing/Cxx/CoreCxxTests --gtest_filter="TestNestedParallelism*" >> "$LOG_FILE" 2>&1; then
        log_success "  TestNestedParallelism passed"
    else
        log_failure "  TestNestedParallelism failed"
        test_failed=1
    fi

    cd ..

    if [[ $test_failed -eq 0 ]]; then
        log_success "All tests passed for $test_name"
        results["$test_name"]="PASS"
        passed_tests=$((passed_tests + 1))
        return 0
    else
        log_failure "Some tests failed for $test_name"
        results["$test_name"]="FAIL (tests)"
        failed_tests=$((failed_tests + 1))
        return 1
    fi
}

# Main execution
main() {
    log_header "XSigma Sanitizer Test Suite"
    log "Starting comprehensive sanitizer testing across all backends"
    log "Log file: $LOG_FILE"
    log "Timestamp: $(date)"

    # Define test matrix
    backends=("native" "openmp" "tbb")
    sanitizers=("address" "undefined" "thread")

    # Note: memory sanitizer requires special libc++ build, skip for now

    # Run all combinations
    for backend in "${backends[@]}"; do
        for sanitizer in "${sanitizers[@]}"; do
            run_test_config "$backend" "$sanitizer" || true
        done
    done

    # Print summary
    log_header "Test Summary"
    log "Total test configurations: $total_tests"
    log "${GREEN}Passed: $passed_tests${NC}"
    log "${RED}Failed: $failed_tests${NC}"
    log "${YELLOW}Skipped: $skipped_tests${NC}"
    log ""

    # Print results table
    log "Results by Configuration:"
    log ""
    printf "%-20s %-15s %-15s %-15s\n" "Backend" "AddressSan" "UBSan" "ThreadSan" | tee -a "$LOG_FILE"
    printf "%-20s %-15s %-15s %-15s\n" "--------------------" "---------------" "---------------" "---------------" | tee -a "$LOG_FILE"

    for backend in "${backends[@]}"; do
        printf "%-20s " "$backend" | tee -a "$LOG_FILE"
        for sanitizer in "address" "undefined" "thread"; do
            test_name="${backend}+${sanitizer}"
            result=${results["$test_name"]}

            case "$result" in
                "PASS")
                    printf "${GREEN}%-15s${NC} " "$result" | tee -a "$LOG_FILE"
                    ;;
                "SKIP")
                    printf "${YELLOW}%-15s${NC} " "$result" | tee -a "$LOG_FILE"
                    ;;
                *)
                    printf "${RED}%-15s${NC} " "$result" | tee -a "$LOG_FILE"
                    ;;
            esac
        done
        echo "" | tee -a "$LOG_FILE"
    done

    log ""
    log "Detailed logs available in: $LOG_FILE"

    # Exit with error if any tests failed
    if [[ $failed_tests -gt 0 ]]; then
        log_failure "Some tests failed!"
        return 1
    else
        log_success "All tests passed!"
        return 0
    fi
}

# Run main function
main
