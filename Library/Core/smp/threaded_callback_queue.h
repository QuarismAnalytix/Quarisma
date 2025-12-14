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
 *
 * Portions of this code are based on VTK (Visualization Toolkit):

 *   Licensed under BSD-3-Clause
 */

/**
 * @class threaded_callback_queue
 * @brief simple threaded callback queue
 *
 * This callback queue executes pushed functions and functors on threads whose
 * purpose is to execute those functions.
 * By default, one thread is created by this class, so it is advised to set `m_number_of_threads`.
 * Upon destruction of an instance of this callback queue, remaining unexecuted threads are
 * executed.
 *
 * When a task is pushed, a `shared_future` is returned. This instance can be used to get the
 * returned value when the task is finished, and provides functionalities to synchronize the main
 * thread with the status of its associated task.
 *
 * All public methods of this class are thread safe.
 */

#ifndef THREADED_CALLBACK_QUEUE_H
#define THREADED_CALLBACK_QUEUE_H

#include <algorithm>           // For any_of
#include <array>               // For array
#include <atomic>              // For atomic_bool
#include <cassert>             // For assert
#include <condition_variable>  // For condition variable
#include <deque>               // For deque
#include <functional>          // For greater
#include <memory>              // For unique_ptr, shared_ptr
#include <mutex>               // For mutex
#include <thread>              // For thread
#include <tuple>               // For tuple
#include <type_traits>         // For type_traits
#include <unordered_map>       // For unordered_map
#include <unordered_set>       // For unordered_set
#include <utility>             // For forward
#include <vector>              // For vector

#include "common/export.h"

class XSIGMA_VISIBILITY threaded_callback_queue
{
private:
    /**
   * Helper to extract the parameter types of a function (passed by template)
   */
    template <class FT>
    struct signature;

    /**
   * Helper that dereferences the input type `T`. If `T == Object`, or
   * `T == Object*` or `T == std::unique_ptr<Object>`, then `Type` is of type `Object`.
   */
    template <class T, class DummyT = std::nullptr_t>
    struct dereference
    {
        struct type;
    };

    /**
   * Convenient typedef to help lighten the code.
   */
    template <class T>
    using dereferenced_type = typename std::decay<typename dereference<T>::type>::type;

    /**
   * This resolves to the return type of the template function `FT`.
   */
    template <class FT>
    using invoke_result = typename signature<dereferenced_type<FT>>::invoke_result;

    /**
   * This class wraps the returned value and gives access to it.
   */
    template <class ReturnT, bool IsLValueReference = std::is_lvalue_reference<ReturnT>::value>
    class return_value_wrapper;

public:
    XSIGMA_API static threaded_callback_queue* create();
    XSIGMA_API virtual ~threaded_callback_queue();

    XSIGMA_API threaded_callback_queue();

    /**
   * `shared_future_base` is the base block to store, run, get the returned value of the tasks that
   * are pushed in the queue.
   */
    class XSIGMA_VISIBILITY shared_future_base
    {
    public:
        shared_future_base()
            : m_number_of_prior_shared_futures_remaining(0),
              m_invoker_index(0),
              m_status(CONSTRUCTING)
        {
        }

        virtual ~shared_future_base() = default;

        /**
     * Blocks current thread until the task associated with this future has terminated.
     */
        XSIGMA_API virtual void wait() const
        {
            if (m_status == READY)
            {
                return;
            }
            std::unique_lock<std::mutex> lock(m_mutex);
            m_condition_variable.wait(lock, [this] { return m_status == READY; });
        }

        friend class threaded_callback_queue;

    protected:
        /**
     * Exclusive binary mask giving the status of the current invoker sharing this state.
     */
        std::atomic_int m_status;

        /**
     * List of futures which are depending on us.
     */
        std::vector<std::shared_ptr<shared_future_base>> m_dependents;

        mutable std::mutex              m_mutex;
        mutable std::condition_variable m_condition_variable;

    private:
        /**
     * This runs the stored task.
     */
        virtual void operator()() = 0;

        /**
     * Number of futures that need to terminate before we can run.
     */
        std::atomic_int m_number_of_prior_shared_futures_remaining;

        /**
     * Index that is set by the invoker to this shared state.
     */
        size_t m_invoker_index;

        /**
     * When set to true, when this invoker becomes ready, whoever picked this invoker must directly
     * run it.
     */
        bool m_is_high_priority = false;

        shared_future_base(const shared_future_base& other) = delete;
        void operator=(const shared_future_base& other)     = delete;
    };

    /**
   * A `shared_future` is an object returned by the methods `push` and `push_dependent`.
   */
    template <class ReturnT>
    class shared_future : public shared_future_base
    {
    public:
        using return_lvalue_ref = typename return_value_wrapper<ReturnT>::return_lvalue_ref;
        using return_const_lvalue_ref =
            typename return_value_wrapper<ReturnT>::return_const_lvalue_ref;

        shared_future() = default;

        /**
     * This returns the return value of the pushed function.
     */
        return_lvalue_ref get();

        /**
     * This returns the return value of the pushed function.
     */
        return_const_lvalue_ref get() const;

        friend class threaded_callback_queue;

    private:
        return_value_wrapper<ReturnT> m_return_value;

        shared_future(const shared_future<ReturnT>& other)  = delete;
        void operator=(const shared_future<ReturnT>& other) = delete;
    };

    using shared_future_base_pointer = std::shared_ptr<shared_future_base>;
    template <class ReturnT>
    using shared_future_pointer = std::shared_ptr<shared_future<ReturnT>>;

    /**
   * Pushes a function f to be passed args... as arguments.
   */
    template <class FT, class... ArgsT>
    XSIGMA_API shared_future_pointer<invoke_result<FT>> push(FT&& f, ArgsT&&... args);

    /**
   * This method behaves the same way `push` does, with the addition of a container of `futures`.
   */
    template <class SharedFutureContainerT, class FT, class... ArgsT>
    XSIGMA_API shared_future_pointer<invoke_result<FT>> push_dependent(
        SharedFutureContainerT&& prior_shared_futures, FT&& f, ArgsT&&... args);

    /**
   * This method blocks the current thread until all the tasks associated with each shared future
   * inside `prior_shared_future` has terminated.
   */
    template <class SharedFutureContainerT>
    XSIGMA_API void wait(SharedFutureContainerT&& prior_shared_future);

    ///@{
    /**
   * Get the returned value from the task associated with the input future.
   */
    template <class ReturnT>
    XSIGMA_API typename shared_future<ReturnT>::return_lvalue_ref get(
        shared_future_pointer<ReturnT>& future);
    template <class ReturnT>
    XSIGMA_API typename shared_future<ReturnT>::return_const_lvalue_ref get(
        const shared_future_pointer<ReturnT>& future);
    ///@}

    /**
   * Sets the number of threads.
   */
    XSIGMA_API void set_number_of_threads(int number_of_threads);

    /**
   * Returns the number of allocated threads.
   */
    XSIGMA_API int get_number_of_threads() const { return m_number_of_threads; }

private:
    ///@{
    /**
   * An `invoker` subclasses `shared_future`. It provides storage and capabilities to run the
   * input function with the given parameters.
   */
    template <class FT, class... ArgsT>
    class invoker;
    ///@}

    struct invoker_impl;

    template <class FT, class... ArgsT>
    using invoker_pointer = std::shared_ptr<invoker<FT, ArgsT...>>;

    class thread_worker;

    friend class thread_worker;

    /**
   * Status that an invoker can be in.
   */
    enum status
    {
        CONSTRUCTING = 0x00,
        ON_HOLD      = 0x01,
        ENQUEUED     = 0x02,
        RUNNING      = 0x04,
        READY        = 0x08
    };

    XSIGMA_API void sync(int start_id = 0);
    XSIGMA_API void pop_front_nullptr();
    XSIGMA_API void signal_dependent_shared_futures(shared_future_base* invoker);

    template <class SharedFutureContainerT, class InvokerT>
    void handle_dependent_invoker(
        SharedFutureContainerT&& prior_shared_futures, InvokerT&& invoker);

    XSIGMA_API void invoke(shared_future_base* invoker);
    XSIGMA_API bool try_invoke(shared_future_base* invoker);

    template <class FT, class... ArgsT>
    void push_control(FT&& f, ArgsT&&... args);

    template <class SharedFutureContainerT>
    static bool must_wait(SharedFutureContainerT&& prior_shared_futures);

    std::deque<shared_future_base_pointer> m_invoker_queue;
    std::mutex                             m_mutex;
    std::mutex                             m_control_mutex;
    std::mutex                             m_destroy_mutex;
    std::mutex                             m_thread_id_to_index_mutex;
    std::condition_variable                m_condition_variable;
    std::atomic_bool                       m_destroying{false};
    std::atomic_int                        m_number_of_threads;
    std::vector<std::thread>               threads_;
    std::unordered_map<std::thread::id, std::shared_ptr<std::atomic_int>> m_thread_id_to_index;
    std::unordered_set<shared_future_base_pointer>                        m_control_futures;

    threaded_callback_queue(const threaded_callback_queue&) = delete;
    void operator=(const threaded_callback_queue&)          = delete;
};

// ========== Template Implementation ==========

//-----------------------------------------------------------------------------
template <>
struct threaded_callback_queue::return_value_wrapper<void, false>
{
    using return_lvalue_ref       = void;
    using return_const_lvalue_ref = void;

    void get() const {}
};

//=============================================================================
template <class ReturnT>
struct threaded_callback_queue::return_value_wrapper<ReturnT, true /* IsLValueReference */>
{
    using return_value_impl       = return_value_wrapper<ReturnT, false>;
    using return_lvalue_ref       = ReturnT&;
    using return_const_lvalue_ref = const ReturnT&;

    return_value_wrapper() = default;
    return_value_wrapper(ReturnT& value)
        : m_value(std::unique_ptr<return_value_impl>(new return_value_impl(value)))
    {
    }

    ReturnT&       get() { return m_value->get(); }
    const ReturnT& get() const { return m_value->get(); }

    std::unique_ptr<return_value_impl> m_value;
};

//=============================================================================
template <class ReturnT>
struct threaded_callback_queue::return_value_wrapper<ReturnT, false /* IsLValueReference */>
{
    using return_lvalue_ref       = ReturnT&;
    using return_const_lvalue_ref = const ReturnT&;

    return_value_wrapper() = default;
    template <class ReturnTT>
    return_value_wrapper(ReturnTT&& value) : m_value(std::forward<ReturnTT>(value))
    {
    }

    ReturnT&       get() { return m_value; }
    const ReturnT& get() const { return m_value; }

    ReturnT m_value;
};

//-----------------------------------------------------------------------------
template <class ReturnT>
typename threaded_callback_queue::shared_future<ReturnT>::return_lvalue_ref
threaded_callback_queue::shared_future<ReturnT>::get()
{
    this->wait();
    return m_return_value.get();
}

//-----------------------------------------------------------------------------
template <class ReturnT>
typename threaded_callback_queue::shared_future<ReturnT>::return_const_lvalue_ref
threaded_callback_queue::shared_future<ReturnT>::get() const
{
    this->wait();
    return m_return_value.get();
}

//=============================================================================
struct threaded_callback_queue::invoker_impl
{
    template <std::size_t... Is>
    struct integer_sequence;
    template <std::size_t N, std::size_t... Is>
    struct make_integer_sequence;

    template <class T>
    static decltype(*std::declval<T&>(), std::true_type{}) can_be_dereferenced(std::nullptr_t);
    template <class>
    static std::false_type can_be_dereferenced(...);

    template <class T, class CanBeDereferencedT = decltype(can_be_dereferenced<T>(nullptr))>
    struct dereference_impl;

    template <class FT, std::size_t N = signature<typename std::decay<FT>::type>::args_size>
    using args_tuple = typename signature<typename std::decay<FT>::type>::args_tuple;

    template <class FT, std::size_t I>
    using arg_type = typename std::tuple_element<I, args_tuple<FT>>::type;

    template <class FunctionArgsTupleT, class InputArgsTupleT, std::size_t... Is>
    static std::tuple<typename std::conditional<
        std::is_lvalue_reference<typename std::tuple_element<Is, FunctionArgsTupleT>::type>::value,
        typename std::decay<typename std::tuple_element<Is, FunctionArgsTupleT>::type>::type,
        typename std::decay<typename std::tuple_element<Is, InputArgsTupleT>::type>::type>::type...>
        get_static_cast_args_tuple(integer_sequence<Is...>);

    template <class FunctionArgsTupleT, class... InputArgsT>
    using static_cast_args_tuple =
        decltype(get_static_cast_args_tuple<FunctionArgsTupleT, std::tuple<InputArgsT...>>(
            make_integer_sequence<sizeof...(InputArgsT)>()));

    template <bool IsMemberFunctionPointer, class... ArgsT>
    class invoker_handle;

    template <class ReturnT>
    struct invoker_helper
    {
        template <class InvokerT>
        static void invoke(InvokerT&& invoker, shared_future<ReturnT>* future)
        {
            future->m_return_value = return_value_wrapper<ReturnT>(invoker());
            future->m_status.store(READY, std::memory_order_release);
            future->m_condition_variable.notify_all();
        }
    };
};

//=============================================================================
template <>
struct threaded_callback_queue::invoker_impl::invoker_helper<void>
{
    template <class InvokerT>
    static void invoke(InvokerT&& invoker, shared_future<void>* future)
    {
        invoker();
        future->m_status.store(READY, std::memory_order_release);
        future->m_condition_variable.notify_all();
    }
};

//=============================================================================
// For lambdas or std::function
template <class ReturnT, class... ArgsT>
struct threaded_callback_queue::signature<ReturnT(ArgsT...)>
{
    using args_tuple                       = std::tuple<ArgsT...>;
    using invoke_result                    = ReturnT;
    static constexpr std::size_t args_size = sizeof...(ArgsT);
};

//=============================================================================
// For methods inside a class ClassT
template <class ClassT, class ReturnT, class... ArgsT>
struct threaded_callback_queue::signature<ReturnT (ClassT::*)(ArgsT...)>
{
    using args_tuple                       = std::tuple<ArgsT...>;
    using invoke_result                    = ReturnT;
    static constexpr std::size_t args_size = sizeof...(ArgsT);
};

//=============================================================================
// For const methods inside a class ClassT
template <class ClassT, class ReturnT, class... ArgsT>
struct threaded_callback_queue::signature<ReturnT (ClassT::*)(ArgsT...) const>
{
    using args_tuple                       = std::tuple<ArgsT...>;
    using invoke_result                    = ReturnT;
    static constexpr std::size_t args_size = sizeof...(ArgsT);
};

//=============================================================================
// For function pointers
template <class ReturnT, class... ArgsT>
struct threaded_callback_queue::signature<ReturnT (*)(ArgsT...)>
{
    using args_tuple                       = std::tuple<ArgsT...>;
    using invoke_result                    = ReturnT;
    static constexpr std::size_t args_size = sizeof...(ArgsT);
};

//=============================================================================
// For function pointers
template <class ReturnT, class... ArgsT>
struct threaded_callback_queue::signature<ReturnT (&)(ArgsT...)>
{
    using args_tuple                       = std::tuple<ArgsT...>;
    using invoke_result                    = ReturnT;
    static constexpr std::size_t args_size = sizeof...(ArgsT);
};

//=============================================================================
// For functors
template <class FT>
struct threaded_callback_queue::signature
    : threaded_callback_queue::signature<decltype(&FT::operator())>
{
};

//=============================================================================
template <std::size_t... Is>
struct threaded_callback_queue::invoker_impl::integer_sequence
{
};

//=============================================================================
template <std::size_t N, std::size_t... Is>
struct threaded_callback_queue::invoker_impl::make_integer_sequence
    : threaded_callback_queue::invoker_impl::make_integer_sequence<N - 1, N - 1, Is...>
{
};

//=============================================================================
template <std::size_t... Is>
struct threaded_callback_queue::invoker_impl::make_integer_sequence<0, Is...>
    : threaded_callback_queue::invoker_impl::integer_sequence<Is...>
{
};

//=============================================================================
template <class T>
struct threaded_callback_queue::invoker_impl::dereference_impl<T, std::true_type>
{
    using type = typename std::remove_pointer<decltype(*std::declval<T>())>::type;
    static type& get(T& instance) { return *instance; }
};

//=============================================================================
template <class T>
struct threaded_callback_queue::invoker_impl::dereference_impl<T, std::false_type>
{
    using type = T;
    static type& get(T& instance) { return instance; }
};

//=============================================================================
template <class T>
struct threaded_callback_queue::dereference<T, std::nullptr_t>
{
    using type = typename invoker_impl::dereference_impl<T>::type;
};

//=============================================================================
template <class FT, class ObjectT, class... ArgsT>
class threaded_callback_queue::invoker_impl::
    invoker_handle<true /* IsMemberFunctionPointer */, FT, ObjectT, ArgsT...>
{
public:
    template <class FTT, class ObjectTT, class... ArgsTT>
    invoker_handle(FTT&& f, ObjectTT&& instance, ArgsTT&&... args)
        : m_function(std::forward<FTT>(f)),
          m_instance(std::forward<ObjectTT>(instance)),
          m_args(std::forward<ArgsTT>(args)...)
    {
    }

    invoke_result<FT> operator()()
    {
        return this->invoke_impl(make_integer_sequence<sizeof...(ArgsT)>());
    }

private:
    template <std::size_t... Is>
    invoke_result<FT> invoke_impl(integer_sequence<Is...>)
    {
        auto& deref = dereference_impl<ObjectT>::get(m_instance);
        return (deref.*m_function)(static_cast<arg_type<FT, Is>>(std::get<Is>(m_args))...);
    }

    FT                                               m_function;
    typename std::decay<ObjectT>::type               m_instance;
    static_cast_args_tuple<args_tuple<FT>, ArgsT...> m_args;
};

//=============================================================================
template <class FT, class... ArgsT>
class threaded_callback_queue::invoker_impl::
    invoker_handle<false /* IsMemberFunctionPointer */, FT, ArgsT...>
{
public:
    template <class FTT, class... ArgsTT>
    invoker_handle(FTT&& f, ArgsTT&&... args)
        : m_function(std::forward<FTT>(f)), m_args(std::forward<ArgsTT>(args)...)
    {
    }

    invoke_result<FT> operator()()
    {
        return this->invoke_impl(make_integer_sequence<sizeof...(ArgsT)>());
    }

private:
    template <std::size_t... Is>
    invoke_result<FT> invoke_impl(integer_sequence<Is...>)
    {
        auto& f = dereference_impl<FT>::get(m_function);
        return f(static_cast<arg_type<decltype(f), Is>>(std::get<Is>(m_args))...);
    }

    typename std::decay<FT>::type                                                m_function;
    static_cast_args_tuple<args_tuple<typename dereference<FT>::type>, ArgsT...> m_args;
};

//=============================================================================
template <class FT, class... ArgsT>
class threaded_callback_queue::invoker
    : public threaded_callback_queue::shared_future<threaded_callback_queue::invoke_result<FT>>
{
public:
    template <class... ArgsTT>
    static invoker<FT, ArgsT...>* create(ArgsTT&&... args)
    {
        return new invoker<FT, ArgsT...>(std::forward<ArgsTT>(args)...);
    }

    template <class... ArgsTT>
    invoker(ArgsTT&&... args) : m_impl(std::forward<ArgsTT>(args)...)
    {
    }

    void operator()() override
    {
        assert(
            this->m_status.load(std::memory_order_relaxed) == RUNNING &&
            "Status should be RUNNING");
        invoker_impl::invoker_helper<invoke_result<FT>>::invoke(m_impl, this);
    }

    friend class threaded_callback_queue;

private:
    invoker_impl::invoker_handle<std::is_member_function_pointer<FT>::value, FT, ArgsT...> m_impl;

    invoker(const invoker<FT, ArgsT...>& other)        = delete;
    void operator=(const invoker<FT, ArgsT...>& other) = delete;
};

//-----------------------------------------------------------------------------
template <class SharedFutureContainerT, class InvokerT>
void threaded_callback_queue::handle_dependent_invoker(
    SharedFutureContainerT&& prior_shared_futures, InvokerT&& invoker)
{
    if (!prior_shared_futures.empty())
    {
        for (const auto& prior : prior_shared_futures)
        {
            if (prior->m_status.load(std::memory_order_acquire) == READY)
            {
                continue;
            }

            std::unique_lock<std::mutex> lock(prior->m_mutex);
            if (prior->m_status.load(std::memory_order_acquire) != READY)
            {
                prior->m_dependents.emplace_back(invoker);
                lock.unlock();
                invoker->m_number_of_prior_shared_futures_remaining.fetch_add(
                    1, std::memory_order_release);
            }
        }
    }

    std::unique_lock<std::mutex> lock(invoker->m_mutex);
    if (invoker->m_number_of_prior_shared_futures_remaining)
    {
        invoker->m_status.store(ON_HOLD, std::memory_order_release);
    }
    else
    {
        invoker->m_status.store(RUNNING, std::memory_order_release);
        lock.unlock();
        this->invoke(std::forward<InvokerT>(invoker).get());
    }
}

//-----------------------------------------------------------------------------
template <class SharedFutureContainerT>
bool threaded_callback_queue::must_wait(SharedFutureContainerT&& prior_shared_futures)
{
    return std::any_of(
        prior_shared_futures.begin(),
        prior_shared_futures.end(),
        [](const auto& prior) { return prior->m_status.load(std::memory_order_acquire) != READY; });
}

//-----------------------------------------------------------------------------
template <class SharedFutureContainerT>
void threaded_callback_queue::wait(SharedFutureContainerT&& prior_shared_futures)
{
    bool                    must_wait_flag         = false;
    SharedFutureContainerT& prior_shared_futures_r = prior_shared_futures;

    for (shared_future_base* prior : prior_shared_futures)
    {
        switch (prior->m_status.load(std::memory_order_acquire))
        {
        case RUNNING:
        case ON_HOLD:
        case CONSTRUCTING:
            must_wait_flag = true;
            break;
        case ENQUEUED:
            must_wait_flag |= !this->try_invoke(prior);
            break;
        }
    }

    if (!must_wait_flag || !this->must_wait(prior_shared_futures_r))
    {
        return;
    }

    auto empty_lambda = [] {};
    auto invoker_ptr  = invoker_pointer<decltype(empty_lambda)>(
        invoker<decltype(empty_lambda)>::create(std::move(empty_lambda)));
    invoker_ptr->m_is_high_priority = true;
    this->handle_dependent_invoker(prior_shared_futures_r, invoker_ptr);
    invoker_ptr->wait();
}

//-----------------------------------------------------------------------------
template <class ReturnT>
typename threaded_callback_queue::shared_future<ReturnT>::return_lvalue_ref
threaded_callback_queue::get(shared_future_pointer<ReturnT>& future)
{
    this->wait(std::array<shared_future<ReturnT>*, 1>{future.get()});
    return future->get();
}

//-----------------------------------------------------------------------------
template <class ReturnT>
typename threaded_callback_queue::shared_future<ReturnT>::return_const_lvalue_ref
threaded_callback_queue::get(const shared_future_pointer<ReturnT>& future)
{
    this->wait(std::array<const shared_future<ReturnT>*, 1>{future.get()});
    return future->get();
}

//-----------------------------------------------------------------------------
template <class SharedFutureContainerT, class FT, class... ArgsT>
threaded_callback_queue::shared_future_pointer<threaded_callback_queue::invoke_result<FT>>
threaded_callback_queue::push_dependent(
    SharedFutureContainerT&& prior_shared_futures, FT&& f, ArgsT&&... args)
{
    SharedFutureContainerT& prior_shared_futures_r = prior_shared_futures;
    if (!this->must_wait(prior_shared_futures_r))
    {
        return this->push(std::forward<FT>(f), std::forward<ArgsT>(args)...);
    }

    using invoker_pointer_type = invoker_pointer<FT, ArgsT...>;
    auto invoker_ptr           = invoker_pointer_type(
        invoker<FT, ArgsT...>::create(std::forward<FT>(f), std::forward<ArgsT>(args)...));

    this->push(
        &threaded_callback_queue::
            handle_dependent_invoker<SharedFutureContainerT, invoker_pointer_type>,
        this,
        prior_shared_futures_r,
        invoker_ptr);

    return invoker_ptr;
}

//-----------------------------------------------------------------------------
template <class FT, class... ArgsT>
void threaded_callback_queue::push_control(FT&& f, ArgsT&&... args)
{
    struct worker
    {
        void operator()(threaded_callback_queue* self, FT&& _f, ArgsT&&... _args)
        {
            _f(std::forward<ArgsT>(_args)...);
            std::lock_guard<std::mutex> lock(self->m_control_mutex);
            self->m_control_futures.erase(m_future);
        }

        shared_future_base_pointer m_future;
    };

    worker w;
    using invoker_pointer_type = invoker_pointer<worker, threaded_callback_queue*, FT, ArgsT...>;
    auto invoker_ptr           = invoker_pointer_type(
        invoker<worker, threaded_callback_queue*, FT, ArgsT...>::create(
            w, this, std::forward<FT>(f), std::forward<ArgsT>(args)...));
    w.m_future = invoker_ptr;

    auto local_control_futures = [this, &invoker_ptr]
    {
        std::lock_guard<std::mutex> lock(m_control_mutex);
        auto                        result = m_control_futures;
        m_control_futures.emplace(invoker_ptr);
        return result;
    }();

    if (!this->must_wait(local_control_futures))
    {
        if (threads_.empty())
        {
            invoker_ptr->m_status.store(RUNNING, std::memory_order_relaxed);
            (*invoker_ptr)();
            return;
        }

        {
            std::lock_guard<std::mutex> invoker_lock(invoker_ptr->m_mutex);
            invoker_ptr->m_status.store(ENQUEUED, std::memory_order_release);

            std::lock_guard<std::mutex> lock(m_mutex);
            invoker_ptr->m_invoker_index =
                m_invoker_queue.empty() ? 0 : m_invoker_queue.front()->m_invoker_index - 1;
            m_invoker_queue.emplace_front(invoker_ptr);
        }
        m_condition_variable.notify_one();
        return;
    }

    invoker_ptr->m_is_high_priority = true;
    this->handle_dependent_invoker(local_control_futures, invoker_ptr);
}

//-----------------------------------------------------------------------------
template <class FT, class... ArgsT>
threaded_callback_queue::shared_future_pointer<threaded_callback_queue::invoke_result<FT>>
threaded_callback_queue::push(FT&& f, ArgsT&&... args)
{
    auto invoker_ptr = invoker_pointer<FT, ArgsT...>(
        invoker<FT, ArgsT...>::create(std::forward<FT>(f), std::forward<ArgsT>(args)...));
    invoker_ptr->m_status.store(ENQUEUED, std::memory_order_release);

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        invoker_ptr->m_invoker_index =
            m_invoker_queue.empty() ? 0 : m_invoker_queue.back()->m_invoker_index + 1;
        m_invoker_queue.emplace_back(invoker_ptr);
    }

    m_condition_variable.notify_one();
    return invoker_ptr;
}

#endif
