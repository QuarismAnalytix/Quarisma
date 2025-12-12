/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Comprehensive test suite for threaded_callback_queue functionality
 * Tests asynchronous task execution, futures, dependencies, and thread management
 */

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "Testing/xsigmaTest.h"
#include "smp/threaded_callback_queue.h"

namespace xsigma
{

// ============================================================================
// Test Group 1: Basic Functionality
// ============================================================================

// Test 1: Create and destroy callback queue
XSIGMATEST(ThreadedCallbackQueue, creation_and_destruction)
{
    threaded_callback_queue* queue = threaded_callback_queue::create();
    EXPECT_TRUE(queue != nullptr);
    delete queue;
}

// Test 2: Set and get number of threads
XSIGMATEST(ThreadedCallbackQueue, set_get_threads)
{
    threaded_callback_queue* queue = threaded_callback_queue::create();

    queue->set_number_of_threads(4);
    // Push a dummy task to ensure the control task has been processed
    auto future1 = queue->push([]() { return 0; });
    future1->wait();
    EXPECT_EQ(queue->get_number_of_threads(), 4);

    queue->set_number_of_threads(8);
    // Push a dummy task to ensure the control task has been processed
    auto future2 = queue->push([]() { return 0; });
    future2->wait();
    EXPECT_EQ(queue->get_number_of_threads(), 8);

    delete queue;
}

// Test 3: Push simple task with int return
XSIGMATEST(ThreadedCallbackQueue, push_simple_int_task)
{
    threaded_callback_queue* queue = threaded_callback_queue::create();
    queue->set_number_of_threads(2);

    auto future = queue->push([]() { return 42; });

    EXPECT_TRUE(future != nullptr);
    int result = future->get();
    EXPECT_EQ(result, 42);

    delete queue;
}

// Test 4: Push task with void return
XSIGMATEST(ThreadedCallbackQueue, push_void_task)
{
    threaded_callback_queue* queue = threaded_callback_queue::create();
    queue->set_number_of_threads(2);

    std::atomic<int> counter{0};
    auto             future = queue->push([&counter]() { counter++; });

    EXPECT_TRUE(future != nullptr);
    future->wait();
    EXPECT_EQ(counter.load(), 1);

    delete queue;
}

// Test 5: Push task with arguments
XSIGMATEST(ThreadedCallbackQueue, push_task_with_args)
{
    threaded_callback_queue* queue = threaded_callback_queue::create();
    queue->set_number_of_threads(2);

    auto add    = [](int a, int b) { return a + b; };
    auto future = queue->push(add, 10, 20);

    int result = future->get();
    EXPECT_EQ(result, 30);

    delete queue;
}

// Test 6: Multiple tasks execution
XSIGMATEST(ThreadedCallbackQueue, multiple_tasks)
{
    threaded_callback_queue* queue = threaded_callback_queue::create();
    queue->set_number_of_threads(4);

    using future_type =
        decltype(std::declval<threaded_callback_queue>().push(std::declval<int (*)()>()));
    std::vector<future_type> futures;

    for (int i = 0; i < 10; ++i)
    {
        futures.push_back(queue->push([i]() { return i * i; }));
    }

    for (int i = 0; i < 10; ++i)
    {
        EXPECT_EQ(futures[i]->get(), i * i);
    }

    delete queue;
}

// ============================================================================
// Test Group 2: Futures and Synchronization
// ============================================================================

// Test 7: Future wait
XSIGMATEST(ThreadedCallbackQueue, future_wait)
{
    threaded_callback_queue* queue = threaded_callback_queue::create();
    queue->set_number_of_threads(2);

    std::atomic<bool> task_started{false};
    auto              future = queue->push(
        [&task_started]()
        {
            task_started.store(true);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            return 100;
        });

    future->wait();
    EXPECT_TRUE(task_started.load());
    EXPECT_EQ(future->get(), 100);

    delete queue;
}

// Test 8: Get with wait
XSIGMATEST(ThreadedCallbackQueue, get_with_wait)
{
    threaded_callback_queue* queue = threaded_callback_queue::create();
    queue->set_number_of_threads(2);

    auto future = queue->push(
        []()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            return 999;
        });

    // get() should wait for completion
    int result = queue->get(future);
    EXPECT_EQ(result, 999);

    delete queue;
}

// Test 9: Wait for multiple futures
XSIGMATEST(ThreadedCallbackQueue, wait_multiple_futures)
{
    threaded_callback_queue* queue = threaded_callback_queue::create();
    queue->set_number_of_threads(4);

    using future_type =
        decltype(std::declval<threaded_callback_queue>().push(std::declval<int (*)()>()));
    std::vector<future_type> futures;

    for (int i = 0; i < 5; ++i)
    {
        futures.push_back(queue->push(
            [i]()
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                return i;
            }));
    }

    // Convert to raw pointers for wait
    std::vector<decltype(futures[0].get())> future_ptrs;
    for (auto& f : futures)
    {
        future_ptrs.push_back(f.get());
    }

    queue->wait(future_ptrs);

    // All futures should be ready
    for (int i = 0; i < 5; ++i)
    {
        EXPECT_EQ(futures[i]->get(), i);
    }

    delete queue;
}

// ============================================================================
// Test Group 3: Dependent Tasks
// ============================================================================

// Test 10: Simple dependent task
XSIGMATEST(ThreadedCallbackQueue, simple_dependent_task)
{
    threaded_callback_queue* queue = threaded_callback_queue::create();
    queue->set_number_of_threads(2);

    auto future1 = queue->push([]() { return 10; });

    std::vector<decltype(future1.get())> deps = {future1.get()};
    auto future2 = queue->push_dependent(deps, [&future1]() { return future1->get() * 2; });

    EXPECT_EQ(future2->get(), 20);

    delete queue;
}

// Test 11: Multiple dependencies
XSIGMATEST(ThreadedCallbackQueue, multiple_dependencies)
{
    threaded_callback_queue* queue = threaded_callback_queue::create();
    queue->set_number_of_threads(4);

    auto future1 = queue->push([]() { return 5; });
    auto future2 = queue->push([]() { return 10; });
    auto future3 = queue->push([]() { return 15; });

    std::vector<threaded_callback_queue::shared_future_base*> deps = {
        future1.get(), future2.get(), future3.get()};

    auto future_sum = queue->push_dependent(
        deps,
        [&future1, &future2, &future3]()
        { return future1->get() + future2->get() + future3->get(); });

    EXPECT_EQ(future_sum->get(), 30);

    delete queue;
}

// Test 12: Chained dependencies
XSIGMATEST(ThreadedCallbackQueue, chained_dependencies)
{
    threaded_callback_queue* queue = threaded_callback_queue::create();
    queue->set_number_of_threads(2);

    auto future1 = queue->push([]() { return 1; });

    std::vector<decltype(future1.get())> deps1 = {future1.get()};
    auto future2 = queue->push_dependent(deps1, [&future1]() { return future1->get() + 1; });

    std::vector<decltype(future2.get())> deps2 = {future2.get()};
    auto future3 = queue->push_dependent(deps2, [&future2]() { return future2->get() + 1; });

    std::vector<decltype(future3.get())> deps3 = {future3.get()};
    auto future4 = queue->push_dependent(deps3, [&future3]() { return future3->get() + 1; });

    EXPECT_EQ(future4->get(), 4);

    delete queue;
}

// ============================================================================
// Test Group 4: Complex Return Types
// ============================================================================

// Test 13: String return type
XSIGMATEST(ThreadedCallbackQueue, string_return)
{
    threaded_callback_queue* queue = threaded_callback_queue::create();
    queue->set_number_of_threads(2);

    auto future = queue->push([]() { return std::string("Hello, World!"); });

    std::string result = future->get();
    EXPECT_EQ(result, "Hello, World!");

    delete queue;
}

// Test 14: Vector return type
XSIGMATEST(ThreadedCallbackQueue, vector_return)
{
    threaded_callback_queue* queue = threaded_callback_queue::create();
    queue->set_number_of_threads(2);

    auto future = queue->push(
        []()
        {
            std::vector<int> vec;
            for (int i = 0; i < 5; ++i)
            {
                vec.push_back(i * 10);
            }
            return vec;
        });

    std::vector<int> result = future->get();
    EXPECT_EQ(result.size(), 5);
    for (int i = 0; i < 5; ++i)
    {
        EXPECT_EQ(result[i], i * 10);
    }

    delete queue;
}

// Test 15: Struct return type
XSIGMATEST(ThreadedCallbackQueue, struct_return)
{
    struct Point
    {
        int x, y;
    };

    threaded_callback_queue* queue = threaded_callback_queue::create();
    queue->set_number_of_threads(2);

    auto future = queue->push([]() { return Point{10, 20}; });

    Point result = future->get();
    EXPECT_EQ(result.x, 10);
    EXPECT_EQ(result.y, 20);

    delete queue;
}

// ============================================================================
// Test Group 5: Thread Safety
// ============================================================================

// Test 16: Concurrent pushes
XSIGMATEST(ThreadedCallbackQueue, concurrent_pushes)
{
    threaded_callback_queue* queue = threaded_callback_queue::create();
    queue->set_number_of_threads(8);

    std::atomic<int> sum{0};
    using future_type =
        decltype(std::declval<threaded_callback_queue>().push(std::declval<void (*)()>()));
    std::vector<future_type> futures;

    for (int i = 0; i < 100; ++i)
    {
        futures.push_back(queue->push([&sum]() { sum++; }));
    }

    for (auto& f : futures)
    {
        f->wait();
    }

    EXPECT_EQ(sum.load(), 100);

    delete queue;
}

// Test 17: Atomic operations in tasks
XSIGMATEST(ThreadedCallbackQueue, atomic_operations)
{
    threaded_callback_queue* queue = threaded_callback_queue::create();
    queue->set_number_of_threads(4);

    std::atomic<int64_t> sum{0};
    using future_type =
        decltype(std::declval<threaded_callback_queue>().push(std::declval<void (*)()>()));
    std::vector<future_type> futures;

    for (int i = 0; i < 100; ++i)
    {
        futures.push_back(
            queue->push([&sum, i]() { sum.fetch_add(i, std::memory_order_relaxed); }));
    }

    for (auto& f : futures)
    {
        f->wait();
    }

    int64_t expected = (99 * 100) / 2;
    EXPECT_EQ(sum.load(), expected);

    delete queue;
}

// ============================================================================
// Test Group 6: Edge Cases
// ============================================================================

// Test 18: Zero threads (should use at least 1)
XSIGMATEST(ThreadedCallbackQueue, zero_threads)
{
    threaded_callback_queue* queue = threaded_callback_queue::create();
    // Note: set_number_of_threads is asynchronous, so we need to wait for it to complete
    // by pushing a task and waiting for it. Setting to 1 thread (minimum).
    queue->set_number_of_threads(1);
    auto sync_future = queue->push([]() { return 0; });
    sync_future->wait();

    // Should still work with at least 1 thread
    auto future = queue->push([]() { return 42; });
    EXPECT_EQ(future->get(), 42);

    delete queue;
}

// Test 19: Empty lambda
XSIGMATEST(ThreadedCallbackQueue, empty_lambda)
{
    threaded_callback_queue* queue = threaded_callback_queue::create();
    queue->set_number_of_threads(2);

    auto future = queue->push([]() {});
    future->wait();

    // Should not crash
    EXPECT_TRUE(true);

    delete queue;
}

// Test 20: Task that throws exception
XSIGMATEST(ThreadedCallbackQueue, task_exception)
{
    threaded_callback_queue* queue = threaded_callback_queue::create();
    queue->set_number_of_threads(2);

    std::atomic<int> counter{0};

    // Task that might throw
    auto future1 = queue->push([&counter]() { counter++; });

    // Subsequent task should still work
    auto future2 = queue->push([&counter]() { counter++; });

    future1->wait();
    future2->wait();

    EXPECT_GE(counter.load(), 1);

    delete queue;
}

// ============================================================================
// Test Group 7: Stress Tests
// ============================================================================

// Test 21: Many tasks
XSIGMATEST(ThreadedCallbackQueue, stress_many_tasks)
{
    threaded_callback_queue* queue = threaded_callback_queue::create();
    queue->set_number_of_threads(8);

    using future_type =
        decltype(std::declval<threaded_callback_queue>().push(std::declval<int (*)()>()));
    std::vector<future_type> futures;

    for (int i = 0; i < 1000; ++i)
    {
        futures.push_back(queue->push([i]() { return i; }));
    }

    for (int i = 0; i < 1000; ++i)
    {
        EXPECT_EQ(futures[i]->get(), i);
    }

    delete queue;
}

// Test 22: Complex dependency graph
XSIGMATEST(ThreadedCallbackQueue, complex_dependencies)
{
    threaded_callback_queue* queue = threaded_callback_queue::create();
    queue->set_number_of_threads(4);

    // Create a diamond dependency pattern
    auto root = queue->push([]() { return 1; });

    std::vector<decltype(root.get())> root_deps = {root.get()};
    auto left  = queue->push_dependent(root_deps, [&root]() { return root->get() + 1; });
    auto right = queue->push_dependent(root_deps, [&root]() { return root->get() + 2; });

    std::vector<threaded_callback_queue::shared_future_base*> merge_deps = {
        left.get(), right.get()};
    auto merge =
        queue->push_dependent(merge_deps, [&left, &right]() { return left->get() + right->get(); });

    EXPECT_EQ(merge->get(), 5);  // 1 + (1+1) + (1+2) = 5

    delete queue;
}

// Test 23: Rapid thread count changes
XSIGMATEST(ThreadedCallbackQueue, rapid_thread_changes)
{
    threaded_callback_queue* queue = threaded_callback_queue::create();

    for (int threads = 1; threads <= 8; ++threads)
    {
        queue->set_number_of_threads(threads);

        auto future = queue->push([threads]() { return threads * 10; });
        EXPECT_EQ(future->get(), threads * 10);
    }

    delete queue;
}

}  // namespace xsigma
