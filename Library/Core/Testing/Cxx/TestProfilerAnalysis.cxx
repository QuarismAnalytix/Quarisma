/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 */

#include <gtest/gtest.h>

#include <cmath>
#include <sstream>

#include "profiler/analysis/stats_calculator.h"
#include "xsigmaTest.h"

using namespace xsigma;

// ============================================================================
// Stat Template Tests
// ============================================================================

XSIGMATEST(Profiler, stat_empty_initialization)
{
    xsigma::stat<int64_t> s;

    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.count(), 0);
}

XSIGMATEST(Profiler, stat_single_update)
{
    xsigma::stat<int64_t> s;
    s.update_stat(10);

    EXPECT_FALSE(s.empty());
    EXPECT_EQ(s.count(), 1);
    EXPECT_EQ(s.first(), 10);
    EXPECT_EQ(s.newest(), 10);
}

XSIGMATEST(Profiler, stat_multiple_updates)
{
    xsigma::stat<int64_t> s;
    s.update_stat(10);
    s.update_stat(20);
    s.update_stat(30);

    EXPECT_EQ(s.count(), 3);
    EXPECT_EQ(s.first(), 10);
    EXPECT_EQ(s.newest(), 30);
}

XSIGMATEST(Profiler, stat_min_max)
{
    xsigma::stat<int64_t> s;
    s.update_stat(10);
    s.update_stat(5);
    s.update_stat(20);

    EXPECT_EQ(s.min(), 5);
    EXPECT_EQ(s.max(), 20);
}

XSIGMATEST(Profiler, stat_sum)
{
    xsigma::stat<int64_t> s;
    s.update_stat(10);
    s.update_stat(20);
    s.update_stat(30);

    EXPECT_EQ(s.sum(), 60);
}

XSIGMATEST(Profiler, stat_average)
{
    xsigma::stat<int64_t> s;
    s.update_stat(10);
    s.update_stat(20);
    s.update_stat(30);

    double avg = s.avg();
    EXPECT_DOUBLE_EQ(avg, 20.0);
}

XSIGMATEST(Profiler, stat_all_same)
{
    xsigma::stat<int64_t> s;
    s.update_stat(10);
    s.update_stat(10);
    s.update_stat(10);

    EXPECT_TRUE(s.all_same());
}

XSIGMATEST(Profiler, stat_not_all_same)
{
    xsigma::stat<int64_t> s;
    s.update_stat(10);
    s.update_stat(20);

    EXPECT_FALSE(s.all_same());
}

XSIGMATEST(Profiler, stat_variance_all_same)
{
    xsigma::stat<int64_t> s;
    s.update_stat(10);
    s.update_stat(10);
    s.update_stat(10);

    EXPECT_EQ(s.variance(), 0);
}

XSIGMATEST(Profiler, stat_std_deviation_all_same)
{
    xsigma::stat<int64_t> s;
    s.update_stat(10);
    s.update_stat(10);
    s.update_stat(10);

    EXPECT_EQ(s.std_deviation(), 0);
}

XSIGMATEST(Profiler, stat_reset)
{
    xsigma::stat<int64_t> s;
    s.update_stat(10);
    s.update_stat(20);

    EXPECT_EQ(s.count(), 2);

    s.reset();

    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.count(), 0);
}

XSIGMATEST(Profiler, stat_output_stream_empty)
{
    xsigma::stat<int64_t> s;
    std::ostringstream    oss;
    s.output_to_stream(&oss);

    std::string output = oss.str();
    EXPECT_NE(output.find("count=0"), std::string::npos);
}

XSIGMATEST(Profiler, stat_output_stream_with_data)
{
    xsigma::stat<int64_t> s;
    s.update_stat(10);
    s.update_stat(20);

    std::ostringstream oss;
    s.output_to_stream(&oss);

    std::string output = oss.str();
    EXPECT_NE(output.find("count=2"), std::string::npos);
}

XSIGMATEST(Profiler, stat_with_percentiles_empty)
{
    xsigma::stat_with_percentiles<int64_t> s;

    EXPECT_TRUE(s.empty());
}

XSIGMATEST(Profiler, stat_with_percentiles_update)
{
    xsigma::stat_with_percentiles<int64_t> s;
    for (int i = 1; i <= 100; ++i)
    {
        s.update_stat(i);
    }

    EXPECT_EQ(s.count(), 100);
}

XSIGMATEST(Profiler, stat_with_percentiles_percentile_50)
{
    xsigma::stat_with_percentiles<int64_t> s;
    for (int i = 1; i <= 100; ++i)
    {
        s.update_stat(i);
    }

    int64_t p50 = s.percentile(50);
    EXPECT_GT(p50, 0);
}

XSIGMATEST(Profiler, stat_double_type)
{
    xsigma::stat<double> s;
    s.update_stat(1.5);
    s.update_stat(2.5);
    s.update_stat(3.5);

    EXPECT_EQ(s.count(), 3);
    EXPECT_DOUBLE_EQ(s.avg(), 2.5);
}

XSIGMATEST(Profiler, stat_with_percentiles_percentile_invalid)
{
    xsigma::stat_with_percentiles<int64_t> s;
    for (int i = 1; i <= 100; ++i)
    {
        s.update_stat(i);
    }

    // Invalid percentile should return NaN
    int64_t p_invalid = s.percentile(101);
    EXPECT_TRUE(std::isnan(static_cast<double>(p_invalid)) || p_invalid == 0);
}

XSIGMATEST(Profiler, stat_with_percentiles_percentile_100)
{
    xsigma::stat_with_percentiles<int64_t> s;
    for (int i = 1; i <= 100; ++i)
    {
        s.update_stat(i);
    }

    int64_t p100 = s.percentile(100);
    EXPECT_EQ(p100, 100);
}

XSIGMATEST(Profiler, stat_variance_calculation)
{
    xsigma::stat<int64_t> s;
    s.update_stat(1);
    s.update_stat(2);
    s.update_stat(3);
    s.update_stat(4);
    s.update_stat(5);

    double variance = s.variance();
    EXPECT_GT(variance, 0);
}

XSIGMATEST(Profiler, stat_sample_variance)
{
    xsigma::stat<int64_t> s;
    s.update_stat(1);
    s.update_stat(2);
    s.update_stat(3);

    int64_t sample_var = s.sample_variance();
    EXPECT_GE(sample_var, 0);
}

// ============================================================================
// Additional Statistical Analysis Tests
// ============================================================================

XSIGMATEST(Profiler, stat_squared_sum)
{
    xsigma::stat<int64_t> s;
    s.update_stat(2);
    s.update_stat(3);
    s.update_stat(4);

    // squared_sum should be 4 + 9 + 16 = 29
    double squared_sum = s.squared_sum();
    EXPECT_EQ(squared_sum, 29.0);
}

XSIGMATEST(Profiler, stat_with_negative_values)
{
    xsigma::stat<int64_t> s;
    s.update_stat(-10);
    s.update_stat(-5);
    s.update_stat(0);
    s.update_stat(5);
    s.update_stat(10);

    EXPECT_EQ(s.min(), -10);
    EXPECT_EQ(s.max(), 10);
    EXPECT_EQ(s.sum(), 0);
}

XSIGMATEST(Profiler, stat_with_large_values)
{
    xsigma::stat<int64_t> s;
    s.update_stat(1000000000);
    s.update_stat(2000000000);
    s.update_stat(3000000000);

    EXPECT_EQ(s.count(), 3);
    EXPECT_EQ(s.sum(), 6000000000LL);
}

XSIGMATEST(Profiler, stat_with_double_type)
{
    xsigma::stat<double> s;
    s.update_stat(1.5);
    s.update_stat(2.5);
    s.update_stat(3.5);

    EXPECT_DOUBLE_EQ(s.avg(), 2.5);
    EXPECT_DOUBLE_EQ(s.sum(), 7.5);
}

XSIGMATEST(Profiler, stat_with_percentiles_output_stream)
{
    xsigma::stat_with_percentiles<int64_t> s;
    for (int i = 1; i <= 100; ++i)
    {
        s.update_stat(i);
    }

    std::ostringstream oss;
    s.output_to_stream(&oss);

    std::string output = oss.str();
    EXPECT_NE(output.find("p5="), std::string::npos);
    EXPECT_NE(output.find("median="), std::string::npos);
    EXPECT_NE(output.find("p95="), std::string::npos);
}

XSIGMATEST(Profiler, stat_with_percentiles_percentile_0)
{
    xsigma::stat_with_percentiles<int64_t> s;
    for (int i = 1; i <= 100; ++i)
    {
        s.update_stat(i);
    }

    int64_t p0 = s.percentile(0);
    EXPECT_GT(p0, 0);
}

XSIGMATEST(Profiler, stat_with_percentiles_percentile_25)
{
    xsigma::stat_with_percentiles<int64_t> s;
    for (int i = 1; i <= 100; ++i)
    {
        s.update_stat(i);
    }

    int64_t p25 = s.percentile(25);
    EXPECT_GT(p25, 0);
    EXPECT_LE(p25, 100);
}

XSIGMATEST(Profiler, stat_with_percentiles_percentile_75)
{
    xsigma::stat_with_percentiles<int64_t> s;
    for (int i = 1; i <= 100; ++i)
    {
        s.update_stat(i);
    }

    int64_t p75 = s.percentile(75);
    EXPECT_GT(p75, 0);
    EXPECT_LE(p75, 100);
}

XSIGMATEST(Profiler, stat_with_percentiles_percentile_99)
{
    xsigma::stat_with_percentiles<int64_t> s;
    for (int i = 1; i <= 100; ++i)
    {
        s.update_stat(i);
    }

    int64_t p99 = s.percentile(99);
    EXPECT_GT(p99, 0);
    EXPECT_LE(p99, 100);
}

XSIGMATEST(Profiler, stat_with_percentiles_single_value)
{
    xsigma::stat_with_percentiles<int64_t> s;
    s.update_stat(42);

    int64_t p50 = s.percentile(50);
    EXPECT_EQ(p50, 42);
}

XSIGMATEST(Profiler, stat_with_percentiles_two_values)
{
    xsigma::stat_with_percentiles<int64_t> s;
    s.update_stat(10);
    s.update_stat(20);

    int64_t p50 = s.percentile(50);
    EXPECT_GE(p50, 10);
    EXPECT_LE(p50, 20);
}

XSIGMATEST(Profiler, stat_reset_clears_all_data)
{
    xsigma::stat<int64_t> s;
    s.update_stat(10);
    s.update_stat(20);
    s.update_stat(30);

    EXPECT_EQ(s.count(), 3);

    s.reset();

    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.count(), 0);
    EXPECT_EQ(s.sum(), 0);
}

XSIGMATEST(Profiler, stat_output_stream_all_same)
{
    xsigma::stat<int64_t> s;
    s.update_stat(10);
    s.update_stat(10);
    s.update_stat(10);

    std::ostringstream oss;
    s.output_to_stream(&oss);

    std::string output = oss.str();
    EXPECT_NE(output.find("all same"), std::string::npos);
}

XSIGMATEST(Profiler, stat_output_stream_operator)
{
    xsigma::stat<int64_t> s;
    s.update_stat(10);
    s.update_stat(20);

    std::ostringstream oss;
    oss << s;

    std::string output = oss.str();
    EXPECT_NE(output.find("count=2"), std::string::npos);
}
