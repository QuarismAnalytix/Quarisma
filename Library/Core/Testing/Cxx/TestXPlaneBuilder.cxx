/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Comprehensive test suite for XPlane Builder classes
 * Tests builder pattern implementation for XPlane data structures
 */

#include <limits>
#include <string>
#include <vector>

#include "Testing/xsigmaTest.h"
#include "profiler/core/timespan.h"
#include "profiler/exporters/xplane/xplane.h"
#include "profiler/exporters/xplane/xplane_builder.h"

using namespace xsigma;

// ============================================================================
// simple_atoi Tests
// ============================================================================

XSIGMATEST(XPlaneBuilder, simple_atoi_valid_positive)
{
    int64_t result = 0;
    EXPECT_TRUE(simple_atoi("12345", &result));
    EXPECT_EQ(result, 12345);
}

XSIGMATEST(XPlaneBuilder, simple_atoi_valid_negative)
{
    int64_t result = 0;
    EXPECT_TRUE(simple_atoi("-9876", &result));
    EXPECT_EQ(result, -9876);
}

XSIGMATEST(XPlaneBuilder, simple_atoi_zero)
{
    int64_t result = 0;
    EXPECT_TRUE(simple_atoi("0", &result));
    EXPECT_EQ(result, 0);
}

XSIGMATEST(XPlaneBuilder, simple_atoi_with_plus_sign)
{
    int64_t result = 0;
    EXPECT_TRUE(simple_atoi("+42", &result));
    EXPECT_EQ(result, 42);
}

XSIGMATEST(XPlaneBuilder, simple_atoi_empty_string)
{
    int64_t result = 0;
    EXPECT_FALSE(simple_atoi("", &result));
}

XSIGMATEST(XPlaneBuilder, simple_atoi_null_pointer)
{
    int64_t* null_ptr = nullptr;
    EXPECT_FALSE(simple_atoi("123", null_ptr));
}

XSIGMATEST(XPlaneBuilder, simple_atoi_invalid_characters)
{
    int64_t result = 0;
    EXPECT_FALSE(simple_atoi("12a34", &result));
}

XSIGMATEST(XPlaneBuilder, simple_atoi_leading_whitespace)
{
    int64_t result = 0;
    EXPECT_FALSE(simple_atoi(" 123", &result));
}

XSIGMATEST(XPlaneBuilder, simple_atoi_trailing_whitespace)
{
    int64_t result = 0;
    EXPECT_FALSE(simple_atoi("123 ", &result));
}

XSIGMATEST(XPlaneBuilder, simple_atoi_overflow_positive)
{
    int64_t result = 0;
    EXPECT_FALSE(simple_atoi("9223372036854775808", &result));  // INT64_MAX + 1
}

XSIGMATEST(XPlaneBuilder, simple_atoi_overflow_negative)
{
    int64_t result = 0;
    EXPECT_FALSE(simple_atoi("-9223372036854775809", &result));  // INT64_MIN - 1
}

XSIGMATEST(XPlaneBuilder, simple_atoi_max_value)
{
    int64_t result = 0;
    EXPECT_TRUE(simple_atoi("9223372036854775807", &result));  // INT64_MAX
    EXPECT_EQ(result, std::numeric_limits<int64_t>::max());
}

XSIGMATEST(XPlaneBuilder, simple_atoi_min_value)
{
    int64_t result = 0;
    EXPECT_TRUE(simple_atoi("-9223372036854775808", &result));  // INT64_MIN
    EXPECT_EQ(result, std::numeric_limits<int64_t>::min());
}

XSIGMATEST(XPlaneBuilder, simple_atoi_uint64_positive)
{
    uint64_t result = 0;
    EXPECT_TRUE(simple_atoi("18446744073709551615", &result));  // UINT64_MAX
    EXPECT_EQ(result, std::numeric_limits<uint64_t>::max());
}

XSIGMATEST(XPlaneBuilder, simple_atoi_only_sign)
{
    int64_t result = 999;
    // simple_atoi accepts "+" and "-" as valid input (returns 0)
    EXPECT_TRUE(simple_atoi("-", &result));
    EXPECT_EQ(result, 0);
    result = 999;
    EXPECT_TRUE(simple_atoi("+", &result));
    EXPECT_EQ(result, 0);
}

// ============================================================================
// simple_atod Tests
// ============================================================================

XSIGMATEST(XPlaneBuilder, simple_atod_valid_positive)
{
    double result = 0.0;
    EXPECT_TRUE(simple_atod("123.456", &result));
    EXPECT_DOUBLE_EQ(result, 123.456);
}

XSIGMATEST(XPlaneBuilder, simple_atod_valid_negative)
{
    double result = 0.0;
    EXPECT_TRUE(simple_atod("-987.654", &result));
    EXPECT_DOUBLE_EQ(result, -987.654);
}

XSIGMATEST(XPlaneBuilder, simple_atod_zero)
{
    double result = 0.0;
    EXPECT_TRUE(simple_atod("0.0", &result));
    EXPECT_DOUBLE_EQ(result, 0.0);
}

XSIGMATEST(XPlaneBuilder, simple_atod_integer)
{
    double result = 0.0;
    EXPECT_TRUE(simple_atod("42", &result));
    EXPECT_DOUBLE_EQ(result, 42.0);
}

XSIGMATEST(XPlaneBuilder, simple_atod_with_plus_sign)
{
    double result = 0.0;
    EXPECT_TRUE(simple_atod("+3.14", &result));
    EXPECT_DOUBLE_EQ(result, 3.14);
}

XSIGMATEST(XPlaneBuilder, simple_atod_empty_string)
{
    double result = 0.0;
    EXPECT_FALSE(simple_atod("", &result));
}

XSIGMATEST(XPlaneBuilder, simple_atod_null_pointer)
{
    double* null_ptr = nullptr;
    EXPECT_FALSE(simple_atod("1.23", null_ptr));
}

XSIGMATEST(XPlaneBuilder, simple_atod_multiple_decimal_points)
{
    double result = 0.0;
    EXPECT_FALSE(simple_atod("1.2.3", &result));
}

XSIGMATEST(XPlaneBuilder, simple_atod_scientific_notation_positive)
{
    double result = 0.0;
    EXPECT_TRUE(simple_atod("1.23e2", &result));
    EXPECT_DOUBLE_EQ(result, 123.0);
}

XSIGMATEST(XPlaneBuilder, simple_atod_scientific_notation_negative)
{
    double result = 0.0;
    EXPECT_TRUE(simple_atod("1.23e-2", &result));
    EXPECT_NEAR(result, 0.0123, 1e-10);
}

XSIGMATEST(XPlaneBuilder, simple_atod_scientific_notation_uppercase)
{
    double result = 0.0;
    EXPECT_TRUE(simple_atod("1.5E3", &result));
    EXPECT_DOUBLE_EQ(result, 1500.0);
}

XSIGMATEST(XPlaneBuilder, simple_atod_exponent_overflow)
{
    double result = 0.0;
    EXPECT_FALSE(simple_atod("1e999999999999999999", &result));
}

XSIGMATEST(XPlaneBuilder, simple_atod_trailing_characters)
{
    double result = 0.0;
    EXPECT_FALSE(simple_atod("1.23abc", &result));
}

XSIGMATEST(XPlaneBuilder, simple_atod_only_decimal_point)
{
    double result = 999.0;
    // simple_atod accepts "." as valid input (returns 0.0)
    EXPECT_TRUE(simple_atod(".", &result));
    EXPECT_DOUBLE_EQ(result, 0.0);
}

XSIGMATEST(XPlaneBuilder, simple_atod_no_digits_after_decimal)
{
    double result = 0.0;
    EXPECT_TRUE(simple_atod("123.", &result));
    EXPECT_DOUBLE_EQ(result, 123.0);
}

// ============================================================================
// xplane_builder Tests
// ============================================================================

XSIGMATEST(XPlaneBuilder, xplane_builder_construction)
{
    xplane         plane;
    xplane_builder builder(&plane);

    EXPECT_EQ(builder.Id(), 0);
    EXPECT_EQ(builder.Name(), "");
}

XSIGMATEST(XPlaneBuilder, xplane_builder_set_id_and_name)
{
    xplane         plane;
    xplane_builder builder(&plane);

    builder.SetId(42);
    builder.SetName("TestPlane");

    EXPECT_EQ(builder.Id(), 42);
    EXPECT_EQ(builder.Name(), "TestPlane");
}

XSIGMATEST(XPlaneBuilder, xplane_builder_get_or_create_line)
{
    xplane         plane;
    xplane_builder builder(&plane);

    auto line1 = builder.get_or_create_line(100);
    auto line2 = builder.get_or_create_line(100);  // Same ID

    EXPECT_EQ(line1.Id(), 100);
    EXPECT_EQ(line2.Id(), 100);
    EXPECT_EQ(plane.lines_size(), 1);  // Should reuse existing line
}

XSIGMATEST(XPlaneBuilder, xplane_builder_multiple_lines)
{
    xplane         plane;
    xplane_builder builder(&plane);

    builder.get_or_create_line(1);
    builder.get_or_create_line(2);
    builder.get_or_create_line(3);

    EXPECT_EQ(plane.lines_size(), 3);
}

XSIGMATEST(XPlaneBuilder, xplane_builder_create_event_metadata)
{
    xplane         plane;
    xplane_builder builder(&plane);

    auto* metadata1 = builder.create_event_metadata();
    auto* metadata2 = builder.create_event_metadata();

    EXPECT_NE(metadata1, nullptr);
    EXPECT_NE(metadata2, nullptr);
    EXPECT_NE(metadata1->id(), metadata2->id());  // Different IDs
}

XSIGMATEST(XPlaneBuilder, xplane_builder_get_or_create_event_metadata_by_id)
{
    xplane         plane;
    xplane_builder builder(&plane);

    auto* metadata1 = builder.get_or_create_event_metadata(100);
    auto* metadata2 = builder.get_or_create_event_metadata(100);  // Same ID

    EXPECT_EQ(metadata1, metadata2);  // Should return same metadata
    EXPECT_EQ(metadata1->id(), 100);
}

XSIGMATEST(XPlaneBuilder, xplane_builder_get_or_create_event_metadata_by_name)
{
    xplane         plane;
    xplane_builder builder(&plane);

    auto* metadata1 = builder.get_or_create_event_metadata("test_event");
    auto* metadata2 = builder.get_or_create_event_metadata("test_event");  // Same name

    EXPECT_EQ(metadata1, metadata2);  // Should return same metadata
    EXPECT_EQ(metadata1->name(), "test_event");
}

XSIGMATEST(XPlaneBuilder, xplane_builder_get_or_create_event_metadata_string_move)
{
    xplane         plane;
    xplane_builder builder(&plane);

    std::string name     = "movable_event";
    auto*       metadata = builder.get_or_create_event_metadata(std::move(name));

    EXPECT_NE(metadata, nullptr);
    EXPECT_EQ(metadata->name(), "movable_event");
}

XSIGMATEST(XPlaneBuilder, xplane_builder_get_or_create_stat_metadata_by_id)
{
    xplane         plane;
    xplane_builder builder(&plane);

    auto* metadata1 = builder.get_or_create_stat_metadata(200);
    auto* metadata2 = builder.get_or_create_stat_metadata(200);  // Same ID

    EXPECT_EQ(metadata1, metadata2);  // Should return same metadata
    EXPECT_EQ(metadata1->id(), 200);
}

XSIGMATEST(XPlaneBuilder, xplane_builder_get_or_create_stat_metadata_by_name)
{
    xplane         plane;
    xplane_builder builder(&plane);

    auto* metadata1 = builder.get_or_create_stat_metadata("test_stat");
    auto* metadata2 = builder.get_or_create_stat_metadata("test_stat");  // Same name

    EXPECT_EQ(metadata1, metadata2);  // Should return same metadata
    EXPECT_EQ(metadata1->name(), "test_stat");
}

XSIGMATEST(XPlaneBuilder, xplane_builder_reserve_lines)
{
    xplane         plane;
    xplane_builder builder(&plane);

    builder.ReserveLines(100);
    // No direct way to verify capacity, but should not crash
    EXPECT_TRUE(true);
}

// ============================================================================
// xline_builder Tests
// ============================================================================

XSIGMATEST(XPlaneBuilder, xline_builder_construction)
{
    xplane         plane;
    xplane_builder builder(&plane);
    auto           line = builder.get_or_create_line(1);

    EXPECT_EQ(line.Id(), 1);
}

XSIGMATEST(XPlaneBuilder, xline_builder_set_name)
{
    xplane         plane;
    xplane_builder builder(&plane);
    auto           line = builder.get_or_create_line(1);

    line.SetName("TestLine");
    EXPECT_EQ(line.Name(), "TestLine");
}

XSIGMATEST(XPlaneBuilder, xline_builder_set_timestamp_ns)
{
    xplane         plane;
    xplane_builder builder(&plane);
    auto           line = builder.get_or_create_line(1);

    line.SetTimestampNs(1000000);
    EXPECT_EQ(line.TimestampNs(), 1000000);
}

XSIGMATEST(XPlaneBuilder, xline_builder_add_event)
{
    xplane         plane;
    xplane_builder builder(&plane);
    auto           line     = builder.get_or_create_line(1);
    auto*          metadata = builder.get_or_create_event_metadata("test_event");

    auto event = line.add_event(*metadata);
    EXPECT_EQ(event.MetadataId(), metadata->id());
}

XSIGMATEST(XPlaneBuilder, xline_builder_add_event_with_timespan)
{
    xplane         plane;
    xplane_builder builder(&plane);
    auto           line     = builder.get_or_create_line(1);
    auto*          metadata = builder.get_or_create_event_metadata("test_event");

    timespan ts(1000, 5000);  // begin=1000, duration=5000
    auto     event = line.add_event(ts, *metadata);

    EXPECT_EQ(event.MetadataId(), metadata->id());
    EXPECT_EQ(event.OffsetPs(), 1000);
    EXPECT_EQ(event.DurationPs(), 5000);  // duration is stored directly
}

XSIGMATEST(XPlaneBuilder, xline_builder_reserve_events)
{
    xplane         plane;
    xplane_builder builder(&plane);
    auto           line = builder.get_or_create_line(1);

    line.ReserveEvents(50);
    // No direct way to verify capacity, but should not crash
    EXPECT_TRUE(true);
}

XSIGMATEST(XPlaneBuilder, xline_builder_num_events)
{
    xplane         plane;
    xplane_builder builder(&plane);
    auto           line     = builder.get_or_create_line(1);
    auto*          metadata = builder.get_or_create_event_metadata("test_event");

    EXPECT_EQ(line.NumEvents(), 0);
    line.add_event(*metadata);
    EXPECT_EQ(line.NumEvents(), 1);
    line.add_event(*metadata);
    EXPECT_EQ(line.NumEvents(), 2);
}

// ============================================================================
// xevent_builder Tests
// ============================================================================

XSIGMATEST(XPlaneBuilder, xevent_builder_set_offset_ps)
{
    xplane         plane;
    xplane_builder builder(&plane);
    auto           line     = builder.get_or_create_line(1);
    auto*          metadata = builder.get_or_create_event_metadata("test_event");
    auto           event    = line.add_event(*metadata);

    event.SetOffsetPs(1000);
    EXPECT_EQ(event.OffsetPs(), 1000);
}

XSIGMATEST(XPlaneBuilder, xevent_builder_set_offset_ns)
{
    xplane         plane;
    xplane_builder builder(&plane);
    auto           line     = builder.get_or_create_line(1);
    auto*          metadata = builder.get_or_create_event_metadata("test_event");
    auto           event    = line.add_event(*metadata);

    event.SetOffsetNs(100);
    EXPECT_EQ(event.OffsetPs(), 100000);  // 100 ns = 100000 ps
}

XSIGMATEST(XPlaneBuilder, xevent_builder_set_timestamp_ns)
{
    xplane         plane;
    xplane_builder builder(&plane);
    auto           line     = builder.get_or_create_line(1);
    auto*          metadata = builder.get_or_create_event_metadata("test_event");
    auto           event    = line.add_event(*metadata);

    line.SetTimestampNs(1000);
    event.SetTimestampNs(2000);
    EXPECT_EQ(event.OffsetPs(), 1000000);  // (2000 - 1000) * 1000 ps
}

XSIGMATEST(XPlaneBuilder, xevent_builder_set_duration_ps)
{
    xplane         plane;
    xplane_builder builder(&plane);
    auto           line     = builder.get_or_create_line(1);
    auto*          metadata = builder.get_or_create_event_metadata("test_event");
    auto           event    = line.add_event(*metadata);

    event.SetDurationPs(5000);
    EXPECT_EQ(event.DurationPs(), 5000);
}

XSIGMATEST(XPlaneBuilder, xevent_builder_set_duration_ns)
{
    xplane         plane;
    xplane_builder builder(&plane);
    auto           line     = builder.get_or_create_line(1);
    auto*          metadata = builder.get_or_create_event_metadata("test_event");
    auto           event    = line.add_event(*metadata);

    event.SetDurationNs(200);
    EXPECT_EQ(event.DurationPs(), 200000);  // 200 ns = 200000 ps
}

XSIGMATEST(XPlaneBuilder, xevent_builder_set_end_timestamp_ns)
{
    xplane         plane;
    xplane_builder builder(&plane);
    auto           line     = builder.get_or_create_line(1);
    auto*          metadata = builder.get_or_create_event_metadata("test_event");
    auto           event    = line.add_event(*metadata);

    line.SetTimestampNs(1000);
    event.SetTimestampNs(2000);
    event.SetEndTimestampNs(3000);
    EXPECT_EQ(event.DurationPs(), 1000000);  // (3000 - 2000) * 1000 ps
}

XSIGMATEST(XPlaneBuilder, xevent_builder_set_timespan)
{
    xplane         plane;
    xplane_builder builder(&plane);
    auto           line     = builder.get_or_create_line(1);
    auto*          metadata = builder.get_or_create_event_metadata("test_event");
    auto           event    = line.add_event(*metadata);

    timespan ts(1000, 5000);  // begin=1000, duration=5000
    event.SetTimespan(ts);
    EXPECT_EQ(event.OffsetPs(), 1000);
    EXPECT_EQ(event.DurationPs(), 5000);  // duration is stored directly
}

XSIGMATEST(XPlaneBuilder, xevent_builder_set_num_occurrences)
{
    xplane         plane;
    xplane_builder builder(&plane);
    auto           line     = builder.get_or_create_line(1);
    auto*          metadata = builder.get_or_create_event_metadata("test_event");
    auto           event    = line.add_event(*metadata);

    event.SetNumOccurrences(10);
    // Verify it was set (no direct getter, but should not crash)
    EXPECT_TRUE(true);
}

XSIGMATEST(XPlaneBuilder, xevent_builder_add_stat_value_int64)
{
    xplane         plane;
    xplane_builder builder(&plane);
    auto           line          = builder.get_or_create_line(1);
    auto*          event_meta    = builder.get_or_create_event_metadata("test_event");
    auto*          stat_metadata = builder.get_or_create_stat_metadata("test_stat");
    auto           event         = line.add_event(*event_meta);

    event.add_stat_value(*stat_metadata, int64_t(42));
    // Verify stat was added (no direct getter, but should not crash)
    EXPECT_TRUE(true);
}

XSIGMATEST(XPlaneBuilder, xevent_builder_add_stat_value_uint64)
{
    xplane         plane;
    xplane_builder builder(&plane);
    auto           line          = builder.get_or_create_line(1);
    auto*          event_meta    = builder.get_or_create_event_metadata("test_event");
    auto*          stat_metadata = builder.get_or_create_stat_metadata("test_stat");
    auto           event         = line.add_event(*event_meta);

    event.add_stat_value(*stat_metadata, uint64_t(100));
    EXPECT_TRUE(true);
}

XSIGMATEST(XPlaneBuilder, xevent_builder_add_stat_value_double)
{
    xplane         plane;
    xplane_builder builder(&plane);
    auto           line          = builder.get_or_create_line(1);
    auto*          event_meta    = builder.get_or_create_event_metadata("test_event");
    auto*          stat_metadata = builder.get_or_create_stat_metadata("test_stat");
    auto           event         = line.add_event(*event_meta);

    event.add_stat_value(*stat_metadata, 3.14);
    EXPECT_TRUE(true);
}
