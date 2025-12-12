// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "threaded_callback_queue.h"

#include <algorithm>

//=============================================================================
class threaded_callback_queue::thread_worker
{
public:
    thread_worker(threaded_callback_queue* queue, std::shared_ptr<std::atomic_int>& thread_index)
        : m_queue(queue), m_thread_index(thread_index)
    {
    }

    thread_worker(thread_worker&& other) noexcept
        : m_queue(other.m_queue), m_thread_index(std::move(other.m_thread_index))
    {
    }

    void operator()()
    {
        while (this->pop()) {}
        const std::scoped_lock lock(m_queue->m_control_mutex);
        m_queue->m_thread_id_to_index.erase(std::this_thread::get_id());
    }

private:
    /**
   * Pops an invoker from the queue and runs it.
   */
    bool pop()
    {
        std::unique_lock<std::mutex> lock(m_queue->m_mutex);

        if (this->on_hold())
        {
            m_queue->m_condition_variable.wait(lock, [this] { return !this->on_hold(); });
        }

        if (!this->can_continue())
        {
            return false;
        }

        auto& invoker_queue = m_queue->m_invoker_queue;

        const shared_future_base_pointer invoker = std::move(invoker_queue.front());
        invoker_queue.pop_front();

        invoker->m_status.store(RUNNING, std::memory_order_release);

        m_queue->pop_front_nullptr();
        lock.unlock();

        m_queue->invoke(invoker.get());

        return true;
    }

    /**
   * Thread is on hold if conditions are met.
   */
    [[nodiscard]] bool on_hold() const
    {
        return *m_thread_index < m_queue->m_number_of_threads &&
               !m_queue->m_destroying.load(std::memory_order_acquire) &&
               m_queue->m_invoker_queue.empty();
    }

    /**
   * We can continue popping elements if conditions are met.
   */
    [[nodiscard]] bool can_continue() const
    {
        return *m_thread_index < m_queue->m_number_of_threads && !m_queue->m_invoker_queue.empty();
    }

    threaded_callback_queue*         m_queue;
    std::shared_ptr<std::atomic_int> m_thread_index;
};

//-----------------------------------------------------------------------------
threaded_callback_queue::threaded_callback_queue()
{
    this->set_number_of_threads(1);
}

//-----------------------------------------------------------------------------
threaded_callback_queue::~threaded_callback_queue()
{
    {
        const std::scoped_lock destroy_lock(m_destroy_mutex);
        {
            const std::scoped_lock lock(m_mutex);
            m_destroying = true;
        }
    }

    m_condition_variable.notify_all();
    this->sync();
}

//-----------------------------------------------------------------------------
threaded_callback_queue* threaded_callback_queue::create()
{
    return new threaded_callback_queue();
}

//-----------------------------------------------------------------------------
void threaded_callback_queue::set_number_of_threads(int number_of_threads)
{
    this->push_control(
        [this, number_of_threads]()
        {
            const int size = static_cast<int>(m_threads.size());

            const std::scoped_lock destroy_lock(m_destroy_mutex);
            if (m_destroying)
            {
                return;
            }
            if (size == number_of_threads)
            {
                return;
            }
            if (size < number_of_threads)
            {
                m_number_of_threads = number_of_threads;

                std::generate_n(
                    std::back_inserter(m_threads),
                    number_of_threads - size,
                    [this]
                    {
                        auto thread_index =
                            std::make_shared<std::atomic_int>(static_cast<int>(m_threads.size()));
                        auto thread = std::thread(thread_worker(this, thread_index));
                        {
                            const std::scoped_lock thread_id_lock(m_thread_id_to_index_mutex);
                            m_thread_id_to_index.emplace(thread.get_id(), thread_index);
                        }
                        return thread;
                    });
            }
            else
            {
                {
                    std::unique_lock<std::mutex> lock(m_thread_id_to_index_mutex);
                    std::atomic_int&             thread_index =
                        *m_thread_id_to_index.at(std::this_thread::get_id());
                    if (thread_index && thread_index >= number_of_threads)
                    {
                        std::atomic_int& thread0_index =
                            *m_thread_id_to_index.at(m_threads[0].get_id());
                        lock.unlock();

                        std::swap(m_threads[thread_index], m_threads[0]);

                        const int tmp = thread0_index;
                        thread0_index.exchange(thread_index);
                        thread_index = tmp;
                    }
                }

                {
                    const std::scoped_lock lock(m_mutex);
                    m_number_of_threads = number_of_threads;
                }
                m_condition_variable.notify_all();
                this->sync(m_number_of_threads);

                m_threads.resize(number_of_threads);
            }
        });
}

//-----------------------------------------------------------------------------
void threaded_callback_queue::sync(int start_id)
{
    std::for_each(
        m_threads.begin() + start_id, m_threads.end(), [](std::thread& thread) { thread.join(); });
}

//-----------------------------------------------------------------------------
void threaded_callback_queue::pop_front_nullptr()
{
    while (!m_invoker_queue.empty() && !m_invoker_queue.front())
    {
        m_invoker_queue.pop_front();
    }
}

//-----------------------------------------------------------------------------
void threaded_callback_queue::invoke(shared_future_base* invoker)
{
    (*invoker)();
    this->signal_dependent_shared_futures(invoker);
}

//-----------------------------------------------------------------------------
void threaded_callback_queue::signal_dependent_shared_futures(shared_future_base* invoker)
{
    std::vector<shared_future_base_pointer> invokers_to_launch;
    {
        const std::scoped_lock lock(invoker->m_mutex);

        for (auto& dependent : invoker->m_dependents)
        {
            std::unique_lock<std::mutex> dependent_lock(dependent->m_mutex);
            --dependent->m_number_of_prior_shared_futures_remaining;
            if (dependent->m_status.load(std::memory_order_acquire) == ON_HOLD &&
                (dependent->m_number_of_prior_shared_futures_remaining == 0))
            {
                if (dependent->m_is_high_priority)
                {
                    dependent->m_status.store(RUNNING, std::memory_order_release);
                    dependent_lock.unlock();
                    this->invoke(dependent.get());
                }
                else
                {
                    dependent_lock.unlock();
                    invokers_to_launch.emplace_back(std::move(dependent));
                }
            }
        }
    }

    if (!invokers_to_launch.empty())
    {
        const std::scoped_lock lock(m_mutex);
        size_t index = m_invoker_queue.empty() ? static_cast<size_t>(invokers_to_launch.size())
                                               : m_invoker_queue.front()->m_invoker_index;
        for (shared_future_base_pointer& inv : invokers_to_launch)
        {
            assert(
                inv->m_status.load(std::memory_order_acquire) == ON_HOLD &&
                "Status should be ON_HOLD");
            inv->m_invoker_index = --index;

            const std::scoped_lock state_lock(inv->m_mutex);
            inv->m_status.store(ENQUEUED, std::memory_order_release);
            m_invoker_queue.emplace_front(std::move(inv));
        }
    }

    for (std::size_t i = 0; i < invokers_to_launch.size(); ++i)
    {
        m_condition_variable.notify_one();
    }
}

//-----------------------------------------------------------------------------
bool threaded_callback_queue::try_invoke(shared_future_base* invoker)
{
    if (![this, &invoker]
        {
            if (invoker->m_status.load(std::memory_order_relaxed) != ENQUEUED)
            {
                return false;
            }

            const std::scoped_lock lock(m_mutex);

            if (m_invoker_queue.empty())
            {
                return false;
            }

            const std::scoped_lock inv_lock(invoker->m_mutex);

            if (invoker->m_status.load(std::memory_order_acquire) != ENQUEUED)
            {
                return false;
            }

            const size_t index =
                invoker->m_invoker_index - m_invoker_queue.front()->m_invoker_index;

            const shared_future_base_pointer& result = m_invoker_queue[index];

            if (result.get() != invoker)
            {
                return false;
            }

            if (index == 0)
            {
                m_invoker_queue.pop_front();
                this->pop_front_nullptr();
            }
            invoker->m_status.store(RUNNING, std::memory_order_release);
            return true;
        }())
    {
        return false;
    }

    this->invoke(invoker);
    return true;
}
