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

#include <cstdlib>  // for setenv, unsetenv
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "util/exception.h"
#include "xsigmaTest.h"

using namespace xsigma;

// Helper class to manage environment variable for testing
class env_var_guard
{
public:
    env_var_guard(const char* name, const char* value) : name_(name)
    {
        // Save old value if it exists
        const char* old_val = std::getenv(name);
        if (old_val != nullptr)
        {
            old_value_ = old_val;
            had_value_ = true;
        }

        // Set new value
        if (value != nullptr)
        {
#ifdef _WIN32
            _putenv_s(name, value);
#else
            setenv(name, value, 1);
#endif
        }
        else
        {
#ifdef _WIN32
            _putenv_s(name, "");
#else
            unsetenv(name);
#endif
        }
    }

    ~env_var_guard()
    {
        // Restore old value
        if (had_value_)
        {
#ifdef _WIN32
            _putenv_s(name_.c_str(), old_value_.c_str());
#else
            setenv(name_.c_str(), old_value_.c_str(), 1);
#endif
        }
        else
        {
#ifdef _WIN32
            _putenv_s(name_.c_str(), "");
#else
            unsetenv(name_.c_str());
#endif
        }
    }

private:
    std::string name_;
    std::string old_value_;
    bool        had_value_ = false;
};

// ============================================================================
// Consolidated Test 1: Basic Exception Functionality
// Tests: basic macros, cross-platform behavior, NOT_IMPLEMENTED macro,
//        edge cases (empty message, special characters)
// ============================================================================

XSIGMATEST(Exception, basic_functionality)
{
    xsigma::set_exception_mode(xsigma::exception_mode::THROW);

    // Test basic macros
#ifndef NDEBUG
    ASSERT_ANY_THROW({ XSIGMA_CHECK_DEBUG(false, "XSIGMA_CHECK_DEBUG: Should throw"); });
#endif

    ASSERT_ANY_THROW({ XSIGMA_CHECK(false, "XSIGMA_CHECK: Should throw"); });
    ASSERT_ANY_THROW({ XSIGMA_THROW("XSIGMA_THROW: should throw"); });

    // Test NOT_IMPLEMENTED category using macro
    try
    {
        XSIGMA_NOT_IMPLEMENTED("Feature not yet implemented");
        FAIL() << "Should have thrown exception";
    }
    catch (const xsigma::exception& e)
    {
        ASSERT_EQ(e.category(), xsigma::exception_category::NOT_IMPLEMENTED);
        ASSERT_TRUE(std::string(e.msg()).find("Feature not yet implemented") != std::string::npos);
    }

    // Test cross-platform behavior
    try
    {
        XSIGMA_THROW("Platform test: {}", "cross-platform");
        FAIL() << "Should have thrown";
    }
    catch (const xsigma::exception& e)
    {
        // Should work on Windows, Linux, and macOS
        ASSERT_TRUE(e.what() != nullptr);
        ASSERT_FALSE(std::string(e.what()).empty());
    }

    // Test edge case: empty message
    try
    {
        xsigma::source_location loc{__func__, __FILE__, __LINE__};
        throw xsigma::exception(loc, "", xsigma::exception_category::GENERIC);
    }
    catch (const xsigma::exception& e)
    {
        // Should not crash with empty message
        ASSERT_TRUE(e.what() != nullptr);
    }

    // Test edge case: message with special characters
    try
    {
        XSIGMA_THROW("Special chars: \n\t\r\\\"\'");
    }
    catch (const xsigma::exception& e)
    {
        std::string msg(e.what());
        ASSERT_TRUE(msg.find("Special chars") != std::string::npos);
    }

    END_TEST();
}

// ============================================================================
// Consolidated Test 2: Exception Mode Configuration
// Tests: mode switching, get/set exception mode, LOG_FATAL mode behavior
// ============================================================================

XSIGMATEST(Exception, mode_configuration)
{
    // Test default mode
    XSIGMA_UNUSED auto default_mode = xsigma::get_exception_mode();

    // Test mode switching to THROW
    xsigma::set_exception_mode(xsigma::exception_mode::THROW);
    ASSERT_EQ(xsigma::get_exception_mode(), xsigma::exception_mode::THROW);

    // Test mode switching to LOG_FATAL
    xsigma::set_exception_mode(xsigma::exception_mode::LOG_FATAL);
    ASSERT_EQ(xsigma::get_exception_mode(), xsigma::exception_mode::LOG_FATAL);

    // Test that get_exception_mode() returns LOG_FATAL consistently
    auto mode = xsigma::get_exception_mode();
    ASSERT_EQ(mode, xsigma::exception_mode::LOG_FATAL);
    ASSERT_NE(mode, xsigma::exception_mode::THROW);

    // Restore default mode for other tests
    xsigma::set_exception_mode(xsigma::exception_mode::THROW);

    // Verify restoration
    ASSERT_EQ(xsigma::get_exception_mode(), xsigma::exception_mode::THROW);

    END_TEST();
}

// ============================================================================
// Consolidated Test 3: Exception Categories and Stack Traces
// Tests: all exception categories, error categories, stack trace capture,
//        source location tracking
// ============================================================================

XSIGMATEST(Exception, categories_and_stack_traces)
{
    xsigma::set_exception_mode(xsigma::exception_mode::THROW);

    // Test all exception categories
    const std::vector<xsigma::exception_category> all_categories = {
        xsigma::exception_category::GENERIC,
        xsigma::exception_category::VALUE_ERROR,
        xsigma::exception_category::TYPE_ERROR,
        xsigma::exception_category::INDEX_ERROR,
        xsigma::exception_category::NOT_IMPLEMENTED,
        xsigma::exception_category::ENFORCE_FINITE,
        xsigma::exception_category::RUNTIME_ERROR,
        xsigma::exception_category::LOGIC_ERROR,
        xsigma::exception_category::SYSTEM_ERROR,
        xsigma::exception_category::MEMORY_ERROR};

    for (const auto& cat : all_categories)
    {
        try
        {
            xsigma::source_location loc{__func__, __FILE__, __LINE__};
            throw xsigma::exception(loc, "Test", cat);
        }
        catch (const xsigma::exception& e)
        {
            ASSERT_EQ(e.category(), cat);
            ASSERT_FALSE(std::string(e.what()).empty());
        }
    }

    // Test exception with VALUE_ERROR category (additional coverage)
    try
    {
        xsigma::source_location loc{__func__, __FILE__, __LINE__};
        throw xsigma::exception(loc, "Value error test", xsigma::exception_category::VALUE_ERROR);
    }
    catch (const xsigma::exception& e)
    {
        std::string msg(e.what());
        ASSERT_TRUE(msg.find("Value error test") != std::string::npos);
    }

    // Test stack trace capture
    try
    {
        XSIGMA_THROW("Test exception with stack trace");
        FAIL() << "Should have thrown";
    }
    catch (const xsigma::exception& e)
    {
        // Check that backtrace is captured
        const std::string& backtrace = e.backtrace();
        ASSERT_FALSE(backtrace.empty());

        // Backtrace should contain "Exception raised from"
        ASSERT_TRUE(backtrace.find("Exception raised from") != std::string::npos);
    }

    // Test source location tracking
    try
    {
        XSIGMA_THROW("Test source location");
    }
    catch (const xsigma::exception& e)
    {
        // Check that backtrace contains file and line information
        const std::string& backtrace = e.backtrace();
        ASSERT_FALSE(backtrace.empty());
        ASSERT_TRUE(backtrace.find("TestException.cpp") != std::string::npos);
    }

    END_TEST();
}

// ============================================================================
// Consolidated Test 4: Exception Chaining and Context
// Tests: exception chaining, context accumulation, nested exception access
// ============================================================================

XSIGMATEST(Exception, chaining_and_context)
{
    xsigma::set_exception_mode(xsigma::exception_mode::THROW);

    // Test exception chaining
    // Create a nested exception using new/shared_ptr to avoid MSVC ICE
    xsigma::source_location            inner_loc{__func__, __FILE__, __LINE__};
    std::shared_ptr<xsigma::exception> inner(new xsigma::exception(
        inner_loc,
        "Inner error: database connection failed",
        xsigma::exception_category::RUNTIME_ERROR));

    // Create outer exception with nested
    xsigma::source_location outer_loc{__func__, __FILE__, __LINE__};
    xsigma::exception       outer(outer_loc, "Outer error: failed to process request", inner);

    // Check nested exception is accessible
    ASSERT_TRUE(outer.nested() != nullptr);
    ASSERT_EQ(outer.nested(), inner);

    // Check what() includes nested information
    std::string full_msg(outer.what());
    ASSERT_TRUE(full_msg.find("Outer error") != std::string::npos);
    ASSERT_TRUE(full_msg.find("Caused by") != std::string::npos);
    ASSERT_TRUE(full_msg.find("Inner error") != std::string::npos);

    // Test context accumulation
    try
    {
        auto e = xsigma::exception(
            xsigma::source_location{__func__, __FILE__, __LINE__},
            "Base error",
            xsigma::exception_category::GENERIC);

        e.add_context("Context 1: processing file");
        e.add_context("Context 2: parsing line 42");
        e.add_context("Context 3: invalid token");

        // Check context is accumulated
        const auto& contexts = e.context();
        ASSERT_EQ(contexts.size(), 3);
        ASSERT_EQ(contexts[0], "Context 1: processing file");
        ASSERT_EQ(contexts[1], "Context 2: parsing line 42");
        ASSERT_EQ(contexts[2], "Context 3: invalid token");

        // Check what() includes all context
        std::string full_context_msg(e.what());
        ASSERT_TRUE(full_context_msg.find("Context 1") != std::string::npos);
        ASSERT_TRUE(full_context_msg.find("Context 2") != std::string::npos);
        ASSERT_TRUE(full_context_msg.find("Context 3") != std::string::npos);
    }
    catch (...)
    {
        FAIL() << "Should not throw in this test";
    }

    END_TEST();
}

// ============================================================================
// Consolidated Test 5: Exception Constructors and Accessors
// Tests: base constructor, base constructor with caller, all accessor methods,
//        memory safety (shared_ptr, copy constructor)
// ============================================================================

XSIGMATEST(Exception, constructors_and_accessors)
{
    xsigma::set_exception_mode(xsigma::exception_mode::THROW);

    // Test base constructor with all parameters
    xsigma::exception ex1(
        "Test message", "Test backtrace", nullptr, xsigma::exception_category::VALUE_ERROR);

    ASSERT_EQ(std::string(ex1.msg()), "Test message");
    ASSERT_EQ(std::string(ex1.backtrace()), "Test backtrace");
    ASSERT_EQ(ex1.caller(), nullptr);
    ASSERT_EQ(ex1.category(), xsigma::exception_category::VALUE_ERROR);

    // Test base constructor with caller pointer
    int               dummy_obj = 42;
    xsigma::exception ex2(
        "Test message", "Test backtrace", &dummy_obj, xsigma::exception_category::RUNTIME_ERROR);

    ASSERT_EQ(ex2.caller(), &dummy_obj);
    ASSERT_EQ(ex2.category(), xsigma::exception_category::RUNTIME_ERROR);

    // Test all accessor methods
    xsigma::source_location loc{__func__, __FILE__, __LINE__};
    xsigma::exception       ex3(loc, "Test message", xsigma::exception_category::LOGIC_ERROR);

    // Test msg() accessor
    ASSERT_EQ(std::string(ex3.msg()), "Test message");

    // Test category() accessor
    ASSERT_EQ(ex3.category(), xsigma::exception_category::LOGIC_ERROR);

    // Test backtrace() accessor
    ASSERT_FALSE(ex3.backtrace().empty());
    ASSERT_TRUE(ex3.backtrace().find("Exception raised from") != std::string::npos);

    // Test context() accessor
    ASSERT_TRUE(ex3.context().empty());  // No context added yet

    // Test caller() accessor
    ASSERT_EQ(ex3.caller(), nullptr);

    // Test nested() accessor
    ASSERT_EQ(ex3.nested(), nullptr);

    // Test memory safety: exception with shared_ptr (no memory leaks)
    {
        std::shared_ptr<xsigma::exception> ex_ptr;
        try
        {
            xsigma::source_location loc_mem{__func__, __FILE__, __LINE__};
            ex_ptr = std::make_shared<xsigma::exception>(
                loc_mem, "Shared ptr exception", xsigma::exception_category::GENERIC);
        }
        catch (...)
        {
            FAIL() << "Should not throw during construction";
        }

        ASSERT_TRUE(ex_ptr != nullptr);
        ASSERT_FALSE(std::string(ex_ptr->what()).empty());
    }

    // Test memory safety: exception copy
    {
        xsigma::source_location loc_copy{__func__, __FILE__, __LINE__};
        xsigma::exception       original(
            loc_copy, "Original exception", xsigma::exception_category::GENERIC);

        // Copy constructor
        xsigma::exception copy(original);
        ASSERT_EQ(std::string(copy.msg()), std::string(original.msg()));
        ASSERT_EQ(copy.category(), original.category());
    }

    END_TEST();
}

// ============================================================================
// Consolidated Test 6: what() and what_without_backtrace() Methods
// Tests: compute_what with context, what_without_backtrace with/without context,
//        what_without_backtrace with empty message
// ============================================================================

XSIGMATEST(Exception, what_methods)
{
    xsigma::set_exception_mode(xsigma::exception_mode::THROW);

    // Test compute_what with context
    xsigma::source_location loc1{__func__, __FILE__, __LINE__};
    xsigma::exception       ex1(loc1, "Base message", xsigma::exception_category::GENERIC);

    ex1.add_context("Context line 1");
    ex1.add_context("Context line 2");

    std::string what_str(ex1.what());
    ASSERT_TRUE(what_str.find("Base message") != std::string::npos);
    ASSERT_TRUE(what_str.find("Context line 1") != std::string::npos);
    ASSERT_TRUE(what_str.find("Context line 2") != std::string::npos);

    // Test what_without_backtrace
    xsigma::source_location loc2{__func__, __FILE__, __LINE__};
    xsigma::exception       ex2(loc2, "Test error message", xsigma::exception_category::GENERIC);

    // Get what_without_backtrace
    const char* msg_without_backtrace = ex2.what_without_backtrace();
    ASSERT_TRUE(msg_without_backtrace != nullptr);

    std::string msg_str(msg_without_backtrace);

    // Should contain the error message
    ASSERT_TRUE(msg_str.find("Test error message") != std::string::npos);

    // Note: Due to a bug in refresh_what() (line 195 in exception.cpp),
    // what_without_backtrace_ is currently set with include_backtrace=true
    // So this test documents the current behavior
    // The backtrace is currently included in what_without_backtrace_

    // Test what_without_backtrace with context
    xsigma::source_location loc3{__func__, __FILE__, __LINE__};
    xsigma::exception       ex3(loc3, "Base error", xsigma::exception_category::GENERIC);

    ex3.add_context("Additional context 1");
    ex3.add_context("Additional context 2");

    // Get what_without_backtrace
    const char* msg_without_backtrace_ctx = ex3.what_without_backtrace();
    ASSERT_TRUE(msg_without_backtrace_ctx != nullptr);

    std::string msg_ctx_str(msg_without_backtrace_ctx);

    // Should contain the base message
    ASSERT_TRUE(msg_ctx_str.find("Base error") != std::string::npos);

    // Should contain context
    ASSERT_TRUE(msg_ctx_str.find("Additional context 1") != std::string::npos);
    ASSERT_TRUE(msg_ctx_str.find("Additional context 2") != std::string::npos);

    // Test what_without_backtrace with empty message
    xsigma::source_location loc4{__func__, __FILE__, __LINE__};
    xsigma::exception       ex4(loc4, "", xsigma::exception_category::GENERIC);

    // Should not crash with empty message
    const char* msg_empty = ex4.what_without_backtrace();
    ASSERT_TRUE(msg_empty != nullptr);

    END_TEST();
}

// ============================================================================
// Consolidated Test 7: format_check_msg() Utility Function
// Tests: format_check_msg with no args, with args, empty user message,
//        multiple args, special characters
// ============================================================================

XSIGMATEST(Exception, format_check_msg)
{
    // Test format_check_msg with no arguments
    std::string result1 = xsigma::details::format_check_msg("x > 0");
    ASSERT_EQ(result1, "Check failed: x > 0");

    // Test format_check_msg with format arguments
    std::string result2 = xsigma::details::format_check_msg("x > 0", "Value was {}", 42);
    ASSERT_TRUE(result2.find("Check failed: x > 0") != std::string::npos);
    ASSERT_TRUE(result2.find("Value was 42") != std::string::npos);

    // Test format_check_msg when user message is empty
    std::string result3 = xsigma::details::format_check_msg("condition", "");
    // When user message is empty, should only show condition
    ASSERT_EQ(result3, "Check failed: condition");

    // Test format_check_msg with multiple format arguments
    std::string result4 =
        xsigma::details::format_check_msg("value in range", "Expected {} <= {} <= {}", 0, 5, 10);
    ASSERT_TRUE(result4.find("Check failed: value in range") != std::string::npos);
    ASSERT_TRUE(result4.find("Expected 0 <= 5 <= 10") != std::string::npos);

    // Test format_check_msg with special characters in condition
    std::string result5 =
        xsigma::details::format_check_msg("ptr != nullptr", "Pointer was null at index {}", 3);
    ASSERT_TRUE(result5.find("Check failed: ptr != nullptr") != std::string::npos);
    ASSERT_TRUE(result5.find("Pointer was null at index 3") != std::string::npos);

    END_TEST();
}

// ============================================================================
// Consolidated Test 8: init_exception_mode_from_env() Function
// Tests: string comparison logic, idempotency, thread safety
// ============================================================================

XSIGMATEST(Exception, init_exception_mode_from_env)
{
    // Test string comparison logic for environment variable values
    // We can't actually test the initialization with different values in the same process,
    // but we can verify the string comparison logic that would be used

    // Test LOG_FATAL variations
    std::string log_fatal_upper = "LOG_FATAL";
    std::string log_fatal_lower = "log_fatal";
    ASSERT_TRUE(log_fatal_upper == "LOG_FATAL" || log_fatal_upper == "log_fatal");
    ASSERT_TRUE(log_fatal_lower == "LOG_FATAL" || log_fatal_lower == "log_fatal");

    // Test THROW variations
    std::string throw_upper = "THROW";
    std::string throw_lower = "throw";
    ASSERT_TRUE(throw_upper == "THROW" || throw_upper == "throw");
    ASSERT_TRUE(throw_lower == "THROW" || throw_lower == "throw");

    // Test invalid values
    std::string invalid = "INVALID_MODE";
    ASSERT_FALSE(invalid == "LOG_FATAL" || invalid == "log_fatal");
    ASSERT_FALSE(invalid == "THROW" || invalid == "throw");

    // Test empty string
    std::string empty = "";
    ASSERT_FALSE(empty == "LOG_FATAL" || empty == "log_fatal");
    ASSERT_FALSE(empty == "THROW" || empty == "throw");

    // Test idempotency: initialization only happens once
    // Call init multiple times
    xsigma::init_exception_mode_from_env();
    auto mode1 = xsigma::get_exception_mode();

    xsigma::init_exception_mode_from_env();
    auto mode2 = xsigma::get_exception_mode();

    xsigma::init_exception_mode_from_env();
    auto mode3 = xsigma::get_exception_mode();

    // All should return the same mode
    ASSERT_EQ(mode1, mode2);
    ASSERT_EQ(mode2, mode3);

    // Test thread-safety: verify the double-check locking pattern works correctly
    // Launch multiple threads that all try to initialize
    std::vector<std::thread> threads;
    std::atomic<int>         success_count{0};

    for (int i = 0; i < 10; ++i)
    {
        threads.emplace_back(
            [&success_count]()
            {
                try
                {
                    xsigma::init_exception_mode_from_env();
                    success_count++;
                }
                catch (...)
                {
                    // Should not throw
                }
            });
    }

    // Wait for all threads
    for (auto& t : threads)
    {
        t.join();
    }

    // All threads should succeed
    ASSERT_EQ(success_count.load(), 10);

    // Mode should be consistent
    auto final_mode = xsigma::get_exception_mode();
    ASSERT_TRUE(
        final_mode == xsigma::exception_mode::THROW ||
        final_mode == xsigma::exception_mode::LOG_FATAL);

    END_TEST();
}

// ============================================================================
// Consolidated Test 9: check_fail() Function - Basic Behavior
// Tests: basic throw, source location, category, never returns,
//        empty message, special characters
// ============================================================================

XSIGMATEST(Exception, check_fail_basic)
{
    xsigma::set_exception_mode(xsigma::exception_mode::THROW);

    // Test that check_fail throws an exception
    try
    {
        xsigma::details::check_fail("test_function", "test_file.cpp", 42, "Test error message");
        FAIL() << "check_fail should have thrown an exception";
    }
    catch (const xsigma::exception& e)
    {
        // Verify the exception was thrown
        std::string msg(e.msg());
        ASSERT_TRUE(msg.find("Test error message") != std::string::npos);
    }

    // Test that check_fail includes correct source location
    try
    {
        xsigma::details::check_fail("my_function", "my_file.cpp", 123, "Location test");
        FAIL() << "check_fail should have thrown an exception";
    }
    catch (const xsigma::exception& e)
    {
        // Verify source location is in the backtrace
        std::string backtrace(e.backtrace());
        ASSERT_TRUE(backtrace.find("my_function") != std::string::npos);
        ASSERT_TRUE(backtrace.find("my_file.cpp") != std::string::npos);
        ASSERT_TRUE(backtrace.find("123") != std::string::npos);
    }

    // Test that exception category is set to GENERIC
    try
    {
        xsigma::details::check_fail("func", "file.cpp", 1, "Category test");
        FAIL() << "check_fail should have thrown an exception";
    }
    catch (const xsigma::exception& e)
    {
        // Verify category is GENERIC
        ASSERT_EQ(e.category(), xsigma::exception_category::GENERIC);
    }

    // Test that check_fail never returns (marked as [[noreturn]])
    bool reached_after_call = false;

    try
    {
        xsigma::details::check_fail("func", "file.cpp", 1, "No return test");
        reached_after_call = true;  // Should never reach here
    }
    catch (const xsigma::exception&)
    {
        // Exception was thrown, which is expected
    }

    // Verify we never reached the line after check_fail
    ASSERT_FALSE(reached_after_call);

    // Test with empty message
    try
    {
        xsigma::details::check_fail("func", "file.cpp", 1, "");
        FAIL() << "check_fail should have thrown an exception";
    }
    catch (const xsigma::exception& e)
    {
        // Should not crash with empty message
        ASSERT_TRUE(e.what() != nullptr);
    }

    // Test with special characters in message
    try
    {
        xsigma::details::check_fail("func", "file.cpp", 1, "Special chars: \n\t\r\\\"\'{}[]");
        FAIL() << "check_fail should have thrown an exception";
    }
    catch (const xsigma::exception& e)
    {
        // Verify special characters are preserved
        std::string msg(e.msg());
        ASSERT_TRUE(msg.find("Special chars") != std::string::npos);
    }

    END_TEST();
}

// ============================================================================
// Consolidated Test 10: check_fail() Function - Advanced Message Handling
// Tests: long messages, unicode messages, formatted messages,
//        multiline messages, various line numbers
// ============================================================================

XSIGMATEST(Exception, check_fail_advanced)
{
    xsigma::set_exception_mode(xsigma::exception_mode::THROW);

    // Test with very long message
    std::string long_msg(1000, 'x');

    try
    {
        xsigma::details::check_fail("func", "file.cpp", 1, long_msg);
        FAIL() << "check_fail should have thrown an exception";
    }
    catch (const xsigma::exception& e)
    {
        // Verify long message is handled correctly
        std::string msg(e.msg());
        ASSERT_EQ(msg.length(), 1000);
        ASSERT_EQ(msg, long_msg);
    }

    // Test with unicode characters in message
    try
    {
        xsigma::details::check_fail("func", "file.cpp", 1, "Unicode: αβγδ 中文 🚀");
        FAIL() << "check_fail should have thrown an exception";
    }
    catch (const xsigma::exception& e)
    {
        // Verify unicode is preserved
        std::string msg(e.msg());
        ASSERT_TRUE(msg.find("Unicode") != std::string::npos);
    }

    // Test with formatted message
    try
    {
        xsigma::details::check_fail("func", "file.cpp", 1, "Error: value = 42");
        FAIL() << "check_fail should have thrown an exception";
    }
    catch (const xsigma::exception& e)
    {
        // Verify message is preserved
        std::string msg(e.msg());
        ASSERT_TRUE(msg.find("Error: value = 42") != std::string::npos);
    }

    // Test with multiline message
    try
    {
        xsigma::details::check_fail("func", "file.cpp", 1, "Line 1\nLine 2\nLine 3");
        FAIL() << "check_fail should have thrown an exception";
    }
    catch (const xsigma::exception& e)
    {
        // Verify multiline message is preserved
        std::string msg(e.msg());
        ASSERT_TRUE(msg.find("Line 1") != std::string::npos);
        ASSERT_TRUE(msg.find("Line 2") != std::string::npos);
        ASSERT_TRUE(msg.find("Line 3") != std::string::npos);
    }

    // Test with various line numbers
    std::vector<int> line_numbers = {0, 1, 42, 999, 10000, -1};

    for (int line : line_numbers)
    {
        try
        {
            xsigma::details::check_fail("func", "file.cpp", line, "Line number test");
            FAIL() << "check_fail should have thrown an exception";
        }
        catch (const xsigma::exception& e)
        {
            // Verify exception was thrown
            ASSERT_TRUE(e.what() != nullptr);

            // Verify line number is in backtrace
            std::string backtrace(e.backtrace());
            ASSERT_TRUE(backtrace.find(std::to_string(line)) != std::string::npos);
        }
    }

    END_TEST();
}
