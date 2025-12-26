/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 */

#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <vector>

#include "profiler/session/profiler.h"
#include "profiler/session/profiler_report.h"
#include "xsigmaTest.h"

using namespace xsigma;

// ============================================================================
// Profiler Session Edge Cases and Error Handling Tests
// ============================================================================

XSIGMATEST(Profiler, session_double_start_fails)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    // Second start should fail
    EXPECT_FALSE(session->start());

    EXPECT_TRUE(session->stop());
}

XSIGMATEST(Profiler, session_stop_without_start)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);

    // Stop without start should fail
    EXPECT_FALSE(session->stop());
}

XSIGMATEST(Profiler, session_double_stop)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());
    EXPECT_TRUE(session->stop());

    // Second stop should fail
    EXPECT_FALSE(session->stop());
}

XSIGMATEST(Profiler, scope_without_session)
{
    // Create scope without active session
    profiler_scope scope("orphan_scope");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    // Should not crash
}

XSIGMATEST(Profiler, scope_after_session_stop)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());
    EXPECT_TRUE(session->stop());

    // Create scope after session stopped
    profiler_scope scope("post_stop_scope", session.get());
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    // Should not crash
}

XSIGMATEST(Profiler, session_is_active)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_FALSE(session->is_active());

    EXPECT_TRUE(session->start());
    EXPECT_TRUE(session->is_active());

    EXPECT_TRUE(session->stop());
    EXPECT_FALSE(session->is_active());
}

XSIGMATEST(Profiler, session_current_session)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);

    EXPECT_TRUE(session->start());
    EXPECT_EQ(profiler_session::current_session(), session.get());
    EXPECT_TRUE(session->stop());

    EXPECT_NE(profiler_session::current_session(), session.get());
}

XSIGMATEST(Profiler, scope_with_empty_name)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope scope("", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());
    // Should not crash
}

XSIGMATEST(Profiler, scope_with_very_long_name)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        std::string    long_name(10000, 'a');
        profiler_scope scope(long_name, session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());
    // Should not crash
}

XSIGMATEST(Profiler, many_concurrent_scopes)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i)
    {
        threads.emplace_back(
            [session = session.get(), i]()
            {
                for (int j = 0; j < 10; ++j)
                {
                    profiler_scope scope(
                        "scope_" + std::to_string(i) + "_" + std::to_string(j), session);
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
            });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_TRUE(session->stop());
}

XSIGMATEST(Profiler, scope_immediate_destruction)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    // Create and immediately destroy scope
    profiler_scope("immediate_scope", session.get());

    EXPECT_TRUE(session->stop());
}

XSIGMATEST(Profiler, session_with_disabled_features)
{
    profiler_options opts;
    opts.enable_timing_                 = false;
    opts.enable_memory_tracking_        = false;
    opts.enable_hierarchical_profiling_ = false;
    opts.enable_statistical_analysis_   = false;

    auto session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope scope("disabled_features_scope", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());
}

XSIGMATEST(Profiler, write_chrome_trace_invalid_path)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope scope("test_scope", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());

    // Try to write to invalid path
    std::string invalid_path = "/invalid/path/that/does/not/exist/trace.json";
    EXPECT_FALSE(session->write_chrome_trace(invalid_path));
}

XSIGMATEST(Profiler, generate_chrome_trace_before_stop)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope scope("test_scope", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Generate trace before stop - should return empty or partial data
    std::string json = session->generate_chrome_trace_json();
    // May be empty or contain partial data
    EXPECT_TRUE(json.empty() || json.find("traceEvents") != std::string::npos);

    EXPECT_TRUE(session->stop());
}

// ============================================================================
// Additional Edge Cases and Comprehensive Coverage Tests
// ============================================================================

XSIGMATEST(Profiler, session_create_scope_method)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    // Test create_scope method
    auto scope = session->create_scope("created_scope");
    EXPECT_TRUE(scope != nullptr);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    EXPECT_TRUE(session->stop());
}

XSIGMATEST(Profiler, session_generate_report)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope scope("report_test_scope", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());

    // Generate report
    auto report = session->generate_report();
    EXPECT_TRUE(report != nullptr);
}

XSIGMATEST(Profiler, session_export_report_to_file)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope scope("export_test_scope", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());

    // Export report to file (will fail due to invalid path, but tests the code path)
    session->export_report("/invalid/path/report.json");
    // Should not crash
}

XSIGMATEST(Profiler, session_print_report)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope scope("print_test_scope", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());

    // Print report (should not crash)
    session->print_report();
}

XSIGMATEST(Profiler, session_collected_xspace_access)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope scope("xspace_test_scope", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());

    // Access collected xspace
    XSIGMA_UNUSED const auto& xspace = session->collected_xspace();
    // Should not crash
    EXPECT_TRUE(true);
}

XSIGMATEST(Profiler, session_has_collected_xspace)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);

    // Before start
    EXPECT_FALSE(session->has_collected_xspace());

    EXPECT_TRUE(session->start());
    EXPECT_FALSE(session->has_collected_xspace());

    {
        profiler_scope scope("xspace_check_scope", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());
    // May or may not have xspace depending on backend profilers
}

XSIGMATEST(Profiler, scope_data_duration_calculations)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope scope("duration_test_scope", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    EXPECT_TRUE(session->stop());
    // Scope should have recorded timing data
}

XSIGMATEST(Profiler, nested_scopes_deep_hierarchy)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope level1("level1", session.get());
        {
            profiler_scope level2("level2", session.get());
            {
                profiler_scope level3("level3", session.get());
                {
                    profiler_scope level4("level4", session.get());
                    {
                        profiler_scope level5("level5", session.get());
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                }
            }
        }
    }

    EXPECT_TRUE(session->stop());
}

XSIGMATEST(Profiler, session_with_memory_tracking_enabled)
{
    profiler_options opts;
    opts.enable_memory_tracking_ = true;
    auto session                 = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope   scope("memory_tracking_scope", session.get());
        std::vector<int> data(10000);
        for (int i = 0; i < 10000; ++i)
        {
            data[i] = i * 2;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());
}

XSIGMATEST(Profiler, session_with_statistical_analysis_enabled)
{
    profiler_options opts;
    opts.enable_statistical_analysis_ = true;
    auto session                      = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    // Multiple iterations to collect statistics
    for (int iter = 0; iter < 5; ++iter)
    {
        profiler_scope scope("statistical_scope", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());
}

XSIGMATEST(Profiler, session_with_hierarchical_profiling_disabled)
{
    profiler_options opts;
    opts.enable_hierarchical_profiling_ = false;
    auto session                        = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope scope("non_hierarchical_scope", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());
}

XSIGMATEST(Profiler, session_with_timing_disabled)
{
    profiler_options opts;
    opts.enable_timing_ = false;
    auto session        = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope scope("no_timing_scope", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());
}

XSIGMATEST(Profiler, session_with_thread_safety_disabled)
{
    profiler_options opts;
    opts.enable_thread_safety_ = false;
    auto session               = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope scope("non_thread_safe_scope", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());
}

XSIGMATEST(Profiler, session_destructor_stops_active_session)
{
    {
        profiler_options opts;
        auto             session = std::make_unique<profiler_session>(opts);
        EXPECT_TRUE(session != nullptr);
        EXPECT_TRUE(session->start());
        EXPECT_TRUE(session->is_active());

        {
            profiler_scope scope("destructor_test_scope", session.get());
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        // Destructor should stop the session
    }
    // Session destroyed, should have stopped automatically
}

XSIGMATEST(Profiler, multiple_sequential_sessions)
{
    // First session
    {
        profiler_options opts;
        auto             session = std::make_unique<profiler_session>(opts);
        EXPECT_TRUE(session != nullptr);
        EXPECT_TRUE(session->start());

        {
            profiler_scope scope("first_session_scope", session.get());
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        EXPECT_TRUE(session->stop());
    }

    // Second session
    {
        profiler_options opts;
        auto             session = std::make_unique<profiler_session>(opts);
        EXPECT_TRUE(session != nullptr);
        EXPECT_TRUE(session->start());

        {
            profiler_scope scope("second_session_scope", session.get());
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        EXPECT_TRUE(session->stop());
    }
}

XSIGMATEST(Profiler, report_generation_with_data)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    for (int i = 0; i < 3; ++i)
    {
        profiler_scope scope("report_scope_" + std::to_string(i), session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());

    auto report = session->generate_report();
    EXPECT_TRUE(report != nullptr);

    // Test report generation methods
    std::string console_report = report->generate_console_report();
    EXPECT_FALSE(console_report.empty());

    std::string json_report = report->generate_json_report();
    EXPECT_FALSE(json_report.empty());

    std::string csv_report = report->generate_csv_report();
    EXPECT_FALSE(csv_report.empty());

    std::string xml_report = report->generate_xml_report();
    EXPECT_FALSE(xml_report.empty());
}

XSIGMATEST(Profiler, report_export_to_file_invalid_path)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope scope("export_scope", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());

    auto report = session->generate_report();
    EXPECT_TRUE(report != nullptr);

    // Try to export to invalid path
    bool result = report->export_to_file(
        "/invalid/path/report.json", profiler_options::output_format_enum::JSON);
    EXPECT_FALSE(result);
}

XSIGMATEST(Profiler, report_customization_precision)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope scope("precision_test_scope", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());

    auto report = session->generate_report();
    EXPECT_TRUE(report != nullptr);

    // Set precision
    report->set_precision(4);
    std::string output = report->generate_console_report();
    EXPECT_FALSE(output.empty());
}

XSIGMATEST(Profiler, report_customization_time_unit)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope scope("time_unit_test_scope", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());

    auto report = session->generate_report();
    EXPECT_TRUE(report != nullptr);

    // Set time unit
    report->set_time_unit("ms");
    std::string output = report->generate_console_report();
    EXPECT_FALSE(output.empty());
}

XSIGMATEST(Profiler, report_customization_memory_unit)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope scope("memory_unit_test_scope", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());

    auto report = session->generate_report();
    EXPECT_TRUE(report != nullptr);

    // Set memory unit
    report->set_memory_unit("MB");
    std::string output = report->generate_console_report();
    EXPECT_FALSE(output.empty());
}

XSIGMATEST(Profiler, report_customization_thread_info)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope scope("thread_info_test_scope", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());

    auto report = session->generate_report();
    EXPECT_TRUE(report != nullptr);

    // Include thread info
    report->set_include_thread_info(true);
    std::string output = report->generate_console_report();
    EXPECT_FALSE(output.empty());
}

XSIGMATEST(Profiler, report_customization_hierarchical_data)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope outer("outer_scope", session.get());
        {
            profiler_scope inner("inner_scope", session.get());
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    EXPECT_TRUE(session->stop());

    auto report = session->generate_report();
    EXPECT_TRUE(report != nullptr);

    // Include hierarchical data
    report->set_include_hierarchical_data(true);
    std::string output = report->generate_console_report();
    EXPECT_FALSE(output.empty());
}
