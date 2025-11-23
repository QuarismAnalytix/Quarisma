
#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include "Testing/xsigmaTest.h"
#include "profiler/base/base.h"
#include "profiler/common/orchestration/observer.h"
#include "profiler/common/record_function.h"
#include "profiler/kineto/profiler_kineto.h"
#include "profiler/native/session/profiler.h"

namespace
{

void busy_wait_for(std::chrono::microseconds duration)
{
    const auto start = std::chrono::high_resolution_clock::now();
    while (std::chrono::high_resolution_clock::now() - start < duration)
    {
        XSIGMA_UNUSED volatile int spin = 0;
        ++spin;
    }
}

std::unordered_set<xsigma::RecordScope> user_scopes()
{
    return {xsigma::RecordScope::USER_SCOPE};
}

}  // namespace

#if XSIGMA_HAS_KINETO

namespace
{

xsigma::autograd::profiler::ProfilerConfig make_kineto_config(bool with_stack = false)
{
    return xsigma::autograd::profiler::ProfilerConfig(
        xsigma::autograd::profiler::ProfilerState::KINETO,
        /*report_input_shapes=*/false,
        /*profile_memory=*/false,
        /*with_stack=*/with_stack,
        /*with_flops=*/false,
        /*with_modules=*/false);
}

}  // namespace

namespace
{

struct kineto_session_guard
{
    kineto_session_guard(
        xsigma::autograd::profiler::ProfilerConfig         cfg,
        std::set<xsigma::autograd::profiler::ActivityType> activities,
        std::unordered_set<xsigma::RecordScope>            scopes = user_scopes())
        : config_{std::move(cfg)}, activities_{std::move(activities)}, scopes_{std::move(scopes)}
    {
        try
        {
            xsigma::autograd::profiler::prepareProfiler(config_, activities_);
            xsigma::autograd::profiler::enableProfiler(config_, activities_, scopes_);
            active_ = true;
        }
        catch (const std::exception& ex)
        {
            error_message_ = ex.what();
        }
    }

    kineto_session_guard(const kineto_session_guard&)            = delete;
    kineto_session_guard(kineto_session_guard&&)                 = delete;
    kineto_session_guard& operator=(const kineto_session_guard&) = delete;
    kineto_session_guard& operator=(kineto_session_guard&&)      = delete;

    ~kineto_session_guard()
    {
        if (active_)
        {
            try
            {
                xsigma::autograd::profiler::disableProfiler();
            }
            catch (...)
            {
                // Never let a failed disable propagate from a test helper.
            }
        }
    }

    bool active() const { return active_; }

    const std::string& error() const { return error_message_; }

    std::unique_ptr<xsigma::autograd::profiler::ProfilerResult> stop()
    {
        if (!active_)
        {
            return nullptr;
        }
        active_ = false;
        return xsigma::autograd::profiler::disableProfiler();
    }

private:
    xsigma::autograd::profiler::ProfilerConfig         config_;
    std::set<xsigma::autograd::profiler::ActivityType> activities_;
    std::unordered_set<xsigma::RecordScope>            scopes_;
    bool                                               active_{false};
    std::string                                        error_message_;
};

const xsigma::autograd::profiler::KinetoEvent* find_event_by_name(
    const std::vector<xsigma::autograd::profiler::KinetoEvent>& events, const std::string& name)
{
    for (const auto& event : events)
    {
        if (event.name() == name)
        {
            return &event;
        }
    }
    return nullptr;
}

}  // namespace

//XSIGMATEST(KinetoIntegration, captures_basic_scope_lifecycle)
//{
//    kineto_session_guard session(
//        make_kineto_config(), {xsigma::autograd::profiler::ActivityType::CPU});
//    if (!session.active())
//    {
//        GTEST_SKIP() << "Kineto profiler unavailable: " << session.error();
//    }
//
//    {
//        RECORD_USER_SCOPE("kineto_basic_scope");
//        std::this_thread::sleep_for(std::chrono::milliseconds(1));
//    }
//
//    auto profiler_result = session.stop();
//    ASSERT_NE(profiler_result, nullptr);
//    const auto& events = profiler_result->events();
//    if (events.empty())
//    {
//        GTEST_SKIP() << "Kineto backend produced no CPU events in this environment";
//    }
//
//    const auto* scope_event = find_event_by_name(events, "kineto_basic_scope");
//    ASSERT_NE(scope_event, nullptr);
//    EXPECT_GT(scope_event->durationNs(), 0);
//    EXPECT_FALSE(scope_event->isHiddenEvent());
//}
//
//XSIGMATEST(KinetoIntegration, nested_scopes_preserve_parent_child_timing)
//{
//    kineto_session_guard session(
//        make_kineto_config(/*with_stack=*/true), {xsigma::autograd::profiler::ActivityType::CPU});
//    if (!session.active())
//    {
//        GTEST_SKIP() << "Kineto profiler unavailable: " << session.error();
//    }
//
//    {
//        RECORD_USER_SCOPE("kineto_parent_scope");
//        busy_wait_for(std::chrono::milliseconds(1));
//        {
//            RECORD_USER_SCOPE("kineto_child_scope");
//            busy_wait_for(std::chrono::milliseconds(1));
//        }
//    }
//
//    auto profiler_result = session.stop();
//    ASSERT_NE(profiler_result, nullptr);
//    const auto& events = profiler_result->events();
//    if (events.empty())
//    {
//        GTEST_SKIP() << "Kineto backend produced no CPU events in this environment";
//    }
//
//    const auto* parent_event = find_event_by_name(events, "kineto_parent_scope");
//    const auto* child_event  = find_event_by_name(events, "kineto_child_scope");
//    if (parent_event == nullptr || child_event == nullptr)
//    {
//        GTEST_SKIP() << "Required Kineto events not emitted";
//    }
//
//    EXPECT_LE(parent_event->startNs(), child_event->startNs());
//    EXPECT_GE(parent_event->endNs(), child_event->endNs());
//    EXPECT_GE(parent_event->durationNs(), child_event->durationNs());
//}
//
//XSIGMATEST(KinetoIntegration, thread_local_participation_requires_opt_in)
//{
//    kineto_session_guard session(
//        make_kineto_config(), {xsigma::autograd::profiler::ActivityType::CPU});
//    if (!session.active())
//    {
//        GTEST_SKIP() << "Kineto profiler unavailable: " << session.error();
//    }
//
//    constexpr const char* detached_scope_name = "kineto_thread_scope_detached";
//    constexpr const char* attached_scope_name = "kineto_thread_scope_attached";
//
//    std::thread detached_thread(
//        [detached_scope_name]()
//        {
//            RECORD_USER_SCOPE(detached_scope_name);
//            std::this_thread::sleep_for(std::chrono::milliseconds(1));
//        });
//
//    std::thread attached_thread(
//        [attached_scope_name]()
//        {
//            xsigma::autograd::profiler::enableProfilerInChildThread();
//            RECORD_USER_SCOPE(attached_scope_name);
//            std::this_thread::sleep_for(std::chrono::milliseconds(1));
//            xsigma::autograd::profiler::disableProfilerInChildThread();
//        });
//
//    detached_thread.join();
//    attached_thread.join();
//
//    auto profiler_result = session.stop();
//    ASSERT_NE(profiler_result, nullptr);
//    const auto& events = profiler_result->events();
//    if (events.empty())
//    {
//        GTEST_SKIP() << "Kineto backend produced no CPU events in this environment";
//    }
//
//    bool found_detached = false;
//    bool found_attached = false;
//    for (const auto& event : events)
//    {
//        if (event.name() == detached_scope_name)
//        {
//            found_detached = true;
//        }
//        if (event.name() == attached_scope_name)
//        {
//            found_attached = true;
//        }
//    }
//
//    EXPECT_TRUE(found_attached);
//    EXPECT_FALSE(found_detached);
//}

//XSIGMATEST(KinetoIntegration, enable_disable_cycles_reset_global_state)
//{
//    kineto_session_guard first_session(
//        make_kineto_config(), {xsigma::autograd::profiler::ActivityType::CPU});
//    if (!first_session.active())
//    {
//        GTEST_SKIP() << "Kineto profiler unavailable: " << first_session.error();
//    }
//
//    EXPECT_TRUE(xsigma::autograd::profiler::isProfilerEnabledInMainThread());
//    first_session.stop();
//    EXPECT_FALSE(xsigma::autograd::profiler::isProfilerEnabledInMainThread());
//
//    kineto_session_guard second_session(
//        make_kineto_config(), {xsigma::autograd::profiler::ActivityType::CPU});
//    ASSERT_TRUE(second_session.active());
//    EXPECT_TRUE(xsigma::autograd::profiler::isProfilerEnabledInMainThread());
//    auto result = second_session.stop();
//    EXPECT_FALSE(xsigma::autograd::profiler::isProfilerEnabledInMainThread());
//    EXPECT_NE(result, nullptr);
//}

#endif  // XSIGMA_HAS_KINETO

#if XSIGMA_HAS_ITT

namespace
{

class recording_itt_stub : public xsigma::profiler::impl::ProfilerStubs
{
public:
    void record(
        xsigma::device_option::int_t*,
        xsigma::profiler::impl::ProfilerVoidEventStub*,
        int64_t*) const override
    {
    }

    float elapsed(
        const xsigma::profiler::impl::ProfilerVoidEventStub*,
        const xsigma::profiler::impl::ProfilerVoidEventStub*) const override
    {
        return 0.0F;
    }

    void mark(const char* name) const override
    {
        if (name != nullptr)
        {
            marks_.emplace_back(name);
        }
    }

    void rangePush(const char* name) const override
    {
        if (name != nullptr)
        {
            pushes_.emplace_back(name);
            active_stack_.emplace_back(name);
        }
        else
        {
            ++null_pushes_;
        }
    }

    void rangePop() const override
    {
        if (!active_stack_.empty())
        {
            closed_.emplace_back(active_stack_.back());
            active_stack_.pop_back();
        }
        ++pops_;
    }

    bool enabled() const override { return true; }

    void onEachDevice(std::function<void(int)>) const override {}

    void synchronize() const override {}

    mutable std::vector<std::string> pushes_;
    mutable std::vector<std::string> closed_;
    mutable std::vector<std::string> marks_;
    mutable std::vector<std::string> active_stack_;
    mutable size_t                   pops_{0};
    mutable size_t                   null_pushes_{0};
};

class scoped_itt_stub
{
public:
    explicit scoped_itt_stub(recording_itt_stub& stub)
        : stub_(stub),
          previous_(
              const_cast<xsigma::profiler::impl::ProfilerStubs*>(
                  xsigma::profiler::impl::ittStubs()))
    {
        xsigma::profiler::impl::registerITTMethods(&stub_);
    }

    scoped_itt_stub(const scoped_itt_stub&)            = delete;
    scoped_itt_stub(scoped_itt_stub&&)                 = delete;
    scoped_itt_stub& operator=(const scoped_itt_stub&) = delete;
    scoped_itt_stub& operator=(scoped_itt_stub&&)      = delete;

    ~scoped_itt_stub() { xsigma::profiler::impl::registerITTMethods(previous_); }

private:
    recording_itt_stub&                          stub_;
    xsigma::profiler::impl::ProfilerStubs* const previous_;
};

}  // namespace

XSIGMATEST(ITTIntegration, records_basic_scope_sequence)
{
    recording_itt_stub stub;
    scoped_itt_stub    stub_guard(stub);

    xsigma::autograd::profiler::ProfilerConfig config(
        xsigma::autograd::profiler::ProfilerState::ITT);
    try
    {
        xsigma::autograd::profiler::enableProfiler(
            config, {xsigma::autograd::profiler::ActivityType::CPU}, user_scopes());
    }
    catch (const std::exception& ex)
    {
        GTEST_SKIP() << "ITT callbacks unavailable: " << ex.what();
    }

    {
        RECORD_USER_SCOPE("itt_basic_scope");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    XSIGMA_UNUSED auto result = xsigma::autograd::profiler::disableProfiler();

    ASSERT_FALSE(stub.pushes_.empty());
    EXPECT_EQ(stub.pushes_.front(), "itt_basic_scope");
    EXPECT_EQ(stub.pops_, stub.pushes_.size());
}

XSIGMATEST(ITTIntegration, nested_scopes_close_in_lifo_order)
{
    recording_itt_stub stub;
    scoped_itt_stub    stub_guard(stub);

    xsigma::autograd::profiler::ProfilerConfig config(
        xsigma::autograd::profiler::ProfilerState::ITT);
    try
    {
        xsigma::autograd::profiler::enableProfiler(
            config, {xsigma::autograd::profiler::ActivityType::CPU}, user_scopes());
    }
    catch (const std::exception& ex)
    {
        GTEST_SKIP() << "ITT callbacks unavailable: " << ex.what();
    }

    {
        RECORD_USER_SCOPE("itt_parent_scope");
        busy_wait_for(std::chrono::milliseconds(1));
        {
            RECORD_USER_SCOPE("itt_child_scope");
            busy_wait_for(std::chrono::milliseconds(1));
        }
    }

    XSIGMA_UNUSED auto result = xsigma::autograd::profiler::disableProfiler();

    ASSERT_GE(stub.closed_.size(), 2U);
    EXPECT_EQ(stub.closed_.front(), "itt_child_scope");
    EXPECT_EQ(stub.closed_.back(), "itt_parent_scope");
}

//XSIGMATEST(ITTIntegration, callbacks_are_thread_local)
//{
//    recording_itt_stub stub;
//    scoped_itt_stub    stub_guard(stub);
//
//    xsigma::autograd::profiler::ProfilerConfig config(
//        xsigma::autograd::profiler::ProfilerState::ITT);
//    try
//    {
//        xsigma::autograd::profiler::enableProfiler(
//            config, {xsigma::autograd::profiler::ActivityType::CPU}, user_scopes());
//    }
//    catch (const std::exception& ex)
//    {
//        GTEST_SKIP() << "ITT callbacks unavailable: " << ex.what();
//    }
//
//    constexpr const char* main_scope   = "itt_thread_main_scope";
//    constexpr const char* worker_scope = "itt_thread_worker_scope";
//
//    std::thread worker(
//        []()
//        {
//            RECORD_USER_SCOPE("worker_scope");
//            std::this_thread::sleep_for(std::chrono::milliseconds(1));
//        });
//
//    {
//        RECORD_USER_SCOPE("main_scope");
//        std::this_thread::sleep_for(std::chrono::milliseconds(1));
//    }
//
//    worker.join();
//
//    XSIGMA_UNUSED auto result = xsigma::autograd::profiler::disableProfiler();
//
//    bool saw_main_scope   = false;
//    bool saw_worker_scope = false;
//    for (const auto& name : stub.pushes_)
//    {
//        if (name == main_scope)
//        {
//            saw_main_scope = true;
//        }
//        if (name == worker_scope)
//        {
//            saw_worker_scope = true;
//        }
//    }
//
//    EXPECT_TRUE(saw_main_scope);
//    EXPECT_FALSE(saw_worker_scope);
//}

XSIGMATEST(ITTIntegration, handles_empty_and_null_range_names)
{
    recording_itt_stub stub;
    scoped_itt_stub    stub_guard(stub);

    xsigma::autograd::profiler::ProfilerConfig config(
        xsigma::autograd::profiler::ProfilerState::ITT);
    try
    {
        xsigma::autograd::profiler::enableProfiler(
            config, {xsigma::autograd::profiler::ActivityType::CPU}, user_scopes());
    }
    catch (const std::exception& ex)
    {
        GTEST_SKIP() << "ITT callbacks unavailable: " << ex.what();
    }

    {
        RECORD_USER_SCOPE("");
        busy_wait_for(std::chrono::milliseconds(1));
    }

    xsigma::profiler::impl::ittStubs()->rangePush(nullptr);
    xsigma::profiler::impl::ittStubs()->rangePop();

    XSIGMA_UNUSED auto result = xsigma::autograd::profiler::disableProfiler();

    bool saw_empty_name = false;
    for (const auto& name : stub.pushes_)
    {
        if (name.empty())
        {
            saw_empty_name = true;
            break;
        }
    }

    EXPECT_TRUE(saw_empty_name);
    EXPECT_EQ(stub.null_pushes_, 1U);
}

#endif  // XSIGMA_HAS_ITT

namespace
{

std::filesystem::path make_temp_trace_path(const std::string& name)
{
    auto            path = std::filesystem::temp_directory_path() / name;
    std::error_code ec;
    std::filesystem::remove(path, ec);
    return path;
}

}  // namespace

XSIGMATEST(ProfilerChromeTrace, exports_nested_scope_metadata)
{
    xsigma::profiler_session_builder builder;
    auto session = builder.with_hierarchical_profiling(true).with_memory_tracking(false).build();
    ASSERT_NE(session, nullptr);
    ASSERT_TRUE(session->start());

    {
        xsigma::profiler_scope outer_scope("chrome_trace_parent", session.get());
        busy_wait_for(std::chrono::milliseconds(1));
        {
            xsigma::profiler_scope inner_scope("chrome_trace_child", session.get());
            busy_wait_for(std::chrono::milliseconds(1));
        }
    }

    ASSERT_TRUE(session->stop());

    auto trace_path = make_temp_trace_path("xsigma_profiler_trace.json");
    ASSERT_TRUE(session->write_chrome_trace(trace_path.string()));

    std::ifstream trace_file(trace_path);
    ASSERT_TRUE(trace_file.good());
    const std::string trace_contents(
        (std::istreambuf_iterator<char>(trace_file)), std::istreambuf_iterator<char>());

    EXPECT_NE(trace_contents.find("\"traceEvents\""), std::string::npos);
    EXPECT_NE(trace_contents.find("chrome_trace_parent"), std::string::npos);
    EXPECT_NE(trace_contents.find("chrome_trace_child"), std::string::npos);

    std::error_code ec;
    std::filesystem::remove(trace_path, ec);
}

XSIGMATEST(ProfilerChromeTrace, write_chrome_trace_rejects_empty_path)
{
    xsigma::profiler_options opts;
    auto                     session = std::make_unique<xsigma::profiler_session>(opts);
    ASSERT_TRUE(session->start());
    {
        xsigma::profiler_scope scope("chrome_trace_invalid_path", session.get());
        busy_wait_for(std::chrono::milliseconds(1));
    }
    ASSERT_TRUE(session->stop());
    EXPECT_FALSE(session->write_chrome_trace(""));
}
