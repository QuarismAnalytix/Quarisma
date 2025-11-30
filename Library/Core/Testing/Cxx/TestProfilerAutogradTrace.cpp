#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>

#include "Testing/xsigmaTest.h"
#include "logging/logger.h"
#include "profiler/common/api.h"
#include "profiler/common/record_function.h"
#include "profiler/kineto/profiler_kineto.h"

namespace
{
#if XSIGMA_HAS_KINETO

std::string makeTracePath()
{
    const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
    return "xsigma_autograd_trace.json";
}

void runSampleWork()
{
    // Use RECORD_EDGE_SCOPE to create a profiling scope
    constexpr int64_t                 kDebugHandle = 42;
    const std::vector<xsigma::IValue> no_inputs{};
    RECORD_EDGE_SCOPE_WITH_DEBUG_HANDLE_AND_INPUTS(
        "autograd_profiler_sample_work", kDebugHandle, no_inputs);

    const auto start_time = std::chrono::steady_clock::now();

    double accumulator = 0.;
    for (int i = 0; i < 10000; ++i)
    {
        double x = static_cast<double>(i / 1000.0);
        accumulator += sinh(x) / x;
    }

    if (accumulator == -1)
    {
        accumulator = 0;
    }
}

#endif  // XSIGMA_HAS_KINETO
}  // namespace

#if XSIGMA_HAS_KINETO

XSIGMATEST(profiler, autograd_chrome_trace_export)
{
    const std::set<xsigma::autograd::profiler::ActivityType> activities{
        xsigma::autograd::profiler::ActivityType::CPU,
    };

    xsigma::autograd::profiler::ProfilerConfig config(
        xsigma::autograd::profiler::ProfilerState::KINETO,
        /*report_input_shapes=*/true,
        /*profile_memory=*/true,
        /*with_stack=*/true,
        /*with_flops=*/true,
        /*with_modules=*/false);

    // Specify LITE_INTERPRETER scope to capture RECORD_EDGE_SCOPE events
    const std::unordered_set<xsigma::RecordScope> scopes = {xsigma::RecordScope::LITE_INTERPRETER};

    xsigma::autograd::profiler::prepareProfiler(config, activities);
    xsigma::autograd::profiler::enableProfiler(config, activities, scopes);

    EXPECT_TRUE(xsigma::hasCallbacks()) << "RecordFunction callbacks not registered for profiler";

    // Enable RecordFunction to allow profiling scopes to work
    xsigma::RecordFunctionGuard record_function_guard(/*is_enabled=*/true);

    runSampleWork();

    auto result = xsigma::autograd::profiler::disableProfiler();
    EXPECT_NE(result, nullptr);
    EXPECT_GT(result->events().size(), 0) << "No profiling events captured";
    EXPECT_GT(result->event_tree().size(), 0) << "No events in event tree";

    const auto trace_path = makeTracePath();
    result->save(trace_path);

    std::ifstream trace_input(trace_path, std::ios::binary | std::ios::ate);
    EXPECT_TRUE(trace_input.is_open());
    const auto file_size = static_cast<std::size_t>(trace_input.tellg());
    EXPECT_GT(file_size, 0) << "Trace file is empty";
    trace_input.close();

    std::remove(trace_path.c_str());
}

#endif  // XSIGMA_HAS_KINETO
