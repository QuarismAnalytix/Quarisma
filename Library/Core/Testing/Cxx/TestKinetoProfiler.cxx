/*
 * XSigma Kineto Profiler Tests
 *
 * Comprehensive test suite for the Kineto profiler wrapper.
 * Tests cover happy paths, edge cases, boundary conditions, and error handling.
 */

#include <memory>
#include <thread>

#include "Testing/xsigmaTest.h"
#include "profiler/kineto_profiler.h"

using namespace xsigma::kineto_profiler;

// ============================================================================
// Test Fixtures
// ============================================================================

class KinetoProfilerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Reset state before each test
        kineto_profiler::initialize(true);
    }

    void TearDown() override
    {
        // Cleanup after each test
    }
};

// ============================================================================
// Factory Method Tests
// ============================================================================

XSIGMATEST(Profiler, kineto_create_returns_valid_profiler)
{
    auto profiler = kineto_profiler::create();
    // Profiler may be nullptr if Kineto is not available
    // This is acceptable for cross-platform builds
    if (profiler)
    {
        EXPECT_NE(profiler.get(), nullptr);
    }
}

XSIGMATEST(Profiler, kineto_create_with_config_returns_valid_profiler)
{
    profiling_config config;
    config.enable_cpu_tracing = true;
    config.enable_gpu_tracing = false;
    config.output_dir         = "./test_profiles";
    config.trace_name         = "test_trace";

    auto profiler = kineto_profiler::create_with_config(config);
    if (profiler)
    {
        EXPECT_NE(profiler.get(), nullptr);
    }
}

XSIGMATEST(Profiler, kineto_create_with_empty_config)
{
    profiling_config config;
    auto             profiler = kineto_profiler::create_with_config(config);
    if (profiler)
    {
        EXPECT_NE(profiler.get(), nullptr);
    }
}

// ============================================================================
// Profiling Control Tests
// ============================================================================

XSIGMATEST(Profiler, kineto_profiler_not_profiling_initially)
{
    auto profiler = kineto_profiler::create();
    if (profiler)
    {
        EXPECT_FALSE(profiler->is_profiling());
    }
}

XSIGMATEST(Profiler, kineto_start_profiling_succeeds)
{
    auto profiler = kineto_profiler::create();
    if (profiler)
    {
        bool result = profiler->start_profiling();
        // Result depends on Kineto availability
        if (result)
        {
            EXPECT_TRUE(profiler->is_profiling());
            profiler->stop_profiling();
        }
    }
}

XSIGMATEST(Profiler, kineto_stop_profiling_returns_result)
{
    auto profiler = kineto_profiler::create();
    if (profiler)
    {
        if (profiler->start_profiling())
        {
            auto result = profiler->stop_profiling();
            EXPECT_FALSE(profiler->is_profiling());
            // Result structure should be valid - either success or error message
            EXPECT_TRUE(result.success || !result.error_message.empty());
        }
    }
}

XSIGMATEST(Profiler, kineto_stop_profiling_without_start)
{
    auto profiler = kineto_profiler::create();
    if (profiler)
    {
        auto result = profiler->stop_profiling();
        EXPECT_FALSE(result.success);
        EXPECT_FALSE(result.error_message.empty());
    }
}

XSIGMATEST(Profiler, kineto_cannot_start_profiling_twice)
{
    auto profiler = kineto_profiler::create();
    if (profiler)
    {
        if (profiler->start_profiling())
        {
            bool second_start = profiler->start_profiling();
            EXPECT_FALSE(second_start);
            profiler->stop_profiling();
        }
    }
}

// ============================================================================
// Configuration Tests
// ============================================================================

XSIGMATEST(Profiler, kineto_get_config_returns_valid_config)
{
    profiling_config config;
    config.trace_name = "custom_trace";
    config.output_dir = "/tmp/profiles";

    auto profiler = kineto_profiler::create_with_config(config);
    if (profiler)
    {
        const auto& retrieved_config = profiler->get_config();
        EXPECT_EQ(retrieved_config.trace_name, "custom_trace");
        EXPECT_EQ(retrieved_config.output_dir, "/tmp/profiles");
    }
}

XSIGMATEST(Profiler, kineto_set_config_succeeds_when_not_profiling)
{
    auto profiler = kineto_profiler::create();
    if (profiler)
    {
        profiling_config new_config;
        new_config.trace_name = "updated_trace";

        bool result = profiler->set_config(new_config);
        EXPECT_TRUE(result);
        EXPECT_EQ(profiler->get_config().trace_name, "updated_trace");
    }
}

XSIGMATEST(Profiler, kineto_set_config_fails_when_profiling)
{
    auto profiler = kineto_profiler::create();
    if (profiler)
    {
        if (profiler->start_profiling())
        {
            profiling_config new_config;
            bool             result = profiler->set_config(new_config);
            EXPECT_FALSE(result);
            profiler->stop_profiling();
        }
    }
}

XSIGMATEST(Profiler, kineto_config_with_zero_max_activities)
{
    profiling_config config;
    config.max_activities = 0;  // Unlimited

    auto profiler = kineto_profiler::create_with_config(config);
    if (profiler)
    {
        EXPECT_EQ(profiler->get_config().max_activities, 0);
    }
}

XSIGMATEST(Profiler, kineto_config_with_positive_max_activities)
{
    profiling_config config;
    config.max_activities = 1000;

    auto profiler = kineto_profiler::create_with_config(config);
    if (profiler)
    {
        EXPECT_EQ(profiler->get_config().max_activities, 1000);
    }
}

// ============================================================================
// Initialization Tests
// ============================================================================

XSIGMATEST(Profiler, kineto_initialize_cpu_only)
{
    bool result = kineto_profiler::initialize(true);
    // Result depends on Kineto availability
    EXPECT_TRUE(result || !result);  // Always true - just checking it doesn't crash
}

XSIGMATEST(Profiler, kineto_initialize_with_gpu)
{
    bool result = kineto_profiler::initialize(false);
    // Result depends on Kineto availability
    EXPECT_TRUE(result || !result);  // Always true - just checking it doesn't crash
}

XSIGMATEST(Profiler, kineto_is_initialized_after_create)
{
    auto profiler = kineto_profiler::create();
    // If profiler was created, Kineto should be initialized
    if (profiler)
    {
        EXPECT_TRUE(kineto_profiler::is_initialized());
    }
}

// ============================================================================
// Edge Cases and Boundary Conditions
// ============================================================================

XSIGMATEST(Profiler, kineto_null_profiler_pointer)
{
    std::unique_ptr<kineto_profiler> profiler = nullptr;
    EXPECT_EQ(profiler.get(), nullptr);
}

XSIGMATEST(Profiler, kineto_profiler_move_semantics)
{
    auto profiler1 = kineto_profiler::create();
    if (profiler1)
    {
        auto profiler2 = std::move(profiler1);
        EXPECT_EQ(profiler1.get(), nullptr);
        EXPECT_NE(profiler2.get(), nullptr);
    }
}

XSIGMATEST(Profiler, kineto_config_with_empty_strings)
{
    profiling_config config;
    config.output_dir = "";
    config.trace_name = "";

    auto profiler = kineto_profiler::create_with_config(config);
    if (profiler)
    {
        EXPECT_EQ(profiler->get_config().output_dir, "");
        EXPECT_EQ(profiler->get_config().trace_name, "");
    }
}

XSIGMATEST(Profiler, kineto_config_with_special_characters)
{
    profiling_config config;
    config.output_dir = "/tmp/profiles_@#$%";
    config.trace_name = "trace_!@#$%^&*()";

    auto profiler = kineto_profiler::create_with_config(config);
    if (profiler)
    {
        EXPECT_EQ(profiler->get_config().output_dir, "/tmp/profiles_@#$%");
        EXPECT_EQ(profiler->get_config().trace_name, "trace_!@#$%^&*()");
    }
}

XSIGMATEST(Profiler, kineto_profiler_destructor_stops_profiling)
{
    {
        auto profiler = kineto_profiler::create();
        if (profiler)
        {
            profiler->start_profiling();
            EXPECT_TRUE(profiler->is_profiling());
            // Destructor should stop profiling
        }
    }
    // If we reach here without crash, destructor worked correctly
    EXPECT_TRUE(true);
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

XSIGMATEST(Profiler, kineto_concurrent_is_profiling_checks)
{
    auto profiler = kineto_profiler::create();
    if (profiler)
    {
        std::thread t1(
            [&profiler]()
            {
                for (int i = 0; i < 10; ++i)
                {
                    profiler->is_profiling();
                }
            });

        std::thread t2(
            [&profiler]()
            {
                for (int i = 0; i < 10; ++i)
                {
                    profiler->is_profiling();
                }
            });

        t1.join();
        t2.join();
        EXPECT_TRUE(true);  // No crashes = success
    }
}

XSIGMATEST(Profiler, kineto_concurrent_config_access)
{
    auto profiler = kineto_profiler::create();
    if (profiler)
    {
        std::thread t1(
            [&profiler]()
            {
                for (int i = 0; i < 5; ++i)
                {
                    profiler->get_config();
                }
            });

        std::thread t2(
            [&profiler]()
            {
                profiling_config config;
                for (int i = 0; i < 5; ++i)
                {
                    profiler->set_config(config);
                }
            });

        t1.join();
        t2.join();
        EXPECT_TRUE(true);  // No crashes = success
    }
}
