#pragma once

/**
 * @file async_handle.h
 * @brief Async Handle for XSigma Asynchronous Parallel Operations
 *
 * This file provides the async_handle class template, which represents a handle
 * to an asynchronous parallel operation. It provides a future-like interface
 * compatible with XSigma's no-exception error handling policy.
 *
 * DESIGN PHILOSOPHY:
 * ==================
 * Unlike std::future which uses exceptions for error reporting, async_handle
 * provides an exception-free interface using explicit error checking methods.
 * This aligns with XSigma's no-exception policy for public APIs.
 *
 * KEY FEATURES:
 * =============
 * - Non-blocking status checking (is_ready())
 * - Blocking wait with optional timeout (wait(), wait_for())
 * - Result retrieval with default value on error (get())
 * - Explicit error checking (has_error(), get_error())
 * - Thread-safe operations
 * - Move-only semantics (like std::future)
 *
 * USAGE PATTERN:
 * ==============
 * @code
 * // Launch async operation
 * auto handle = xsigma::async_parallel_for(0, 1000, 100, [](int64_t b, int64_t e) {
 *     // Parallel work...
 * });
 *
 * // Do other work...
 *
 * // Wait for completion
 * handle.wait();
 *
 * // Check for errors
 * if (handle.has_error()) {
 *     XSIGMA_LOG_ERROR("Async operation failed: {}", handle.get_error());
 * }
 * @endcode
 *
 * ERROR HANDLING:
 * ===============
 * The async_handle provides three ways to handle errors:
 * 1. Check has_error() after wait()
 * 2. Check has_error() after get() (get returns default value on error)
 * 3. Use wait_for() with timeout to detect hanging operations
 *
 * THREAD SAFETY:
 * ==============
 * - All methods are thread-safe
 * - Multiple threads can call is_ready(), has_error(), get_error()
 * - Only one thread should call get() (consumes the result)
 * - wait() can be called by multiple threads (all will block until ready)
 *
 * CODING STANDARDS:
 * =================
 * - Follows XSigma C++ coding standards
 * - snake_case naming convention
 * - noexcept methods where possible
 * - No exceptions in public API
 * - Move-only semantics (non-copyable)
 */

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>

#include "common/export.h"
#include "common/macros.h"

namespace xsigma
{

// Forward declarations
template <typename T>
class async_handle;

namespace internal
{

/**
 * @brief Internal shared state for async operations
 *
 * This class manages the synchronization and state for async operations.
 * It is shared between the async operation (producer) and the async_handle
 * (consumer) via std::shared_ptr.
 *
 * Thread Safety: All methods are thread-safe (protected by mutex)
 * Lifetime: Managed by std::shared_ptr, destroyed when last reference is released
 *
 * @tparam T Result type (void for operations without return value)
 */
template <typename T>
class async_state
{
public:
    async_state() = default;

    // Non-copyable and non-movable (managed via shared_ptr)
    async_state(const async_state&)            = delete;
    async_state(async_state&&)                 = delete;
    async_state& operator=(const async_state&) = delete;
    async_state& operator=(async_state&&)      = delete;

    /**
     * @brief Checks if the async operation has completed
     * @return true if ready (completed or error), false if still running
     * Thread Safety: Thread-safe (uses atomic)
     */
    bool is_ready() const noexcept { return ready_.load(std::memory_order_acquire); }

    /**
     * @brief Waits for the async operation to complete
     * Blocks the calling thread until the operation finishes
     * Thread Safety: Thread-safe (multiple threads can wait)
     */
    void wait() const noexcept
    {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] { return ready_.load(std::memory_order_acquire); });
    }

    /**
     * @brief Waits for the operation with a timeout
     * @param timeout_ms Timeout in milliseconds
     * @return true if completed within timeout, false if timed out
     * Thread Safety: Thread-safe
     */
    bool wait_for(int64_t timeout_ms) const noexcept
    {
        std::unique_lock<std::mutex> lock(mtx_);
        return cv_.wait_for(
            lock,
            std::chrono::milliseconds(timeout_ms),
            [this] { return ready_.load(std::memory_order_acquire); });
    }

    /**
     * @brief Checks if an error occurred
     * @return true if error occurred, false otherwise
     * Thread Safety: Thread-safe (uses atomic)
     */
    bool has_error() const noexcept { return has_error_.load(std::memory_order_acquire); }

    /**
     * @brief Gets the error message
     * @return Error message string (empty if no error)
     * Thread Safety: Thread-safe (protected by mutex)
     */
    std::string get_error() const noexcept
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return error_msg_;
    }

    /**
     * @brief Sets the result value and marks as ready (non-void version)
     * @param value Result value to store
     * Thread Safety: Thread-safe, should only be called once
     */
    template <typename U = T, typename std::enable_if<!std::is_void<U>::value, int>::type = 0>
    void set_value(U value) noexcept
    {
        std::lock_guard<std::mutex> lock(mtx_);
        value_     = std::move(value);
        has_error_ = false;
        ready_.store(true, std::memory_order_release);
        cv_.notify_all();
    }

    /**
     * @brief Marks as ready without value (void version)
     * Thread Safety: Thread-safe, should only be called once
     */
    template <typename U = T, typename std::enable_if<std::is_void<U>::value, int>::type = 0>
    void set_ready() noexcept
    {
        std::lock_guard<std::mutex> lock(mtx_);
        has_error_ = false;
        ready_.store(true, std::memory_order_release);
        cv_.notify_all();
    }

    /**
     * @brief Sets error state and marks as ready
     * @param msg Error message
     * Thread Safety: Thread-safe, should only be called once
     */
    void set_error(std::string msg) noexcept
    {
        std::lock_guard<std::mutex> lock(mtx_);
        error_msg_ = std::move(msg);
        has_error_.store(true, std::memory_order_release);
        ready_.store(true, std::memory_order_release);
        cv_.notify_all();
    }

    /**
     * @brief Gets the result value (non-void version)
     * @return Result value, or default-constructed T if error occurred
     * Thread Safety: Thread-safe, but should only be called once
     *
     * @note Caller should check has_error() before relying on the result
     */
    template <typename U = T, typename std::enable_if<!std::is_void<U>::value, int>::type = 0>
    U get_value() noexcept
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (has_error_.load(std::memory_order_acquire))
        {
            return T{};  // Return default-constructed value on error
        }
        return std::move(value_);
    }

private:
    mutable std::mutex              mtx_;               ///< Protects shared state
    mutable std::condition_variable cv_;                ///< Signals completion
    std::atomic<bool>               ready_{false};      ///< Ready flag (atomic for lock-free check)
    std::atomic<bool>               has_error_{false};  ///< Error flag (atomic for lock-free check)
    std::string                     error_msg_;         ///< Error message (empty if no error)
    T                               value_;             ///< Result value (only for non-void T)
};

// Specialization for void (no value storage needed)
template <>
class async_state<void>
{
public:
    async_state() = default;

    async_state(const async_state&)            = delete;
    async_state(async_state&&)                 = delete;
    async_state& operator=(const async_state&) = delete;
    async_state& operator=(async_state&&)      = delete;

    bool is_ready() const noexcept { return ready_.load(std::memory_order_acquire); }

    void wait() const noexcept
    {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] { return ready_.load(std::memory_order_acquire); });
    }

    bool wait_for(int64_t timeout_ms) const noexcept
    {
        std::unique_lock<std::mutex> lock(mtx_);
        return cv_.wait_for(
            lock,
            std::chrono::milliseconds(timeout_ms),
            [this] { return ready_.load(std::memory_order_acquire); });
    }

    bool has_error() const noexcept { return has_error_.load(std::memory_order_acquire); }

    std::string get_error() const noexcept
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return error_msg_;
    }

    void set_ready() noexcept
    {
        std::lock_guard<std::mutex> lock(mtx_);
        has_error_ = false;
        ready_.store(true, std::memory_order_release);
        cv_.notify_all();
    }

    void set_error(std::string msg) noexcept
    {
        std::lock_guard<std::mutex> lock(mtx_);
        error_msg_ = std::move(msg);
        has_error_.store(true, std::memory_order_release);
        ready_.store(true, std::memory_order_release);
        cv_.notify_all();
    }

private:
    mutable std::mutex              mtx_;
    mutable std::condition_variable cv_;
    std::atomic<bool>               ready_{false};
    std::atomic<bool>               has_error_{false};
    std::string                     error_msg_;
};

}  // namespace internal

/**
 * @brief Handle for asynchronous parallel operations
 *
 * Represents a handle to an asynchronous operation (async_parallel_for or
 * async_parallel_reduce). Provides methods to wait for completion, check
 * status, and retrieve results.
 *
 * This class is similar to std::future but uses exception-free error handling
 * compatible with XSigma's no-exception policy.
 *
 * Thread Safety: All methods are thread-safe
 * Lifetime: Move-only (like std::future)
 * Coding Standard: Follows XSigma C++ standards (snake_case, noexcept)
 *
 * @tparam T Result type (void for async_parallel_for)
 */
template <typename T>
class XSIGMA_VISIBILITY async_handle
{
public:
    /**
     * @brief Default constructor creates invalid handle
     */
    async_handle() = default;

    /**
     * @brief Constructs handle from shared state (internal use only)
     * @param state Shared state pointer
     */
    explicit async_handle(std::shared_ptr<internal::async_state<T>> state)
        : state_(std::move(state))
    {
    }

    /**
     * @brief Move constructor
     */
    async_handle(async_handle&&) noexcept = default;

    /**
     * @brief Move assignment operator
     */
    async_handle& operator=(async_handle&&) noexcept = default;

    // Non-copyable (like std::future)
    async_handle(const async_handle&)            = delete;
    async_handle& operator=(const async_handle&) = delete;

    /**
     * @brief Checks if this handle is valid
     * @return true if valid (associated with async operation), false otherwise
     * Thread Safety: Thread-safe
     */
    bool valid() const noexcept { return state_ != nullptr; }

    /**
     * @brief Checks if the async operation has completed
     * @return true if completed, false if still running
     * Thread Safety: Thread-safe
     *
     * @note Non-blocking operation
     * @note Returns false if handle is invalid
     */
    bool is_ready() const noexcept { return valid() && state_->is_ready(); }

    /**
     * @brief Waits for the async operation to complete
     *
     * Blocks the calling thread until the operation finishes (either
     * successfully or with error).
     *
     * Thread Safety: Thread-safe (multiple threads can wait)
     *
     * @note No-op if handle is invalid
     * @note Use has_error() after wait() to check for errors
     */
    void wait() const noexcept
    {
        if (valid())
        {
            state_->wait();
        }
    }

    /**
     * @brief Waits for the operation with a timeout
     *
     * Blocks the calling thread until the operation finishes or timeout expires.
     *
     * @param timeout_ms Timeout in milliseconds (must be > 0)
     * @return true if completed within timeout, false if timed out
     * Thread Safety: Thread-safe
     *
     * @note Returns false immediately if handle is invalid
     *
     * Example:
     * @code
     * auto handle = async_parallel_for(...);
     * if (!handle.wait_for(5000)) {
     *     XSIGMA_LOG_ERROR("Operation timed out after 5 seconds");
     * }
     * @endcode
     */
    bool wait_for(int64_t timeout_ms) const noexcept
    {
        if (!valid())
        {
            return false;
        }
        return state_->wait_for(timeout_ms);
    }

    /**
     * @brief Checks if an error occurred during execution
     * @return true if error occurred, false otherwise
     * Thread Safety: Thread-safe
     *
     * @note Returns false if handle is invalid
     * @note Should be called after wait() or get()
     */
    bool has_error() const noexcept { return valid() && state_->has_error(); }

    /**
     * @brief Gets error message if an error occurred
     * @return Error message string, empty if no error or invalid handle
     * Thread Safety: Thread-safe
     *
     * Example:
     * @code
     * handle.wait();
     * if (handle.has_error()) {
     *     XSIGMA_LOG_ERROR("Error: {}", handle.get_error());
     * }
     * @endcode
     */
    std::string get_error() const noexcept
    {
        if (!valid())
        {
            return "";
        }
        return state_->get_error();
    }

    /**
     * @brief Gets the result (blocking if not ready)
     *
     * Waits for the operation to complete and returns the result.
     * If an error occurred, returns default-constructed T.
     *
     * @return Result value, or default-constructed T on error or invalid handle
     * Thread Safety: Thread-safe, but only one thread should call get()
     *
     * @note Blocks until operation completes
     * @note Use has_error() to distinguish between valid result and error
     * @note For void specialization, this just waits
     *
     * Example:
     * @code
     * auto handle = async_parallel_reduce(...);
     * int sum = handle.get();
     * if (handle.has_error()) {
     *     XSIGMA_LOG_ERROR("Reduction failed: {}", handle.get_error());
     * } else {
     *     XSIGMA_LOG_INFO("Sum: {}", sum);
     * }
     * @endcode
     */
    template <typename U = T, typename std::enable_if<!std::is_void<U>::value, int>::type = 0>
    U get() noexcept
    {
        if (!valid())
        {
            return T{};
        }
        state_->wait();
        return state_->get_value();
    }

    /**
     * @brief Waits for completion (void specialization)
     *
     * For async_handle<void>, get() just waits for completion.
     *
     * Thread Safety: Thread-safe
     */
    template <typename U = T, typename std::enable_if<std::is_void<U>::value, int>::type = 0>
    void get() noexcept
    {
        wait();
    }

private:
    std::shared_ptr<internal::async_state<T>> state_;  ///< Shared state (null if invalid)
};

}  // namespace xsigma
