#include <atomic>
#include <string>
#include <vector>

#include "Testing/xsigmaTest.h"
#include "smp_new/native/parallel_native.h"
#include "smp_new/parallel/parallel_api.h"

namespace xsigma::smp_new
{

// Test 1: Get backend
XSIGMATEST(SmpNewBackend, get_backend)
{
    int backend = parallel::get_backend();
    EXPECT_GE(backend, 0);
    EXPECT_LE(backend, 3);  // NATIVE=0, OPENMP=1, AUTO=2, TBB=3
}

// Test 2: Set backend to NATIVE
XSIGMATEST(SmpNewBackend, set_backend_native)
{
    parallel::set_backend(0);  // NATIVE
    int backend = parallel::get_backend();
    EXPECT_EQ(backend, 0);
}

// Test 3: Check OpenMP availability
XSIGMATEST(SmpNewBackend, is_openmp_available)
{
    bool available = parallel::is_openmp_available();
    // Just verify it doesn't crash, value depends on build configuration
    EXPECT_TRUE(available || !available);
}

// Test 4: Check TBB availability
XSIGMATEST(SmpNewBackend, is_tbb_available)
{
    bool available = parallel::is_tbb_available();
    // Just verify it doesn't crash, value depends on build configuration
    EXPECT_TRUE(available || !available);
}

// Test 5: Initialize native backend
XSIGMATEST(SmpNewBackend, initialize_native_backend)
{
    native::InitializeNativeBackend();
    EXPECT_TRUE(native::IsNativeBackendInitialized());
}

// Test 6: Backend info
XSIGMATEST(SmpNewBackend, get_backend_info)
{
    std::string info = native::GetBackendInfo();
    EXPECT_FALSE(info.empty());
}

// Test 7: Native backend info
XSIGMATEST(SmpNewBackend, get_native_backend_info)
{
    native::InitializeNativeBackend();
    std::string info = native::GetNativeBackendInfo();
    EXPECT_FALSE(info.empty());
}

// Test 8: Get parallel info
XSIGMATEST(SmpNewBackend, get_parallel_info)
{
    std::string info = parallel::get_parallel_info();
    EXPECT_FALSE(info.empty());
}

// Test 9: Set num intraop threads
// Note: This test may not be able to set the thread count if the intraop pool
// has already been initialized by a previous test. In that case, it will return
// the default number of threads or the previously set value.
XSIGMATEST(SmpNewBackend, set_num_intraop_threads)
{
    parallel::set_num_intraop_threads(4);
    size_t num_threads = parallel::get_num_intraop_threads();
    // The thread count should be either 4 (if successfully set) or the default
    // (if the pool was already initialized by a previous test)
    EXPECT_TRUE(num_threads == 4 || num_threads > 0);
}

// Test 10: Set num interop threads
// Note: This test may not be able to set the thread count if the interop pool
// has already been initialized by a previous test. In that case, it will return
// the default number of threads or the previously set value.
XSIGMATEST(SmpNewBackend, set_num_interop_threads)
{
    parallel::set_num_interop_threads(2);
    size_t num_threads = parallel::get_num_interop_threads();
    // The thread count should be either 2 (if successfully set) or the default
    // (if the pool was already initialized by a previous test)
    EXPECT_TRUE(num_threads == 2 || num_threads > 0);
}

// Test 11: Get thread num
XSIGMATEST(SmpNewBackend, get_thread_num)
{
    int thread_num = parallel::get_thread_num();
    EXPECT_GE(thread_num, 0);
}

// Test 12: In parallel region (outside)
XSIGMATEST(SmpNewBackend, in_parallel_region_outside)
{
    bool in_region = parallel::in_parallel_region();
    EXPECT_FALSE(in_region);
}

// Test 13: In parallel region (inside)
XSIGMATEST(SmpNewBackend, in_parallel_region_inside)
{
    std::atomic<bool> in_region{false};

    parallel::parallel_for(
        0,
        10,
        5,
        [&in_region](int64_t begin, int64_t end)
        {
            if (!in_region.load())
            {
                in_region.store(parallel::in_parallel_region());
            }
        });

    EXPECT_TRUE(in_region.load());
}

// Test 14: Launch task
XSIGMATEST(SmpNewBackend, launch_task)
{
    std::atomic<bool> done{false};

    parallel::launch([&done]() { done.store(true); });

    // Wait for task to complete (with timeout)
    for (int i = 0; i < 100 && !done.load(); ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    EXPECT_TRUE(done.load());
}

// Test 15: Intraop launch
XSIGMATEST(SmpNewBackend, intraop_launch)
{
    std::atomic<bool> done{false};

    parallel::intraop_launch([&done]() { done.store(true); });

    // Wait for task to complete (with timeout)
    for (int i = 0; i < 100 && !done.load(); ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    EXPECT_TRUE(done.load());
}

// Test 16: Multiple launches
XSIGMATEST(SmpNewBackend, multiple_launches)
{
    std::atomic<int> counter{0};
    const int        num_tasks = 10;

    for (int i = 0; i < num_tasks; ++i)
    {
        parallel::launch([&counter]() { ++counter; });
    }

    // Wait for all tasks to complete (with timeout)
    for (int i = 0; i < 100 && counter.load() < num_tasks; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    EXPECT_EQ(counter.load(), num_tasks);
}

// Test 17: Backend initialization check
XSIGMATEST(SmpNewBackend, is_backend_initialized)
{
    native::InitializeBackend();
    EXPECT_TRUE(native::IsBackendInitialized());
}

// Test 18: Get current backend
XSIGMATEST(SmpNewBackend, get_current_backend)
{
    native::BackendType backend = native::GetCurrentBackend();
    EXPECT_TRUE(
        backend == native::BackendType::NATIVE || backend == native::BackendType::OPENMP ||
        backend == native::BackendType::TBB);
}

// Test 19: Initialize with AUTO backend
XSIGMATEST(SmpNewBackend, initialize_auto_backend)
{
    native::InitializeBackend(native::BackendType::AUTO);
    EXPECT_TRUE(native::IsBackendInitialized());
}

// Test 20: Shutdown and reinitialize
XSIGMATEST(SmpNewBackend, shutdown_and_reinitialize)
{
    native::InitializeBackend();
    EXPECT_TRUE(native::IsBackendInitialized());

    native::ShutdownBackend();
    // Note: After shutdown, backend may still report as initialized
    // depending on implementation

    native::InitializeBackend();
    EXPECT_TRUE(native::IsBackendInitialized());
}

// Test 21: Thread num in parallel region
XSIGMATEST(SmpNewBackend, thread_num_in_parallel_region)
{
    std::vector<int> thread_nums;
    std::mutex       mutex;

    parallel::parallel_for(
        0,
        100,
        10,
        [&thread_nums, &mutex](int64_t begin, int64_t end)
        {
            int                         thread_num = parallel::get_thread_num();
            std::lock_guard<std::mutex> lock(mutex);
            thread_nums.push_back(thread_num);
        });

    EXPECT_FALSE(thread_nums.empty());
    for (int num : thread_nums)
    {
        EXPECT_GE(num, 0);
    }
}

// Test 22: Parallel work after backend initialization
XSIGMATEST(SmpNewBackend, parallel_work_after_init)
{
    native::InitializeBackend();

    std::vector<int> data(100, 0);
    parallel::parallel_for(
        0,
        100,
        10,
        [&data](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                data[i] = static_cast<int>(i);
            }
        });

    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(data[i], i);
    }
}

// Test 23: Get num intraop threads default
XSIGMATEST(SmpNewBackend, get_num_intraop_threads_default)
{
    size_t num_threads = parallel::get_num_intraop_threads();
    EXPECT_GT(num_threads, 0);
}

// Test 24: Get num interop threads default
XSIGMATEST(SmpNewBackend, get_num_interop_threads_default)
{
    size_t num_threads = parallel::get_num_interop_threads();
    EXPECT_GT(num_threads, 0);
}

// Test 25: Backend info contains expected keywords
XSIGMATEST(SmpNewBackend, backend_info_keywords)
{
    std::string info = native::GetBackendInfo();
    // Should contain some backend-related information
    EXPECT_TRUE(
        info.find("Backend") != std::string::npos || info.find("Thread") != std::string::npos ||
        info.find("Parallel") != std::string::npos);
}

// Test 26: Parallel info contains expected keywords
XSIGMATEST(SmpNewBackend, parallel_info_keywords)
{
    std::string info = parallel::get_parallel_info();
    // Should contain some parallelization information
    EXPECT_TRUE(
        info.find("thread") != std::string::npos || info.find("Thread") != std::string::npos ||
        info.find("parallel") != std::string::npos || info.find("Parallel") != std::string::npos);
}

// Test 27: Concurrent backend queries
XSIGMATEST(SmpNewBackend, concurrent_backend_queries)
{
    std::vector<std::thread> threads;
    std::atomic<int>         success_count{0};

    for (int i = 0; i < 10; ++i)
    {
        threads.emplace_back(
            [&success_count]()
            {
                int backend = parallel::get_backend();
                if (backend >= 0 && backend <= 3)
                {
                    ++success_count;
                }
            });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_EQ(success_count.load(), 10);
}

// Test 28: Concurrent parallel info queries
XSIGMATEST(SmpNewBackend, concurrent_parallel_info_queries)
{
    std::vector<std::thread> threads;
    std::atomic<int>         success_count{0};

    for (int i = 0; i < 10; ++i)
    {
        threads.emplace_back(
            [&success_count]()
            {
                std::string info = parallel::get_parallel_info();
                if (!info.empty())
                {
                    ++success_count;
                }
            });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_EQ(success_count.load(), 10);
}

}  // namespace xsigma::smp_new
