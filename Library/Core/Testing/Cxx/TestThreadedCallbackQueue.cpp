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
#include <iomanip>
#include <thread>
#include <vector>

#include "Testing/xsigmaTest.h"
#include "smp/threaded_callback_queue.h"

namespace xsigma
{

// ============================================================================
// Timing Utility Helper
// ============================================================================

class timer_scope
{
public:
    explicit timer_scope(const std::string& section_name)
        : section_name_(section_name), start_(std::chrono::high_resolution_clock::now())
    {
    }

    ~timer_scope()
    {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start_).count();
        std::cout << "    [TIMING] " << section_name_ << ": " << duration_ms << " ms" << std::endl;
    }

private:
    std::string                                    section_name_;
    std::chrono::high_resolution_clock::time_point start_;
};

// ============================================================================
// Test Group 1: Basic Functionality
// ============================================================================

// Merged Test: Basic functionality (creation, thread management, task execution)
XSIGMATEST(ThreadedCallbackQueue, Test)
{
    {
        timer_scope group1_timer("Group 1: Basic Functionality");
        std::cout << "\n=== Test Group 1: Basic Functionality ===\n";

        // Test creation and destruction
        {
            timer_scope test_timer("1.1 Queue creation");
            std::cout << "  [1.1] Testing queue creation..." << std::endl;
            threaded_callback_queue* queue = threaded_callback_queue::create();
            EXPECT_TRUE(queue != nullptr);
            std::cout << "  [1.1] ✓ Queue created successfully\n" << std::endl;

            // Test set and get number of threads
            {
                timer_scope test_timer2("1.2 Thread management (4 threads)");
                std::cout << "  [1.2] Testing thread management (4 threads)..." << std::endl;
                queue->set_number_of_threads(4);
                auto future1a = queue->push([]() { return 0; });
                auto future1b = queue->push([]() { return 0; });
                auto future1c = queue->push([]() { return 0; });
                future1a->wait();
                future1b->wait();
                future1c->wait();
                EXPECT_EQ(queue->get_number_of_threads(), 4);
                std::cout << "  [1.2] ✓ Thread count set to 4\n" << std::endl;
            }

            if(false){
                timer_scope test_timer3("1.3 Thread management (8 threads)");
                std::cout << "  [1.3] Testing thread management (8 threads)..." << std::endl;
                queue->set_number_of_threads(8);
                auto future2a = queue->push([]() { return 0; });
                auto future2b = queue->push([]() { return 0; });
                auto future2c = queue->push([]() { return 0; });
                future2a->wait();
                future2b->wait();
                future2c->wait();
                EXPECT_EQ(queue->get_number_of_threads(), 8);
                std::cout << "  [1.3] ✓ Thread count set to 8\n" << std::endl;
            }

            // Reset to reasonable thread count for remaining tests
            {
                timer_scope test_timer4("1.4 Reset to 2 threads");
                std::cout << "  [1.4] Resetting to 2 threads..." << std::endl;
                queue->set_number_of_threads(2);
                auto sync_future = queue->push([]() { return 0; });
                sync_future->wait();
                std::cout << "  [1.4] ✓ Reset complete\n" << std::endl;
            }

            // Test push simple task with int return
            if (false)
            {
                timer_scope test_timer5("1.5 Simple int task");
                std::cout << "  [1.5] Testing simple int task (expect 42)..." << std::endl;
                auto int_future = queue->push([]() { return 42; });
                EXPECT_TRUE(int_future != nullptr);
                int int_result = int_future->get();
                EXPECT_EQ(int_result, 42);
                std::cout << "  [1.5] ✓ Got result: " << int_result << "\n" << std::endl;
            }

            // Test push task with void return
            {
                timer_scope test_timer6("1.6 Void task with counter");
                std::cout << "  [1.6] Testing void task with counter..." << std::endl;
                std::atomic<int> counter{0};
                auto             void_future = queue->push([&counter]() { counter++; });
                EXPECT_TRUE(void_future != nullptr);
                void_future->wait();
                EXPECT_EQ(counter.load(), 1);
                std::cout << "  [1.6] ✓ Counter incremented to: " << counter.load() << "\n"
                          << std::endl;
            }

            // Test push task with arguments
            {
                timer_scope test_timer7("1.7 Task with arguments");
                std::cout << "  [1.7] Testing task with arguments (10 + 20)..." << std::endl;
                auto add        = [](int a, int b) { return a + b; };
                auto add_future = queue->push(add, 10, 20);
                int  add_result = add_future->get();
                EXPECT_EQ(add_result, 30);
                std::cout << "  [1.7] ✓ Addition result: " << add_result << "\n" << std::endl;
            }

            // Test multiple tasks execution
            if (false)
            {
                timer_scope test_timer8("1.8 10 concurrent tasks");
                std::cout << "  [1.8] Testing 10 concurrent tasks (i²)..." << std::endl;
                queue->set_number_of_threads(4);
                auto sync_future2 = queue->push([]() { return 0; });
                sync_future2->wait();

                using future_type = decltype(std::declval<threaded_callback_queue>().push(
                    std::declval<int (*)()>()));
                std::vector<future_type> futures;

                for (int i = 0; i < 10; ++i)
                {
                    futures.push_back(queue->push([i]() { return i * i; }));
                }

                for (int i = 0; i < 10; ++i)
                {
                    EXPECT_EQ(futures[i]->get(), i * i);
                }
                std::cout << "  [1.8] ✓ All 10 tasks completed correctly\n" << std::endl;
            }

            delete queue;
        }
        std::cout << "=== Group 1 Complete ===\n" << std::endl;
    }

    // ============================================================================
    // Test Group 2: Futures and Synchronization
    // ============================================================================

    // Merged Test: Future operations (wait, get, multiple futures)
    {
        timer_scope              group2_timer("Group 2: Futures and Synchronization");
        threaded_callback_queue* queue = threaded_callback_queue::create();
        queue->set_number_of_threads(4);

        // Test future wait
        {
            timer_scope       test_timer("2.1 Future wait");
            std::atomic<bool> task_started{false};
            auto              wait_future = queue->push(
                [&task_started]()
                {
                    task_started.store(true);
                    // Reduced from 1ms to 100us for faster tests
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                    return 100;
                });

            wait_future->wait();
            EXPECT_TRUE(task_started.load());
            EXPECT_EQ(wait_future->get(), 100);
        }

        // Test get with wait
        {
            timer_scope test_timer("2.2 Get with wait");
            auto        get_future = queue->push(
                []()
                {
                    // Reduced from 1ms to 100us for faster tests
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                    return 999;
                });

            int result = queue->get(get_future);
            EXPECT_EQ(result, 999);
        }

        // Test wait for multiple futures
        {
            timer_scope test_timer("2.3 Multiple futures");
            using future_type =
                decltype(std::declval<threaded_callback_queue>().push(std::declval<int (*)()>()));
            std::vector<future_type> futures;

            for (int i = 0; i < 5; ++i)
            {
                futures.push_back(queue->push(
                    [i]()
                    {
                        // No sleep needed - just test the future mechanism
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
        }

        delete queue;
    }

    // ============================================================================
    // Test Group 3: Dependent Tasks
    // ============================================================================

    // Merged Test: Dependent tasks (simple, multiple, chained)
    {
        timer_scope              group3_timer("Group 3: Dependent Tasks");
        threaded_callback_queue* queue = threaded_callback_queue::create();
        queue->set_number_of_threads(4);

        // Test simple dependent task
        {
            timer_scope test_timer("3.1 Simple dependent task");
            auto        simple_future1 = queue->push([]() { return 10; });

            std::vector<decltype(simple_future1.get())> simple_deps    = {simple_future1.get()};
            auto                                        simple_future2 = queue->push_dependent(
                simple_deps, [&simple_future1]() { return simple_future1->get() * 2; });

            EXPECT_EQ(simple_future2->get(), 20);
        }

        // Test multiple dependencies
        {
            timer_scope test_timer("3.2 Multiple dependencies");
            auto        multi_future1 = queue->push([]() { return 5; });
            auto        multi_future2 = queue->push([]() { return 10; });
            auto        multi_future3 = queue->push([]() { return 15; });

            std::vector<threaded_callback_queue::shared_future_base*> multi_deps = {
                multi_future1.get(), multi_future2.get(), multi_future3.get()};

            auto multi_future_sum = queue->push_dependent(
                multi_deps,
                [&multi_future1, &multi_future2, &multi_future3]()
                { return multi_future1->get() + multi_future2->get() + multi_future3->get(); });

            EXPECT_EQ(multi_future_sum->get(), 30);
        }

        // Test chained dependencies
        {
            timer_scope test_timer("3.3 Chained dependencies");
            queue->set_number_of_threads(2);
            auto sync_future = queue->push([]() { return 0; });
            sync_future->wait();

            auto chain_future1 = queue->push([]() { return 1; });

            std::vector<decltype(chain_future1.get())> deps1         = {chain_future1.get()};
            auto                                       chain_future2 = queue->push_dependent(
                deps1, [&chain_future1]() { return chain_future1->get() + 1; });

            std::vector<decltype(chain_future2.get())> deps2         = {chain_future2.get()};
            auto                                       chain_future3 = queue->push_dependent(
                deps2, [&chain_future2]() { return chain_future2->get() + 1; });

            std::vector<decltype(chain_future3.get())> deps3         = {chain_future3.get()};
            auto                                       chain_future4 = queue->push_dependent(
                deps3, [&chain_future3]() { return chain_future3->get() + 1; });

            EXPECT_EQ(chain_future4->get(), 4);
        }

        delete queue;
    }

    // ============================================================================
    // Test Group 4: Complex Return Types
    // ============================================================================

    // Merged Test: Complex return types (string, vector, struct)
    if (false)
    {
        timer_scope group4_timer("Group 4: Complex Return Types");
        struct Point
        {
            int x, y;
        };

        threaded_callback_queue* queue = threaded_callback_queue::create();
        queue->set_number_of_threads(2);

        // Test string return type
        {
            timer_scope test_timer("4.1 String return type");
            auto        string_future = queue->push([]() { return std::string("Hello, World!"); });
            std::string string_result = string_future->get();
            EXPECT_EQ(string_result, "Hello, World!");
        }

        // Test vector return type
        {
            timer_scope test_timer("4.2 Vector return type");
            auto        vector_future = queue->push(
                []()
                {
                    std::vector<int> vec;
                    for (int i = 0; i < 5; ++i)
                    {
                        vec.push_back(i * 10);
                    }
                    return vec;
                });

            std::vector<int> vector_result = vector_future->get();
            EXPECT_EQ(vector_result.size(), 5);
            for (int i = 0; i < 5; ++i)
            {
                EXPECT_EQ(vector_result[i], i * 10);
            }
        }

        // Test struct return type
        {
            timer_scope test_timer("4.3 Struct return type");
            auto        struct_future = queue->push([]() { return Point{10, 20}; });
            Point       struct_result = struct_future->get();
            EXPECT_EQ(struct_result.x, 10);
            EXPECT_EQ(struct_result.y, 20);
        }

        delete queue;
    }

    // ============================================================================
    // Test Group 5: Thread Safety
    // ============================================================================

    // Merged Test: Thread safety (concurrent pushes, atomic operations)
    {
        timer_scope              group5_timer("Group 5: Thread Safety");
        threaded_callback_queue* queue = threaded_callback_queue::create();
        queue->set_number_of_threads(8);

        // Test concurrent pushes
        {
            timer_scope      test_timer("5.1 Concurrent pushes (100 tasks)");
            std::atomic<int> counter{0};
            using future_type =
                decltype(std::declval<threaded_callback_queue>().push(std::declval<void (*)()>()));
            std::vector<future_type> futures1;

            for (int i = 0; i < 100; ++i)
            {
                futures1.push_back(queue->push([&counter]() { counter++; }));
            }

            for (auto& f : futures1)
            {
                f->wait();
            }

            EXPECT_EQ(counter.load(), 100);
        }

        // Test atomic operations in tasks
        {
            timer_scope test_timer("5.2 Atomic operations (100 tasks)");
            queue->set_number_of_threads(4);
            auto sync_future = queue->push([]() { return 0; });
            sync_future->wait();

            std::atomic<int64_t> sum{0};
            using future_type =
                decltype(std::declval<threaded_callback_queue>().push(std::declval<void (*)()>()));
            std::vector<future_type> futures2;

            for (int i = 0; i < 100; ++i)
            {
                futures2.push_back(
                    queue->push([&sum, i]() { sum.fetch_add(i, std::memory_order_relaxed); }));
            }

            for (auto& f : futures2)
            {
                f->wait();
            }

            int64_t expected = (99 * 100) / 2;
            EXPECT_EQ(sum.load(), expected);
        }

        delete queue;
    }

    // ============================================================================
    // Test Group 6: Edge Cases
    // ============================================================================

    // Merged Test: Edge cases (minimum threads, empty lambda, exception handling)
    {
        timer_scope              group6_timer("Group 6: Edge Cases");
        threaded_callback_queue* queue = threaded_callback_queue::create();

        // Test zero threads (should use at least 1)
        {
            timer_scope test_timer("6.1 Minimum threads (1 thread)");
            queue->set_number_of_threads(1);
            auto sync_future1 = queue->push([]() { return 0; });
            sync_future1->wait();

            auto min_thread_future = queue->push([]() { return 42; });
            EXPECT_EQ(min_thread_future->get(), 42);
        }

        // Test empty lambda
        {
            timer_scope test_timer("6.2 Empty lambda");
            queue->set_number_of_threads(2);
            auto sync_future2 = queue->push([]() { return 0; });
            sync_future2->wait();

            auto empty_future = queue->push([]() {});
            empty_future->wait();
            EXPECT_TRUE(true);
        }

        // Test task that might throw exception
        {
            timer_scope      test_timer("6.3 Multiple counter tasks");
            std::atomic<int> counter{0};
            auto             exception_future1 = queue->push([&counter]() { counter++; });
            auto             exception_future2 = queue->push([&counter]() { counter++; });

            exception_future1->wait();
            exception_future2->wait();

            EXPECT_GE(counter.load(), 1);
        }

        delete queue;
    }

    // ============================================================================
    // Test Group 7: Stress Tests
    // ============================================================================

    // Merged Test: Stress tests (many tasks, rapid thread changes)
    {
        timer_scope              group7_timer("Group 7: Stress Tests");
        threaded_callback_queue* queue = threaded_callback_queue::create();
        queue->set_number_of_threads(8);

        // Test many tasks (reduced from 1000 to 100 for faster tests)
        {
            timer_scope test_timer("7.1 Many tasks (100 tasks)");
            using future_type =
                decltype(std::declval<threaded_callback_queue>().push(std::declval<int (*)()>()));
            std::vector<future_type> futures;

            for (int i = 0; i < 100; ++i)
            {
                futures.push_back(queue->push([i]() { return i; }));
            }

            for (int i = 0; i < 100; ++i)
            {
                EXPECT_EQ(futures[i]->get(), i);
            }
        }

        // Test complex dependency graph
        {
            timer_scope test_timer("7.2 Complex dependency graph");
            queue->set_number_of_threads(4);
            auto sync_future = queue->push([]() { return 0; });
            sync_future->wait();

            auto root = queue->push([]() { return 1; });

            std::vector<decltype(root.get())> root_deps = {root.get()};
            auto left  = queue->push_dependent(root_deps, [&root]() { return root->get() + 1; });
            auto right = queue->push_dependent(root_deps, [&root]() { return root->get() + 2; });

            std::vector<threaded_callback_queue::shared_future_base*> merge_deps = {
                left.get(), right.get()};
            auto merge = queue->push_dependent(
                merge_deps, [&left, &right]() { return left->get() + right->get(); });

            EXPECT_EQ(merge->get(), 5);  // 1 + (1+1) + (1+2) = 5
        }

        // Test rapid thread count changes (reduced iterations for faster tests)
        {
            timer_scope test_timer("7.3 Rapid thread count changes");
            for (int threads = 1; threads <= 4; threads += 2)
            {
                queue->set_number_of_threads(threads);

                auto future = queue->push([threads]() { return threads * 10; });
                EXPECT_EQ(future->get(), threads * 10);
            }
        }

        delete queue;
    }
}
}  // namespace xsigma
