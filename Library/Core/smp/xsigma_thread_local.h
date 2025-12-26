
// This file incorporates code from the Visualization Toolkit (VTK) and remains subject to the BSD-3-Clause VTK license.

/**
 * @class   quarisma_thread_local
 * @brief   Thread local storage for QUARISMA objects.
 *
 * A thread local object is one that maintains a copy of an object of the
 * template type for each thread that processes data. quarisma_thread_local
 * creates storage for all threads but the actual objects are created
 * the first time Local() is called. Note that some of the quarisma_thread_local
 * API is not thread safe. It can be safely used in a multi-threaded
 * environment because Local() returns storage specific to a particular
 * thread, which by default will be accessed sequentially. It is also
 * thread-safe to iterate over quarisma_thread_local as long as each thread
 * creates its own iterator and does not change any of the thread local
 * objects.
 *
 * A common design pattern in using a thread local storage object is to
 * write/accumulate data to local object when executing in parallel and
 * then having a sequential code block that iterates over the whole storage
 * using the iterators to do the final accumulation.
 *
 * @warning
 * There is absolutely no guarantee to the order in which the local objects
 * will be stored and hence the order in which they will be traversed when
 * using iterators. You should not even assume that two quarisma_thread_local
 * populated in the same parallel section will be populated in the same
 * order. For example, consider the following
 *
 * @code
 * quarisma_thread_local<int> Foo;
 * quarisma_thread_local<int> Bar;
 * class AFunctor
 * {
 *    void Initialize() const
 *    {
 *        int& foo = Foo.Local();
 *        int& bar = Bar.Local();
 *        foo = random();
 *        bar = foo;
 *    }
 *
 *    void operator()(int, int) const
 *    {}
 * };
 *
 * AFunctor functor;
 * quarismaParalllelUtilities::For(0, 100000, functor);
 *
 * quarisma_thread_local<int>::iterator itr1 = Foo.begin();
 * quarisma_thread_local<int>::iterator itr2 = Bar.begin();
 * while (itr1 != Foo.end())
 * {
 *   assert(*itr1 == *itr2);
 *   ++itr1; ++itr2;
 * }
 * @endcode
 *
 * @warning
 * It is possible and likely that the assert() will fail using the TBB
 * backend. So if you need to store values related to each other and
 * iterate over them together, use a struct or class to group them together
 * and use a thread local of that class.
 *
 * @sa
 * thread_localObject
 */

#pragma once

#include "smp/Common/thread_local_api.h"

namespace quarisma
{
template <typename T>
class quarisma_thread_local
{
public:
    /**
   * Default constructor. Creates a default exemplar.
   */
    quarisma_thread_local() = default;

    /**
   * Constructor that allows the specification of an exemplar object
   * which is used when constructing objects when Local() is first called.
   * Note that a copy of the exemplar is created using its copy constructor.
   */
    explicit quarisma_thread_local(const T& exemplar) : ThreadLocalAPI(exemplar) {}

    /**
   * This needs to be called mainly within a threaded execution path.
   * It will create a new object (local to the thread so each thread
   * get their own when calling Local) which is a copy of exemplar as passed
   * to the constructor (or a default object if no exemplar was provided)
   * the first time it is called. After the first time, it will return
   * the same object.
   */
    T& Local() { return this->ThreadLocalAPI.Local(); }

    /**
   * Return the number of thread local objects that have been initialized
   */
    size_t size() { return this->ThreadLocalAPI.size(); }

    /**
   * Subset of the standard iterator API.
   * The most common design pattern is to use iterators in a sequential
   * code block and to use only the thread local objects in parallel
   * code blocks.
   * It is thread safe to iterate over the thread local containers
   * as long as each thread uses its own iterator and does not modify
   * objects in the container.
   */
    using iterator = typename quarisma::detail::smp::thread_local_api<T>::iterator;

    /**
   * Returns a new iterator pointing to the beginning of
   * the local storage container. Thread safe.
   */
    iterator begin() { return this->ThreadLocalAPI.begin(); }

    /**
   * Returns a new iterator pointing to past the end of
   * the local storage container. Thread safe.
   */
    iterator end() { return this->ThreadLocalAPI.end(); }

private:
    quarisma::detail::smp::thread_local_api<T> ThreadLocalAPI;

    // disable copying
    quarisma_thread_local(const quarisma_thread_local&) = delete;
    void operator=(const quarisma_thread_local&)      = delete;
};

}  // namespace quarisma
// QUARISMA-HeaderTest-Exclude: quarisma_thread_local.h
