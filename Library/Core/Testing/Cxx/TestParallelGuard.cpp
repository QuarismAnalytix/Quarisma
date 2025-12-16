/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Comprehensive test suite for parallel_guard functionality
 * Tests RAII-based parallel region state tracking
 */

#include "Testing/xsigmaTest.h"
#include "parallel/parallel.h"
#include "parallel/parallel_guard.h"

namespace xsigma
{

// ============================================================================
// Test Group 1: Basic Functionality
// ============================================================================

// Test 1: Guard sets and restores parallel state
XSIGMATEST(ParallelGuard, Test)
{
    {
        EXPECT_FALSE(parallel_guard::is_enabled());

        {
            parallel_guard guard(true);
            EXPECT_TRUE(parallel_guard::is_enabled());
        }

        EXPECT_FALSE(parallel_guard::is_enabled());
    }

    // Test 2: Guard with false state
    {
        EXPECT_FALSE(parallel_guard::is_enabled());

        {
            parallel_guard guard(false);
            EXPECT_FALSE(parallel_guard::is_enabled());
        }

        EXPECT_FALSE(parallel_guard::is_enabled());
    }

    // Test 3: Guard preserves previous state
    {
        {
            parallel_guard guard1(true);
            EXPECT_TRUE(parallel_guard::is_enabled());

            {
                parallel_guard guard2(false);
                EXPECT_FALSE(parallel_guard::is_enabled());
            }

            EXPECT_TRUE(parallel_guard::is_enabled());
        }

        EXPECT_FALSE(parallel_guard::is_enabled());
    }

    // ============================================================================
    // Test Group 2: Nested Guards
    // ============================================================================

    // Test 4: Nested guards with same state
    {
        EXPECT_FALSE(parallel_guard::is_enabled());

        {
            parallel_guard guard1(true);
            EXPECT_TRUE(parallel_guard::is_enabled());

            {
                parallel_guard guard2(true);
                EXPECT_TRUE(parallel_guard::is_enabled());
            }

            EXPECT_TRUE(parallel_guard::is_enabled());
        }

        EXPECT_FALSE(parallel_guard::is_enabled());
    }

    // Test 5: Multiple levels of nesting
    {
        EXPECT_FALSE(parallel_guard::is_enabled());

        {
            parallel_guard guard1(true);
            EXPECT_TRUE(parallel_guard::is_enabled());

            {
                parallel_guard guard2(false);
                EXPECT_FALSE(parallel_guard::is_enabled());

                {
                    parallel_guard guard3(true);
                    EXPECT_TRUE(parallel_guard::is_enabled());

                    {
                        parallel_guard guard4(false);
                        EXPECT_FALSE(parallel_guard::is_enabled());
                    }

                    EXPECT_TRUE(parallel_guard::is_enabled());
                }

                EXPECT_FALSE(parallel_guard::is_enabled());
            }

            EXPECT_TRUE(parallel_guard::is_enabled());
        }

        EXPECT_FALSE(parallel_guard::is_enabled());
    }

    // ============================================================================
    // Test Group 3: Integration with in_parallel_region
    // ============================================================================

    // Test 6: Guard affects in_parallel_region
    {
        EXPECT_FALSE(in_parallel_region());

        {
            parallel_guard guard(true);
            // Note: in_parallel_region() uses parallel_guard::is_enabled()
            EXPECT_TRUE(parallel_guard::is_enabled());
        }

        EXPECT_FALSE(in_parallel_region());
    }

    // Test 7: Guard state independent of parallel operations
    {
        std::vector<int> data(100, 0);

        // Manually set guard state
        {
            parallel_guard guard(true);
            EXPECT_TRUE(parallel_guard::is_enabled());

            // Run parallel_for (which will set its own guard)
            parallel_for(
                0,
                100,
                10,
                [&data](int64_t begin, int64_t end)
                {
                    for (int64_t i = begin; i < end; ++i)
                    {
                        data[i] = static_cast<int>(i);
                    }
                });

            // After parallel_for, our guard should still be active
            EXPECT_TRUE(parallel_guard::is_enabled());
        }

        // After guard destruction, should be false
        EXPECT_FALSE(parallel_guard::is_enabled());

        for (int i = 0; i < 100; ++i)
        {
            EXPECT_EQ(data[i], i);
        }
    }

    // ============================================================================
    // Test Group 4: Exception Safety
    // ============================================================================

    // Test 8: Guard restores on exception
    {
        EXPECT_FALSE(parallel_guard::is_enabled());

        try
        {
            parallel_guard guard(true);
            EXPECT_TRUE(parallel_guard::is_enabled());
            throw std::runtime_error("Test exception");
        }
        catch (const std::exception&)
        {
            // Exception caught
        }

        EXPECT_FALSE(parallel_guard::is_enabled());
    }

    // Test 9: Nested guards with exception
    {
        EXPECT_FALSE(parallel_guard::is_enabled());

        try
        {
            parallel_guard guard1(true);
            EXPECT_TRUE(parallel_guard::is_enabled());

            try
            {
                parallel_guard guard2(false);
                EXPECT_FALSE(parallel_guard::is_enabled());
                throw std::runtime_error("Inner exception");
            }
            catch (const std::exception&)
            {
                // Inner exception caught
            }

            EXPECT_TRUE(parallel_guard::is_enabled());
            throw std::runtime_error("Outer exception");
        }
        catch (const std::exception&)
        {
            // Outer exception caught
        }

        EXPECT_FALSE(parallel_guard::is_enabled());
    }

    // ============================================================================
    // Test Group 5: Edge Cases
    // ============================================================================

    // Test 10: Guard with same state as current
    {
        EXPECT_FALSE(parallel_guard::is_enabled());

        {
            parallel_guard guard(false);
            EXPECT_FALSE(parallel_guard::is_enabled());
        }

        EXPECT_FALSE(parallel_guard::is_enabled());
    }

    // Test 11: Multiple sequential guards
    {
        EXPECT_FALSE(parallel_guard::is_enabled());

        {
            parallel_guard guard1(true);
            EXPECT_TRUE(parallel_guard::is_enabled());
        }

        EXPECT_FALSE(parallel_guard::is_enabled());

        {
            parallel_guard guard2(true);
            EXPECT_TRUE(parallel_guard::is_enabled());
        }

        EXPECT_FALSE(parallel_guard::is_enabled());

        {
            parallel_guard guard3(false);
            EXPECT_FALSE(parallel_guard::is_enabled());
        }

        EXPECT_FALSE(parallel_guard::is_enabled());
    }

    // Test 12: Guard in function scope
    {
        auto test_function = [](bool state)
        {
            parallel_guard guard(state);
            return parallel_guard::is_enabled();
        };

        EXPECT_TRUE(test_function(true));
        EXPECT_FALSE(parallel_guard::is_enabled());

        EXPECT_FALSE(test_function(false));
        EXPECT_FALSE(parallel_guard::is_enabled());
    }
}
}  // namespace xsigma
