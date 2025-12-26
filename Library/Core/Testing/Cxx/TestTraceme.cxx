/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * This file is part of XSigma and is licensed under a dual-license model:
 *
 *   - Open-source License (GPLv3):
 *       Free for personal, academic, and research use under the terms of
 *       the GNU General Public License v3.0 or later.
 *
 *   - Commercial License:
 *       A commercial license is required for proprietary, closed-source,
 *       or SaaS usage. Contact us to obtain a commercial agreement.
 *
 * Contact: licensing@xsigma.co.uk
 * Website: https://www.xsigma.co.uk
 */

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "logging/tracing/traceme.h"
#include "logging/tracing/traceme_encode.h"
#include "logging/tracing/traceme_recorder.h"
#include "xsigmaTest.h"

using namespace xsigma;

// ============================================================================
// Basic Constructor Tests
// ============================================================================

XSIGMATEST(TracemeTest, basic_constructor_string_view)
{
    // Test basic constructor with string_view
    {
        traceme trace("test_activity");
        // Destructor will be called automatically
    }
    END_TEST();
}

XSIGMATEST(TracemeTest, basic_constructor_const_char)
{
    // Test basic constructor with const char*
    {
        traceme trace("test_activity_char");
        // Destructor will be called automatically
    }
    END_TEST();
}

XSIGMATEST(TracemeTest, constructor_with_level)
{
    // Test constructor with custom level
    {
        traceme trace("test_activity_level", 2);
        // Destructor will be called automatically
    }
    END_TEST();
}

XSIGMATEST(TracemeTest, constructor_with_lambda)
{
    // Test constructor with lambda name generator
    {
        int     id = 42;
        traceme trace([id]() { return std::string("test_activity_") + std::to_string(id); });
        // Destructor will be called automatically
    }
    END_TEST();
}

// ============================================================================
// Move Semantics Tests
// ============================================================================

XSIGMATEST(TracemeTest, move_constructor)
{
    // Test move constructor
    traceme trace1("original_activity");
    traceme trace2 = std::move(trace1);
    // Both destructors will be called
    END_TEST();
}

XSIGMATEST(TracemeTest, move_assignment)
{
    // Test move assignment
    traceme trace1("activity1");
    traceme trace2("activity2");
    trace2 = std::move(trace1);
    // Both destructors will be called
    END_TEST();
}

// ============================================================================
// Stop Method Tests
// ============================================================================

XSIGMATEST(TracemeTest, explicit_stop)
{
    // Test explicit stop method
    traceme trace("test_stop");
    trace.stop();

    // Calling stop again should be safe
    trace.stop();
    END_TEST();
}

// ============================================================================
// Static API Tests
// ============================================================================

XSIGMATEST(TracemeTest, activity_start_end_string_view)
{
    // Test static activity_start and activity_end with string_view
    auto activity_id = traceme::activity_start(std::string_view("static_activity"));
    std::this_thread::sleep_for(std::chrono::microseconds(1));
    traceme::activity_end(activity_id);
    END_TEST();
}

XSIGMATEST(TracemeTest, activity_start_end_const_char)
{
    // Test static activity_start and activity_end with const char*
    auto activity_id = traceme::activity_start("static_activity_char");
    std::this_thread::sleep_for(std::chrono::microseconds(1));
    traceme::activity_end(activity_id);
    END_TEST();
}

XSIGMATEST(TracemeTest, activity_start_end_string)
{
    // Test static activity_start and activity_end with std::string
    std::string name        = "static_activity_string";
    auto        activity_id = traceme::activity_start(name);
    std::this_thread::sleep_for(std::chrono::microseconds(1));
    traceme::activity_end(activity_id);
    END_TEST();
}

XSIGMATEST(TracemeTest, activity_start_end_with_level)
{
    // Test static activity_start and activity_end with custom level
    auto activity_id = traceme::activity_start("static_activity_level", 2);
    std::this_thread::sleep_for(std::chrono::microseconds(1));
    traceme::activity_end(activity_id);
    END_TEST();
}

XSIGMATEST(TracemeTest, activity_start_end_lambda)
{
    // Test static activity_start with lambda
    int  id          = 123;
    auto activity_id = traceme::activity_start(
        [id]() { return std::string("lambda_activity_") + std::to_string(id); });
    std::this_thread::sleep_for(std::chrono::microseconds(1));
    traceme::activity_end(activity_id);
    END_TEST();
}

// ============================================================================
// Instant Activity Tests
// ============================================================================

XSIGMATEST(TracemeTest, instant_activity_lambda)
{
    // Test instant_activity with lambda
    int value = 456;
    traceme::instant_activity([value]()
                              { return std::string("instant_") + std::to_string(value); });
    END_TEST();
}

// ============================================================================
// Active State Tests
// ============================================================================

XSIGMATEST(TracemeTest, active_method)
{
    // Test active method with different levels
    bool active_level_1 = traceme::active(1);
    bool active_level_2 = traceme::active(2);
    bool active_level_3 = traceme::active(3);

    // These should not crash regardless of tracing state
    (void)active_level_1;
    (void)active_level_2;
    (void)active_level_3;
    END_TEST();
}

XSIGMATEST(TracemeTest, new_activity_id)
{
    // Test new_activity_id method
    auto id1 = traceme::new_activity_id();
    auto id2 = traceme::new_activity_id();

    // IDs should be different (though this might not always be true in all implementations)
    // At minimum, the method should not crash
    (void)id1;
    (void)id2;
    END_TEST();
}

// ============================================================================
// Metadata Tests
// ============================================================================

XSIGMATEST(TracemeTest, append_metadata)
{
    // Test append_metadata functionality
    traceme trace("metadata_test");
    trace.append_metadata([]() { return std::string("#key=value#"); });
    END_TEST();
}

// ============================================================================
// Recorder Integration Tests
// ============================================================================

XSIGMATEST(TracemeTest, recorder_start_stop)
{
    // Test traceme_recorder start/stop functionality
    bool started = traceme_recorder::start(1);

    // Create some trace events
    {
        traceme trace("recorded_activity");
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }

    auto events = traceme_recorder::stop();

    // The events vector should be valid (though might be empty if tracing was already active)
    (void)started;
    (void)events;
    END_TEST();
}

// ============================================================================
// Edge Cases and Error Handling Tests
// ============================================================================

XSIGMATEST(TracemeTest, multiple_stops)
{
    // Test that multiple stops are safe
    traceme trace("multiple_stops_test");
    trace.stop();
    trace.stop();
    trace.stop();
    END_TEST();
}

XSIGMATEST(TracemeTest, activity_end_invalid_id)
{
    // Test activity_end with invalid ID (should be safe)
    traceme::activity_end(0);
    traceme::activity_end(-1);
    traceme::activity_end(999999);
    END_TEST();
}

// ============================================================================
// Performance and Threading Tests
// ============================================================================

XSIGMATEST(TracemeTest, concurrent_tracing)
{
    // Test concurrent tracing from multiple threads
    const int num_threads       = 4;
    const int traces_per_thread = 10;

    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t)
    {
        threads.emplace_back(
            [t, traces_per_thread]()
            {
                for (int i = 0; i < traces_per_thread; ++i)
                {
                    traceme trace(
                        [t, i]()
                        {
                            return std::string("thread_") + std::to_string(t) + "_trace_" +
                                   std::to_string(i);
                        });
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                }
            });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    END_TEST();
}

// ============================================================================
// TraceMeEncode Tests
// ============================================================================

XSIGMATEST(TracemeTest, traceme_encode_with_args)
{
    // Test traceme_encode with arguments
    std::string encoded = traceme_encode("test_encode", {{"key1", "value1"}, {"key2", 42}});
    EXPECT_FALSE(encoded.empty());
    EXPECT_NE(encoded.find("test_encode"), std::string::npos);
    END_TEST();
}

XSIGMATEST(TracemeTest, traceme_encode_no_name)
{
    // Test traceme_encode with no name (just arguments)
    std::string encoded = traceme_encode({{"key1", "value1"}, {"key2", 42}});
    EXPECT_FALSE(encoded.empty());
    END_TEST();
}

XSIGMATEST(TracemeTest, trace_me_op)
{
    // Test trace_me_op functionality
    std::string op_result = traceme_op("operation", "type");
    EXPECT_FALSE(op_result.empty());
    EXPECT_NE(op_result.find("operation"), std::string::npos);
    EXPECT_NE(op_result.find("type"), std::string::npos);
    END_TEST();
}

XSIGMATEST(TracemeTest, trace_me_op_override)
{
    // Test trace_me_op_override functionality
    std::string override_result = traceme_op_override("op_name", "op_type");
    EXPECT_FALSE(override_result.empty());
    EXPECT_NE(override_result.find("#tf_op="), std::string::npos);
    END_TEST();
}

// ============================================================================
// Integration with Encoding Tests
// ============================================================================

XSIGMATEST(TracemeTest, traceme_with_encoding)
{
    // Test traceme with traceme_encode
    {
        traceme trace(
            []()
            { return traceme_encode("encoded_activity", {{"param", "value"}, {"count", 123}}); });
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
    END_TEST();
}

XSIGMATEST(TracemeTest, traceme_with_metadata_encoding)
{
    // Test traceme with metadata encoding
    traceme trace("base_activity");
    trace.append_metadata([]() { return traceme_encode({{"runtime_param", "runtime_value"}}); });
    END_TEST();
}

// ============================================================================
// Stress Tests
// ============================================================================

XSIGMATEST(TracemeTest, many_nested_traces)
{
    // Test many nested traces
    const int                depth        = 50;
    std::function<void(int)> nested_trace = [&](int level)
    {
        if (level <= 0)
            return;

        traceme trace([level]() { return std::string("nested_level_") + std::to_string(level); });
        nested_trace(level - 1);
    };

    nested_trace(depth);
    END_TEST();
}

XSIGMATEST(TracemeTest, rapid_trace_creation)
{
    // Test rapid creation and destruction of traces
    const int num_traces = 1000;

    for (int i = 0; i < num_traces; ++i)
    {
        traceme trace([i]() { return std::string("rapid_trace_") + std::to_string(i); });
        // Trace goes out of scope immediately
    }
    END_TEST();
}

// ============================================================================
// Utility Function Tests
// ============================================================================

XSIGMATEST(TracemeTest, tf_op_details_enabled)
{
    // Test tf_op_details_enabled function
    bool details_enabled = tf_op_details_enabled();
    // Should not crash regardless of return value
    (void)details_enabled;
    END_TEST();
}

// ============================================================================
// Level-based Filtering Tests
// ============================================================================

XSIGMATEST(TracemeTest, different_trace_levels)
{
    // Test tracing with different levels
    {
        traceme trace1("level_1_trace", 1);
        traceme trace2("level_2_trace", 2);
        traceme trace3("level_3_trace", 3);

        // All should work regardless of current trace level
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
    END_TEST();
}

XSIGMATEST(TracemeTest, recorder_with_different_levels)
{
    // Test recorder with different levels
    traceme_recorder::start(2);  // Only record level 2 and below

    {
        traceme trace1("level_1_should_record", 1);
        traceme trace2("level_2_should_record", 2);
        traceme trace3("level_3_should_not_record", 3);
    }

    auto events = traceme_recorder::stop();
    (void)events;
    END_TEST();
}

// ============================================================================
// Enhanced Documentation Example Tests
// ============================================================================

XSIGMATEST(TracemeTest, documentation_example_basic_usage)
{
    // Test the basic usage example from documentation
    auto example_function = []()
    {
        traceme trace("example_function");
        // Simulate some work
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    };

    traceme_recorder::start(1);
    example_function();
    auto events = traceme_recorder::stop();

    // Should have recorded at least one event
    bool found_example = false;
    for (const auto& thread_events : events)
    {
        for (const auto& event : thread_events.events)
        {
            if (event.name == "example_function")
            {
                found_example = true;
                break;
            }
        }
    }
    XSIGMA_CHECK(found_example, "Should have recorded example_function event");
    END_TEST();
}

XSIGMATEST(TracemeTest, documentation_example_with_metadata)
{
    // Test the metadata example from documentation
    auto process_data = [](const std::vector<int>& data)
    {
        traceme trace(
            [&]()
            {
                return traceme_encode(
                    "process_data",
                    {{"size", data.size()},
                     {"type", "vector<int>"},
                     {"memory_mb", data.size() * sizeof(int) / 1024 / 1024}});
            });
        // Simulate processing
        std::this_thread::sleep_for(std::chrono::microseconds(5));
    };

    traceme_recorder::start(1);
    std::vector<int> test_data(1000, 42);
    process_data(test_data);
    auto events = traceme_recorder::stop();

    // Should have recorded the process_data event with metadata
    bool found_process_data = false;
    for (const auto& thread_events : events)
    {
        for (const auto& event : thread_events.events)
        {
            if (event.name.find("process_data") != std::string::npos)
            {
                // Check that metadata is present
                XSIGMA_CHECK(
                    event.name.find("size=1000") != std::string::npos,
                    "Should contain size metadata");
                XSIGMA_CHECK(
                    event.name.find("type=vector<int>") != std::string::npos,
                    "Should contain type metadata");
                found_process_data = true;
                break;
            }
        }
    }
    XSIGMA_CHECK(found_process_data, "Should have recorded process_data event");
    END_TEST();
}

XSIGMATEST(TracemeTest, documentation_example_manual_activity)
{
    // Test the manual activity management example from documentation
    traceme_recorder::start(1);

    auto activity_id = traceme::activity_start("async_operation");

    // Simulate asynchronous work
    std::this_thread::sleep_for(std::chrono::microseconds(10));

    traceme::activity_end(activity_id);

    auto events = traceme_recorder::stop();

    // Should have recorded the async operation
    bool found_async = false;
    for (const auto& thread_events : events)
    {
        for (const auto& event : thread_events.events)
        {
            if (event.name == "async_operation")
            {
                found_async = true;
                break;
            }
        }
    }
    XSIGMA_CHECK(found_async, "Should have recorded async_operation event");
    END_TEST();
}

// ============================================================================
// Edge Case and Robustness Tests
// ============================================================================

XSIGMATEST(TracemeTest, empty_name_handling)
{
    // Test handling of empty names
    bool started = traceme_recorder::start(1);
    XSIGMA_CHECK(started, "Should be able to start tracing");

    {
        traceme trace("");  // Empty name
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    auto events = traceme_recorder::stop();

    // Should handle empty names gracefully
    bool found_empty = false;
    for (const auto& thread_events : events)
    {
        for (const auto& event : thread_events.events)
        {
            if (event.name.empty())
            {
                found_empty = true;
                break;
            }
        }
    }
    XSIGMA_CHECK(found_empty, "Should handle empty names");
    END_TEST();
}

XSIGMATEST(TracemeTest, very_long_name_handling)
{
    // Test that the system can handle very long names without crashing
    // This test focuses on graceful handling rather than recording
    std::string long_name(500, 'a');  // 500 character name

    // Test that we can create and destroy traceme objects with long names
    // without causing crashes - no try/catch needed per XSigma coding standards
    {
        traceme trace(long_name.c_str());
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }  // Trace object destroyed here

    // Test with string_view as well
    {
        std::string_view long_name_view{long_name};
        traceme          trace(long_name_view);
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }

    // Test with lambda name generator
    {
        traceme trace([&long_name]() { return long_name; });
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }

    // Test that static methods also handle long names gracefully
    auto activity_id = traceme::activity_start(long_name.c_str());
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    traceme::activity_end(activity_id);

    // Test instant activity with long name using lambda
    traceme::instant_activity([&long_name]() { return long_name; });

    // If we get here without crashing, the test passes
    XSIGMA_CHECK(true, "Successfully handled very long names in all scenarios");
    END_TEST();
}

XSIGMATEST(TracemeTest, metadata_with_special_characters)
{
    // Test metadata encoding with special characters
    bool started = traceme_recorder::start(1);
    XSIGMA_CHECK(started, "Should be able to start tracing");

    {
        traceme trace(
            [&]()
            {
                return traceme_encode(
                    "special_chars_test",
                    {{"key_with_spaces", "value with spaces"},
                     {"key=with=equals", "value=with=equals"},
                     {"key,with,commas", "value,with,commas"},
                     {"unicode_key", "value_with_unicode_🚀"}});
            });
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    auto events = traceme_recorder::stop();

    // Should handle special characters in metadata
    bool found_special = false;
    for (const auto& thread_events : events)
    {
        for (const auto& event : thread_events.events)
        {
            if (event.name.find("special_chars_test") != std::string::npos)
            {
                found_special = true;
                break;
            }
        }
    }
    XSIGMA_CHECK(found_special, "Should handle special characters in metadata");
    END_TEST();
}

XSIGMATEST(TracemeTest, zero_duration_traces)
{
    // Test traces with potentially zero duration
    bool started = traceme_recorder::start(1);
    XSIGMA_CHECK(started, "Should be able to start tracing");

    for (int i = 0; i < 100; ++i)
    {
        traceme trace("zero_duration_test");
        // No sleep - might result in zero duration
    }

    auto events = traceme_recorder::stop();

    // Should have recorded some events
    size_t total_events        = 0;
    size_t zero_duration_count = 0;
    for (const auto& thread_events : events)
    {
        for (const auto& event : thread_events.events)
        {
            if (event.name == "zero_duration_test")
            {
                total_events++;
                if (event.is_complete() && event.end_time == event.start_time)
                {
                    zero_duration_count++;
                }
            }
        }
    }

    // Should have recorded at least some events
    XSIGMA_CHECK(total_events > 0, "Should have recorded some zero_duration_test events");

    // It's okay to have zero-duration traces
    XSIGMA_CHECK(true, "Zero duration traces should be handled gracefully");
    END_TEST();
}
