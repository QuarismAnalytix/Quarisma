/*
 * XSigma Profiler API Tests
 *
 * Tests for the high-level XSigma profiler API including:
 * - ProfilerSession singleton
 * - ProfilerGuard RAII behavior
 * - RecordFunction scope tracking
 * - Profiler state transitions
 * - Configuration management
 */

#include "xsigmaTest.h"

#if XSIGMA_HAS_PROFILER

#include <chrono>
#include <thread>

#include "profiler/profiler_api.h"
#include "profiler/profiler_guard.h"

using namespace xsigma::profiler;

// ============================================================================
// Test Fixture for ProfilerSession Tests
// ============================================================================

class ProfilerSessionTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Reset profiler to Disabled state before each test
        auto& session = ProfilerSession::instance();
        if (session.is_profiling())
        {
            session.stop();
        }
        session.reset();
    }

    void TearDown() override
    {
        // Cleanup after each test
        auto& session = ProfilerSession::instance();
        if (session.is_profiling())
        {
            session.stop();
        }
        session.reset();
    }
};

// ============================================================================
// ProfilerSession Tests
// ============================================================================

TEST_F(ProfilerSessionTest, SingletonInstance)
{
    // Test that ProfilerSession returns the same instance
    auto& session1 = ProfilerSession::instance();
    auto& session2 = ProfilerSession::instance();

    EXPECT_EQ(&session1, &session2);
}

TEST_F(ProfilerSessionTest, InitialState)
{
    // Test that profiler starts in Disabled or Ready state
    auto&         session = ProfilerSession::instance();
    ProfilerState state   = get_profiler_state();

    // After cleanup, profiler should be in Disabled or Ready state
    EXPECT_TRUE(state == ProfilerState::Disabled || state == ProfilerState::Ready);
}

TEST_F(ProfilerSessionTest, StartProfiler)
{
    // Test starting the profiler
    auto& session = ProfilerSession::instance();

    ProfilerConfig config;
    config.activities = {ActivityType::CPU};
    config.verbose    = false;

    bool started = session.start(config);
    EXPECT_TRUE(started);

    ProfilerState state = get_profiler_state();
    EXPECT_EQ(state, ProfilerState::Recording);
}

TEST_F(ProfilerSessionTest, StopProfiler)
{
    // Test stopping the profiler
    auto& session = ProfilerSession::instance();

    ProfilerConfig config;
    config.activities = {ActivityType::CPU};

    session.start(config);
    bool stopped = session.stop();

    EXPECT_TRUE(stopped);

    ProfilerState state = get_profiler_state();
    EXPECT_EQ(state, ProfilerState::Ready);
}

TEST_F(ProfilerSessionTest, GetProfilerConfig)
{
    // Test retrieving profiler configuration
    auto& session = ProfilerSession::instance();

    ProfilerConfig config;
    config.activities = {ActivityType::CPU, ActivityType::CUDA};
    config.verbose    = true;

    session.start(config);

    const ProfilerConfig& retrieved_config = get_profiler_config();
    EXPECT_EQ(retrieved_config.verbose, true);
}

// ============================================================================
// ProfilerGuard Tests
// ============================================================================

TEST_F(ProfilerSessionTest, ProfilerGuardRAII)
{
    // Test that ProfilerGuard starts profiler on construction
    // and stops on destruction
    {
        ProfilerConfig config;
        config.activities = {ActivityType::CPU};

        ProfilerGuard guard(config);

        ProfilerState state = get_profiler_state();
        EXPECT_EQ(state, ProfilerState::Recording);
    }

    // After guard destruction, profiler should be stopped
    ProfilerState state = get_profiler_state();
    EXPECT_EQ(state, ProfilerState::Ready);
}

TEST_F(ProfilerSessionTest, ProfilerGuardMultiple)
{
    // Test multiple ProfilerGuard instances
    ProfilerConfig config;
    config.activities = {ActivityType::CPU};

    {
        ProfilerGuard guard1(config);
        EXPECT_EQ(get_profiler_state(), ProfilerState::Recording);

        {
            ProfilerGuard guard2(config);
            EXPECT_EQ(get_profiler_state(), ProfilerState::Recording);
        }

        // After inner guard, profiler should still be recording
        EXPECT_EQ(get_profiler_state(), ProfilerState::Recording);
    }

    // After all guards, profiler should be stopped
    EXPECT_EQ(get_profiler_state(), ProfilerState::Ready);
}

// ============================================================================
// RecordFunction Tests
// ============================================================================

TEST_F(ProfilerSessionTest, RecordFunctionScope)
{
    // Test RecordFunction scope tracking
    ProfilerConfig config;
    config.activities = {ActivityType::CPU};

    auto& session = ProfilerSession::instance();
    session.start(config);

    {
        RecordFunction record("test_function");
        // Function is being recorded
        EXPECT_TRUE(profiler_enabled());
    }
}

TEST_F(ProfilerSessionTest, RecordFunctionNested)
{
    // Test nested RecordFunction scopes
    ProfilerConfig config;
    config.activities = {ActivityType::CPU};

    auto& session = ProfilerSession::instance();
    session.start(config);

    {
        RecordFunction outer("outer_function");
        {
            RecordFunction inner("inner_function");
            EXPECT_TRUE(profiler_enabled());
        }
        EXPECT_TRUE(profiler_enabled());
    }
}

// ============================================================================
// ScopedActivity Tests
// ============================================================================

TEST_F(ProfilerSessionTest, ScopedActivityTracking)
{
    // Test ScopedActivity for tracking activities
    ProfilerConfig config;
    config.activities = {ActivityType::CPU};

    auto& session = ProfilerSession::instance();
    session.start(config);

    {
        ScopedActivity activity("test_activity");
        EXPECT_TRUE(profiler_enabled());
    }
}

// ============================================================================
// Configuration Tests
// ============================================================================

TEST_F(ProfilerSessionTest, ConfigurationActivities)
{
    // Test configuration with multiple activity types
    ProfilerConfig config;
    config.activities = {ActivityType::CPU, ActivityType::CUDA, ActivityType::Memory};
    config.verbose    = false;

    auto& session = ProfilerSession::instance();
    bool  started = session.start(config);

    EXPECT_TRUE(started);
    EXPECT_EQ(get_profiler_state(), ProfilerState::Recording);
}

TEST_F(ProfilerSessionTest, ConfigurationVerbose)
{
    // Test verbose configuration
    ProfilerConfig config;
    config.activities = {ActivityType::CPU};
    config.verbose    = true;

    auto& session = ProfilerSession::instance();
    session.start(config);

    const ProfilerConfig& retrieved = get_profiler_config();
    EXPECT_EQ(retrieved.verbose, true);
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_F(ProfilerSessionTest, ThreadSafety)
{
    // Test that profiler is thread-safe
    ProfilerConfig config;
    config.activities = {ActivityType::CPU};

    auto& session = ProfilerSession::instance();
    session.start(config);

    std::thread t1(
        [&]()
        {
            RecordFunction record("thread1_function");
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        });

    std::thread t2(
        [&]()
        {
            RecordFunction record("thread2_function");
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        });

    t1.join();
    t2.join();
}

// ============================================================================
// State Transition Tests
// ============================================================================

TEST_F(ProfilerSessionTest, StateTransitions)
{
    // Test valid state transitions: Disabled/Ready -> Recording -> Ready
    auto& session = ProfilerSession::instance();

    // Initial state (after cleanup)
    ProfilerState initial_state = get_profiler_state();
    EXPECT_TRUE(initial_state == ProfilerState::Disabled || initial_state == ProfilerState::Ready);

    // Start profiler
    ProfilerConfig config;
    config.activities = {ActivityType::CPU};
    session.start(config);
    EXPECT_EQ(get_profiler_state(), ProfilerState::Recording);

    // Stop profiler
    session.stop();
    EXPECT_EQ(get_profiler_state(), ProfilerState::Ready);
}

#endif  // XSIGMA_HAS_PROFILER
