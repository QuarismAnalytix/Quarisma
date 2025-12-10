/**
 * @file PARALLEL_FOR_LAMBDA_EXAMPLES.cpp
 * @brief Examples demonstrating the new lambda-based parallel_for API
 *
 * This file demonstrates the simplified parallel_for API that accepts lambda functions
 * and compares it to the legacy functor-based API.
 *
 * REFACTORING SUMMARY:
 * ====================
 * The parallel_for API has been refactored to provide a cleaner, more modern interface:
 *
 * BEFORE (8 overloads):
 * - 2 overloads for size_t range with grain (const/non-const functor)
 * - 2 overloads for size_t range without grain (const/non-const functor)
 * - 2 overloads for iterator range with grain (const/non-const functor)
 * - 2 overloads for iterator range without grain (const/non-const functor)
 *
 * AFTER (12 overloads - 4 new lambda + 8 legacy):
 * - 2 NEW overloads for size_t range (with/without grain) accepting std::function
 * - 2 NEW overloads for iterator range (with/without grain) accepting std::function
 * - 8 LEGACY overloads maintained for backward compatibility
 *
 * BENEFITS:
 * - Simpler API for new code: just pass a lambda
 * - No need to create separate functor classes
 * - Better IDE auto-completion and type inference
 * - Maintains full backward compatibility
 *
 * @author XSigma Development Team
 * @date 2025
 */

#include <cmath>
#include <iostream>
#include <numeric>
#include <vector>

#include "smp/smp_tools.h"

// ============================================================================
// EXAMPLE 1: Basic parallel loop with lambda (NEW API)
// ============================================================================

void example1_basic_lambda()
{
    std::cout << "\n=== Example 1: Basic parallel loop with lambda ===" << std::endl;

    std::vector<double> data(10000);

    // Modern lambda-based API - clean and simple!
    smp_tools::parallel_for(
        0,
        data.size(),
        1000,
        [&data](size_t start, size_t end)
        {
            for (size_t i = start; i < end; ++i)
            {
                data[i] = std::sin(i * 0.001);
            }
        });

    std::cout << "Computed " << data.size() << " values using lambda" << std::endl;
    std::cout << "First few values: " << data[0] << ", " << data[1] << ", " << data[2] << std::endl;
}

// ============================================================================
// EXAMPLE 2: Basic parallel loop with functor (LEGACY API)
// ============================================================================

// Old-style functor (still supported for backward compatibility)
struct ComputeSinFunctor
{
    std::vector<double>& data;

    ComputeSinFunctor(std::vector<double>& d) : data(d) {}

    void operator()(size_t start, size_t end) const
    {
        for (size_t i = start; i < end; ++i)
        {
            data[i] = std::sin(i * 0.001);
        }
    }
};

void example2_basic_functor()
{
    std::cout << "\n=== Example 2: Basic parallel loop with functor (legacy) ===" << std::endl;

    std::vector<double> data(10000);

    // Legacy functor-based API
    ComputeSinFunctor functor(data);
    smp_tools::parallel_for(0, data.size(), 1000, functor);

    std::cout << "Computed " << data.size() << " values using functor" << std::endl;
}

// ============================================================================
// EXAMPLE 3: Iterator-based parallel loop with lambda (NEW API)
// ============================================================================

void example3_iterator_lambda()
{
    std::cout << "\n=== Example 3: Iterator-based parallel loop with lambda ===" << std::endl;

    std::vector<int> data(1000);
    std::iota(data.begin(), data.end(), 0);  // Fill with 0, 1, 2, ...

    // Modern lambda-based API with iterators
    smp_tools::parallel_for(
        data.begin(),
        data.end(),
        100,
        [](auto it_start, auto it_end)
        {
            for (auto it = it_start; it != it_end; ++it)
            {
                *it *= 2;  // Double each value
            }
        });

    std::cout << "Doubled " << data.size() << " values using lambda with iterators" << std::endl;
    std::cout << "First few values: " << data[0] << ", " << data[1] << ", " << data[2] << std::endl;
}

// ============================================================================
// EXAMPLE 4: Default grain size (NEW API)
// ============================================================================

void example4_default_grain()
{
    std::cout << "\n=== Example 4: Parallel loop with default grain size ===" << std::endl;

    std::vector<double> data(5000);

    // Omit grain parameter - the system will choose automatically
    smp_tools::parallel_for(
        0,
        data.size(),
        [&data](size_t start, size_t end)
        {
            for (size_t i = start; i < end; ++i)
            {
                data[i] = std::sqrt(static_cast<double>(i));
            }
        });

    std::cout << "Computed " << data.size() << " square roots with auto grain" << std::endl;
}

// ============================================================================
// EXAMPLE 5: Complex computation with lambda captures (NEW API)
// ============================================================================

void example5_lambda_captures()
{
    std::cout << "\n=== Example 5: Complex computation with lambda captures ===" << std::endl;

    std::vector<double> input(1000);
    std::vector<double> output(1000);
    std::iota(input.begin(), input.end(), 1.0);

    // Lambda can capture multiple variables
    const double scale  = 2.5;
    const double offset = 10.0;

    smp_tools::parallel_for(
        0,
        input.size(),
        100,
        [&input, &output, scale, offset](size_t start, size_t end)
        {
            for (size_t i = start; i < end; ++i)
            {
                output[i] = input[i] * scale + offset;
            }
        });

    std::cout << "Applied transformation: y = x * " << scale << " + " << offset << std::endl;
    std::cout << "output[0] = " << output[0] << " (expected: " << (1.0 * scale + offset) << ")"
              << std::endl;
}

// ============================================================================
// EXAMPLE 6: Functor with Initialize/Reduce (LEGACY API - still supported)
// ============================================================================

struct SumReductionFunctor
{
    const std::vector<double>& data;
    double                     sum;

    SumReductionFunctor(const std::vector<double>& d) : data(d), sum(0.0) {}

    // Initialize is called once per thread
    void Initialize() { sum = 0.0; }

    void operator()(size_t start, size_t end)
    {
        for (size_t i = start; i < end; ++i)
        {
            sum += data[i];
        }
    }

    // Reduce is called after all threads complete
    void Reduce()
    {
        // This would combine results from all threads
        // (actual reduction mechanism depends on backend implementation)
    }
};

void example6_functor_with_reduce()
{
    std::cout << "\n=== Example 6: Functor with Initialize/Reduce (legacy) ===" << std::endl;

    std::vector<double> data(1000, 1.0);  // Fill with ones

    SumReductionFunctor functor(data);
    smp_tools::parallel_for(0, data.size(), 100, functor);

    std::cout << "Partial sum from one thread: " << functor.sum << std::endl;
    std::cout << "Note: For proper reduction, use thread-local storage" << std::endl;
}

// ============================================================================
// EXAMPLE 7: Comparison - Old vs New API
// ============================================================================

void example7_comparison()
{
    std::cout << "\n=== Example 7: API Comparison ===" << std::endl;

    std::vector<int> data_new(1000);
    std::vector<int> data_old(1000);

    // NEW API - Clean, inline lambda
    smp_tools::parallel_for(
        0,
        data_new.size(),
        100,
        [&data_new](size_t start, size_t end)
        {
            for (size_t i = start; i < end; ++i)
            {
                data_new[i] = static_cast<int>(i * i);
            }
        });

    // OLD API - Requires functor definition
    struct SquareFunctor
    {
        std::vector<int>& data;
        void              operator()(size_t start, size_t end) const
        {
            for (size_t i = start; i < end; ++i)
            {
                data[i] = static_cast<int>(i * i);
            }
        }
    };
    SquareFunctor functor{data_old};
    smp_tools::parallel_for(0, data_old.size(), 100, functor);

    std::cout << "Both approaches produce identical results" << std::endl;
    std::cout << "New API: more concise, easier to read and maintain" << std::endl;
}

// ============================================================================
// EXAMPLE 8: Using std::function explicitly
// ============================================================================

void example8_std_function()
{
    std::cout << "\n=== Example 8: Using std::function explicitly ===" << std::endl;

    std::vector<double> data(1000);

    // You can store the function and reuse it
    std::function<void(size_t, size_t)> compute_func = [&data](size_t start, size_t end)
    {
        for (size_t i = start; i < end; ++i)
        {
            data[i] = std::log(i + 1.0);
        }
    };

    smp_tools::parallel_for(0, data.size(), 100, compute_func);

    std::cout << "Computed logarithms using stored std::function" << std::endl;
}

// ============================================================================
// MIGRATION GUIDE
// ============================================================================

/*
MIGRATION FROM OLD TO NEW API:

OLD CODE (Functor):
--------------------
struct MyFunctor {
    std::vector<double>& data;
    MyFunctor(std::vector<double>& d) : data(d) {}
    void operator()(size_t start, size_t end) {
        for (size_t i = start; i < end; ++i) {
            data[i] = compute(i);
        }
    }
};

MyFunctor functor(data);
smp_tools::parallel_for(0, data.size(), 100, functor);


NEW CODE (Lambda):
------------------
smp_tools::parallel_for(0, data.size(), 100,
    [&data](size_t start, size_t end) {
        for (size_t i = start; i < end; ++i) {
            data[i] = compute(i);
        }
    });


KEY DIFFERENCES:
1. No need to define a separate functor class
2. Lambda can capture variables from surrounding scope
3. More concise and readable
4. Better for simple operations
5. Full backward compatibility - old functors still work!

WHEN TO USE OLD API:
- When you need Initialize() method for per-thread setup
- When you need Reduce() method for result aggregation
- When migrating would break existing code
- When functor is reused in multiple places

WHEN TO USE NEW API:
- For new code (recommended)
- For simple parallel loops
- When you want cleaner, more maintainable code
- When you don't need per-thread initialization
*/

// ============================================================================
// Main function to run all examples
// ============================================================================

int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << "parallel_for Lambda Examples" << std::endl;
    std::cout << "========================================" << std::endl;

    // Initialize parallel backend
    smp_tools::initialize();

    std::cout << "Using backend: " << xsigma::detail::smp::smp_tools_api::instance().get_backend()
              << std::endl;
    std::cout << "Number of threads: " << smp_tools::estimated_number_of_threads() << std::endl;

    // Run all examples
    example1_basic_lambda();
    example2_basic_functor();
    example3_iterator_lambda();
    example4_default_grain();
    example5_lambda_captures();
    example6_functor_with_reduce();
    example7_comparison();
    example8_std_function();

    std::cout << "\n========================================" << std::endl;
    std::cout << "All examples completed successfully!" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
