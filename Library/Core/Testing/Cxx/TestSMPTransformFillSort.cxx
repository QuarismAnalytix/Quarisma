/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Test suite for SMP Transform, Fill, and Sort operations
 */

#include <algorithm>
#include <numeric>
#include <vector>

#include "Testing/xsigmaTest.h"
#include "smp/tools.h"

namespace xsigma
{

// Test 1: Basic Transform operation
XSIGMATEST(SMPTransformFillSort, transform_basic)
{
    std::vector<int> input(100);
    std::iota(input.begin(), input.end(), 0);
    std::vector<int> output(100, 0);

    tools::Transform(input.begin(), input.end(), output.begin(), [](int x) { return x * 2; });

    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(output[i], i * 2);
    }
}

// Test 2: Transform with different types
XSIGMATEST(SMPTransformFillSort, transform_type_conversion)
{
    std::vector<int> input(50);
    std::iota(input.begin(), input.end(), 1);
    std::vector<double> output(50, 0.0);

    tools::Transform(
        input.begin(),
        input.end(),
        output.begin(),
        [](int x) { return static_cast<double>(x) * 1.5; });

    EXPECT_DOUBLE_EQ(output[0], 1.5);
    EXPECT_DOUBLE_EQ(output[49], 75.0);
}

// Test 3: Transform with complex operation
XSIGMATEST(SMPTransformFillSort, transform_complex_operation)
{
    std::vector<double> input(100);
    std::iota(input.begin(), input.end(), 1.0);
    std::vector<double> output(100, 0.0);

    tools::Transform(
        input.begin(), input.end(), output.begin(), [](double x) { return x * x + 2.0 * x + 1.0; });

    EXPECT_DOUBLE_EQ(output[0], 4.0);   // 1^2 + 2*1 + 1
    EXPECT_DOUBLE_EQ(output[1], 9.0);   // 2^2 + 2*2 + 1
    EXPECT_DOUBLE_EQ(output[2], 16.0);  // 3^2 + 2*3 + 1
}

// Test 4: Transform with empty range
XSIGMATEST(SMPTransformFillSort, transform_empty_range)
{
    std::vector<int> input;
    std::vector<int> output;

    tools::Transform(input.begin(), input.end(), output.begin(), [](int x) { return x * 2; });

    EXPECT_TRUE(output.empty());
}

// Test 5: Transform with single element
XSIGMATEST(SMPTransformFillSort, transform_single_element)
{
    std::vector<int> input = {42};
    std::vector<int> output(1, 0);

    tools::Transform(input.begin(), input.end(), output.begin(), [](int x) { return x + 10; });

    EXPECT_EQ(output[0], 52);
}

// Test 6: Transform with large dataset
XSIGMATEST(SMPTransformFillSort, transform_large_dataset)
{
    const int        size = 100000;
    std::vector<int> input(size);
    std::iota(input.begin(), input.end(), 0);
    std::vector<int> output(size, 0);

    tools::Transform(input.begin(), input.end(), output.begin(), [](int x) { return x + 1; });

    EXPECT_EQ(output[0], 1);
    EXPECT_EQ(output[size - 1], size);
}

// Test 7: Basic Fill operation
XSIGMATEST(SMPTransformFillSort, fill_basic)
{
    std::vector<int> data(100, 0);

    tools::Fill(data.begin(), data.end(), 42);

    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(data[i], 42);
    }
}

// Test 8: Fill with different types
XSIGMATEST(SMPTransformFillSort, fill_different_types)
{
    std::vector<double> data(50, 0.0);

    tools::Fill(data.begin(), data.end(), 3.14159);

    for (int i = 0; i < 50; ++i)
    {
        EXPECT_DOUBLE_EQ(data[i], 3.14159);
    }
}

// Test 9: Fill with empty range
XSIGMATEST(SMPTransformFillSort, fill_empty_range)
{
    std::vector<int> data;

    tools::Fill(data.begin(), data.end(), 10);

    EXPECT_TRUE(data.empty());
}

// Test 10: Fill with single element
XSIGMATEST(SMPTransformFillSort, fill_single_element)
{
    std::vector<int> data(1, 0);

    tools::Fill(data.begin(), data.end(), 99);

    EXPECT_EQ(data[0], 99);
}

// Test 11: Fill with large dataset
XSIGMATEST(SMPTransformFillSort, fill_large_dataset)
{
    const int        size = 100000;
    std::vector<int> data(size, 0);

    tools::Fill(data.begin(), data.end(), 7);

    EXPECT_EQ(data[0], 7);
    EXPECT_EQ(data[size / 2], 7);
    EXPECT_EQ(data[size - 1], 7);
}

// Test 12: Basic Sort operation
XSIGMATEST(SMPTransformFillSort, sort_basic)
{
    std::vector<int> data = {5, 2, 8, 1, 9, 3, 7, 4, 6};

    tools::Sort(data.begin(), data.end());

    for (size_t i = 0; i < data.size(); ++i)
    {
        EXPECT_EQ(data[i], static_cast<int>(i + 1));
    }
}

// Test 13: Sort with custom comparator
XSIGMATEST(SMPTransformFillSort, sort_custom_comparator)
{
    std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    tools::Sort(data.begin(), data.end(), std::greater<int>());

    for (size_t i = 0; i < data.size(); ++i)
    {
        EXPECT_EQ(data[i], static_cast<int>(9 - i));
    }
}

// Test 14: Sort with duplicates
XSIGMATEST(SMPTransformFillSort, sort_with_duplicates)
{
    std::vector<int> data = {5, 2, 8, 2, 9, 5, 7, 2, 5};

    tools::Sort(data.begin(), data.end());

    EXPECT_TRUE(std::is_sorted(data.begin(), data.end()));
}

// Test 15: Sort already sorted data
XSIGMATEST(SMPTransformFillSort, sort_already_sorted)
{
    std::vector<int> data(100);
    std::iota(data.begin(), data.end(), 0);

    tools::Sort(data.begin(), data.end());

    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(data[i], i);
    }
}

// Test 16: Sort reverse sorted data
XSIGMATEST(SMPTransformFillSort, sort_reverse_sorted)
{
    std::vector<int> data(100);
    std::iota(data.rbegin(), data.rend(), 0);

    tools::Sort(data.begin(), data.end());

    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(data[i], i);
    }
}

// Test 17: Sort with empty range
XSIGMATEST(SMPTransformFillSort, sort_empty_range)
{
    std::vector<int> data;

    tools::Sort(data.begin(), data.end());

    EXPECT_TRUE(data.empty());
}

// Test 18: Sort with single element
XSIGMATEST(SMPTransformFillSort, sort_single_element)
{
    std::vector<int> data = {42};

    tools::Sort(data.begin(), data.end());

    EXPECT_EQ(data[0], 42);
}

// Test 19: Sort with large dataset
XSIGMATEST(SMPTransformFillSort, sort_large_dataset)
{
    const int        size = 100000;
    std::vector<int> data(size);

    // Fill with random-ish data
    for (int i = 0; i < size; ++i)
    {
        data[i] = (i * 7919) % size;
    }

    tools::Sort(data.begin(), data.end());

    EXPECT_TRUE(std::is_sorted(data.begin(), data.end()));
}

// Test 20: Sort with floating point
XSIGMATEST(SMPTransformFillSort, sort_floating_point)
{
    std::vector<double> data = {3.14, 1.41, 2.71, 0.57, 1.61};

    tools::Sort(data.begin(), data.end());

    EXPECT_DOUBLE_EQ(data[0], 0.57);
    EXPECT_DOUBLE_EQ(data[1], 1.41);
    EXPECT_DOUBLE_EQ(data[2], 1.61);
    EXPECT_DOUBLE_EQ(data[3], 2.71);
    EXPECT_DOUBLE_EQ(data[4], 3.14);
}

// Test 21: Transform in-place
XSIGMATEST(SMPTransformFillSort, transform_in_place)
{
    std::vector<int> data(100);
    std::iota(data.begin(), data.end(), 0);

    tools::Transform(data.begin(), data.end(), data.begin(), [](int x) { return x * x; });

    EXPECT_EQ(data[0], 0);
    EXPECT_EQ(data[1], 1);
    EXPECT_EQ(data[2], 4);
    EXPECT_EQ(data[10], 100);
}

}  // namespace xsigma
