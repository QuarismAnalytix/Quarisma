/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Comprehensive test suite for nested parallelism functionality
 *
 * This file demonstrates safe and efficient multi-level parallel execution
 * using the XSigma parallel module. It covers:
 * - Two-level nested parallel_for operations
 * - Nested parallel_reduce operations
 * - Mixed nested operations (parallel_for with inner parallel_reduce)
 * - Use of parallel_guard to control nested parallelism depth
 * - Thread naming for debugging and profiling
 * - Edge cases: empty ranges, single-element ranges in nested loops
 *
 * THREAD SAFETY AND PERFORMANCE CONSIDERATIONS:
 * =============================================
 * 1. XSigma prevents nested parallelism by default using parallel_guard to
 *    avoid over-subscription (too many threads competing for CPU resources).
 *
 * 2. When in_parallel_region() returns true, inner parallel_for/reduce calls
 *    execute sequentially on the calling thread.
 *
 * 3. Thread naming via set_thread_name() is useful for debugging and profiling
 *    to identify which thread is executing which part of the nested structure.
 *
 * 4. The parallel_guard RAII pattern ensures proper state restoration even
 *    when early returns or exceptions occur (though XSigma avoids exceptions).
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <mutex>
#include <numeric>
#include <set>
#include <thread>
#include <vector>

#include "Testing/xsigmaTest.h"
#include "parallel/parallel.h"
#include "parallel/parallel_guard.h"
#include "parallel/thread_name.h"

namespace xsigma
{

// ============================================================================
// Test Group 1: Basic Nested parallel_for (2-level nesting)
// ============================================================================

// Test 1: Basic nested parallel_for with automatic nesting prevention
XSIGMATEST(NestedParallelism, Test)
{
    {
        // Matrix dimensions: outer loop processes rows, inner loop processes columns
        constexpr int rows  = 10;
        constexpr int cols  = 10;
        constexpr int total = rows * cols;

        std::vector<int> matrix(total, 0);

        // Outer parallel_for: processes rows
        parallel_for(
            0,
            rows,
            2,
            [&matrix, cols](int64_t row_begin, int64_t row_end)
            {
                for (int64_t row = row_begin; row < row_end; ++row)
                {
                    // Inner parallel_for: processes columns
                    // Note: This will execute SEQUENTIALLY because we're already
                    // inside a parallel region (in_parallel_region() returns true)
                    parallel_for(
                        0,
                        cols,
                        2,
                        [&matrix, row, cols](int64_t col_begin, int64_t col_end)
                        {
                            for (int64_t col = col_begin; col < col_end; ++col)
                            {
                                // Store row * cols + col pattern for verification
                                matrix[row * cols + col] = static_cast<int>(row * 10 + col);
                            }
                        });
                }
            });

        // Verify all elements were processed correctly
        for (int row = 0; row < rows; ++row)
        {
            for (int col = 0; col < cols; ++col)
            {
                EXPECT_EQ(matrix[row * cols + col], row * 10 + col)
                    << "Mismatch at row=" << row << ", col=" << col;
            }
        }
    }

    // Test 2: Nested parallelism with in_parallel_region check
    {
        std::atomic<int> outer_parallel_count{0};
        std::atomic<int> inner_sequential_count{0};

        // Track if inner loops detected they were in a parallel region
        std::atomic<int> inner_in_parallel_count{0};

        parallel_for(
            0,
            10,
            2,
            [&](int64_t begin, int64_t end)
            {
                outer_parallel_count++;

                for (int64_t i = begin; i < end; ++i)
                {
                    // Check if we're in a parallel region before inner loop
                    bool was_in_parallel = in_parallel_region();
                    if (was_in_parallel)
                    {
                        inner_in_parallel_count++;
                    }

                    // Inner parallel_for - should execute sequentially
                    parallel_for(
                        0,
                        5,
                        1,
                        [&inner_sequential_count](int64_t, int64_t) { inner_sequential_count++; });
                }
            });

        // Verify outer loop was parallelized (at least 1 chunk executed)
        EXPECT_GE(outer_parallel_count.load(), 1);

        // Verify inner loops detected they were in a parallel region
        // (should match number of outer iterations)
        EXPECT_EQ(inner_in_parallel_count.load(), 10);
    }

    // Test 3: Thread naming in nested parallelism for debugging
    {
        std::mutex            names_mutex;
        std::set<std::string> outer_thread_names;
        std::atomic<int>      inner_execution_count{0};

        parallel_for(
            0,
            8,
            2,
            [&](int64_t begin, int64_t end)
            {
                // Set thread name for outer loop workers
                const std::string outer_name = "outer_worker_" + std::to_string(begin);
                detail::parallel::set_thread_name(outer_name);

                // Collect outer thread names
                {
                    std::lock_guard<std::mutex> lock(names_mutex);
                    outer_thread_names.insert(detail::parallel::get_thread_name());
                }

                for (int64_t i = begin; i < end; ++i)
                {
                    // Inner loop - executes sequentially on same thread
                    parallel_for(
                        0,
                        4,
                        1,
                        [&inner_execution_count](int64_t, int64_t) { inner_execution_count++; });
                }
            });

        // Verify outer threads were named (at least one unique name)
        EXPECT_GE(outer_thread_names.size(), 1u);

        // Verify inner executions occurred (8 outer * 4 inner iterations)
        EXPECT_GE(inner_execution_count.load(), 1);
    }

    // ============================================================================
    // Test Group 2: Nested parallelism with parallel_guard control
    // ============================================================================

    // Test 4: Explicit parallel_guard to control nesting behavior
    // Note: parallel_guard::is_enabled() and in_parallel_region() may differ depending on backend:
    // - TBB backend: in_parallel_region() uses parallel_guard::is_enabled()
    // - Native backend: in_parallel_region() uses internal thread-local state set by parallel primitives
    // - OpenMP backend: in_parallel_region() uses omp_in_parallel()
    {
        std::atomic<int> count{0};

        // Without guard - default behavior
        EXPECT_FALSE(parallel_guard::is_enabled());

        {
            // Set guard to simulate being in a parallel region
            parallel_guard guard(true);
            EXPECT_TRUE(parallel_guard::is_enabled());

            // Note: in_parallel_region() may not reflect guard state in native backend
            // because native backend uses internal _set_in_parallel_region() mechanism

            // parallel_for behavior with guard set:
            // - If backend respects parallel_guard (TBB), should execute sequentially
            // - If backend uses internal state (native), may still parallelize
            parallel_for(0, 10, 2, [&count](int64_t, int64_t) { count++; });
        }

        // After guard destruction, state should be restored
        EXPECT_FALSE(parallel_guard::is_enabled());

        // Verify execution happened (either parallel or sequential)
        EXPECT_GE(count.load(), 1);
    }

    // Test 5: Nested guards restore state correctly
    {
        EXPECT_FALSE(parallel_guard::is_enabled());

        {
            parallel_guard outer_guard(true);
            EXPECT_TRUE(parallel_guard::is_enabled());

            {
                // Inner guard sets different state
                parallel_guard inner_guard(false);
                EXPECT_FALSE(parallel_guard::is_enabled());

                {
                    // Third level guard
                    parallel_guard third_guard(true);
                    EXPECT_TRUE(parallel_guard::is_enabled());
                }

                // After third_guard: should restore inner_guard's state
                EXPECT_FALSE(parallel_guard::is_enabled());
            }

            // After inner_guard: should restore outer_guard's state
            EXPECT_TRUE(parallel_guard::is_enabled());
        }

        // After outer_guard: should restore original state
        EXPECT_FALSE(parallel_guard::is_enabled());
    }

    // ============================================================================
    // Test Group 3: Nested parallel_reduce operations
    // ============================================================================

    // Test 6: Nested parallel_reduce (matrix sum)
    {
        // Matrix: 10x10, values are row*10 + col
        constexpr int rows = 10;
        constexpr int cols = 10;

        std::vector<int> matrix(rows * cols);
        for (int row = 0; row < rows; ++row)
        {
            for (int col = 0; col < cols; ++col)
            {
                matrix[row * cols + col] = row * 10 + col;
            }
        }

        // Outer reduce: sum of row sums
        int64_t total_sum = parallel_reduce(
            0,
            rows,
            2,
            static_cast<int64_t>(0),
            [&matrix, cols](int64_t row_begin, int64_t row_end, int64_t identity)
            {
                int64_t partial_sum = identity;
                for (int64_t row = row_begin; row < row_end; ++row)
                {
                    // Inner reduce: sum of column values for this row
                    // Note: Executes sequentially inside parallel region
                    int64_t row_sum = parallel_reduce(
                        0,
                        cols,
                        2,
                        static_cast<int64_t>(0),
                        [&matrix, row, cols](int64_t col_begin, int64_t col_end, int64_t inner_id)
                        {
                            int64_t col_sum = inner_id;
                            for (int64_t col = col_begin; col < col_end; ++col)
                            {
                                col_sum += matrix[row * cols + col];
                            }
                            return col_sum;
                        },
                        [](int64_t a, int64_t b) { return a + b; });

                    partial_sum += row_sum;
                }
                return partial_sum;
            },
            [](int64_t a, int64_t b) { return a + b; });

        // Expected sum: sum of all (row*10 + col) for row=0..9, col=0..9
        // = sum(row*10 for row=0..9) * 10 + sum(col for col=0..9) * 10
        // = 10 * 45 * 10 + 45 * 10 = 4500 + 450 = 4950
        int64_t expected_sum = 0;
        for (int row = 0; row < rows; ++row)
        {
            for (int col = 0; col < cols; ++col)
            {
                expected_sum += row * 10 + col;
            }
        }

        EXPECT_EQ(total_sum, expected_sum);
    }

    // Test 7: Nested parallel_reduce for finding matrix max
    {
        constexpr int rows = 8;
        constexpr int cols = 8;

        std::vector<int> matrix(rows * cols);
        for (int row = 0; row < rows; ++row)
        {
            for (int col = 0; col < cols; ++col)
            {
                // Create varied pattern: higher values near center
                int dist_from_center     = std::abs(row - rows / 2) + std::abs(col - cols / 2);
                matrix[row * cols + col] = 100 - dist_from_center * 10;
            }
        }

        // Find maximum value using nested reduce
        int max_val = parallel_reduce(
            0,
            rows,
            2,
            std::numeric_limits<int>::min(),
            [&matrix, cols](int64_t row_begin, int64_t row_end, int identity)
            {
                int row_max = identity;
                for (int64_t row = row_begin; row < row_end; ++row)
                {
                    // Inner reduce: find max in this row
                    int inner_max = parallel_reduce(
                        0,
                        cols,
                        2,
                        std::numeric_limits<int>::min(),
                        [&matrix, row, cols](int64_t col_begin, int64_t col_end, int inner_id)
                        {
                            int col_max = inner_id;
                            for (int64_t col = col_begin; col < col_end; ++col)
                            {
                                col_max = std::max(col_max, matrix[row * cols + col]);
                            }
                            return col_max;
                        },
                        [](int a, int b) { return std::max(a, b); });

                    row_max = std::max(row_max, inner_max);
                }
                return row_max;
            },
            [](int a, int b) { return std::max(a, b); });

        int expected_max = *std::max_element(matrix.begin(), matrix.end());
        EXPECT_EQ(max_val, expected_max);
    }

    // ============================================================================
    // Test Group 4: Mixed nested operations
    // ============================================================================

    // Test 8: Outer parallel_for with inner parallel_reduce
    {
        constexpr int rows = 10;
        constexpr int cols = 10;

        // Input matrix with known values
        std::vector<int> matrix(rows * cols);
        for (int row = 0; row < rows; ++row)
        {
            for (int col = 0; col < cols; ++col)
            {
                matrix[row * cols + col] = col + 1;  // Values 1 to 10 in each row
            }
        }

        // Output: row sums
        std::vector<int64_t> row_sums(rows, 0);

        // Outer parallel_for processes rows
        parallel_for(
            0,
            rows,
            2,
            [&matrix, &row_sums, cols](int64_t row_begin, int64_t row_end)
            {
                for (int64_t row = row_begin; row < row_end; ++row)
                {
                    // Inner parallel_reduce computes row sum
                    row_sums[row] = parallel_reduce(
                        0,
                        cols,
                        2,
                        static_cast<int64_t>(0),
                        [&matrix, row, cols](int64_t col_begin, int64_t col_end, int64_t identity)
                        {
                            int64_t partial = identity;
                            for (int64_t col = col_begin; col < col_end; ++col)
                            {
                                partial += matrix[row * cols + col];
                            }
                            return partial;
                        },
                        [](int64_t a, int64_t b) { return a + b; });
                }
            });

        // Each row should sum to 1+2+...+10 = 55
        constexpr int64_t expected_row_sum = (10 * 11) / 2;
        for (int row = 0; row < rows; ++row)
        {
            EXPECT_EQ(row_sums[row], expected_row_sum) << "Row " << row << " sum mismatch";
        }
    }

    // ============================================================================
    // Test Group 5: Edge cases in nested parallelism
    // ============================================================================

    // Test 9: Empty ranges in nested loops
    {
        std::atomic<int> outer_count{0};
        std::atomic<int> inner_count{0};

        // Outer loop with non-empty range
        parallel_for(
            0,
            5,
            1,
            [&](int64_t, int64_t)
            {
                outer_count++;

                // Inner loop with empty range (begin == end)
                parallel_for(0, 0, 1, [&inner_count](int64_t, int64_t) { inner_count++; });
            });

        // Outer should have executed
        EXPECT_GE(outer_count.load(), 1);

        // Inner should NOT have executed (empty range)
        EXPECT_EQ(inner_count.load(), 0);
    }

    // Test 10: Single-element ranges in nested loops
    {
        std::vector<int> result(1, 0);

        // Single-element outer loop
        parallel_for(
            0,
            1,
            1,
            [&result](int64_t, int64_t)
            {
                // Single-element inner loop
                parallel_for(0, 1, 1, [&result](int64_t, int64_t) { result[0] = 42; });
            });

        EXPECT_EQ(result[0], 42);
    }

    // Test 11: Single-element reduce in nested context
    {
        int result = parallel_reduce(
            0,
            1,
            1,
            0,
            [](int64_t begin, int64_t end, int identity)
            {
                // Inner single-element reduce
                return parallel_reduce(
                    0,
                    1,
                    1,
                    identity,
                    [](int64_t b, int64_t e, int id)
                    {
                        int sum = id;
                        for (int64_t i = b; i < e; ++i)
                        {
                            sum += 100;
                        }
                        return sum;
                    },
                    [](int a, int b) { return a + b; });
            },
            [](int a, int b) { return a + b; });

        EXPECT_EQ(result, 100);
    }

    // Test 12: Large nested iteration counts
    {
        constexpr int outer_size = 100;
        constexpr int inner_size = 100;

        std::atomic<int64_t> total_sum{0};

        parallel_for(
            0,
            outer_size,
            10,
            [&total_sum, inner_size](int64_t begin, int64_t end)
            {
                for (int64_t outer = begin; outer < end; ++outer)
                {
                    int64_t inner_sum = parallel_reduce(
                        0,
                        inner_size,
                        10,
                        static_cast<int64_t>(0),
                        [outer](int64_t col_begin, int64_t col_end, int64_t identity)
                        {
                            int64_t partial = identity;
                            for (int64_t col = col_begin; col < col_end; ++col)
                            {
                                partial += outer * inner_size + col;
                            }
                            return partial;
                        },
                        [](int64_t a, int64_t b) { return a + b; });

                    total_sum.fetch_add(inner_sum, std::memory_order_relaxed);
                }
            });

        // Expected: sum of (i) for i = 0 to (outer_size * inner_size - 1)
        // = (n-1)*n/2 where n = outer_size * inner_size = 10000
        int64_t n        = outer_size * inner_size;
        int64_t expected = (n - 1) * n / 2;
        EXPECT_EQ(total_sum.load(), expected);
    }
}
}  // namespace xsigma
