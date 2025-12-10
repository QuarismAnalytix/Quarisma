// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME smp_thread_local - A TBB based thread local storage implementation.

#ifndef TBB_SMP_THREAD_LOCAL_IMPL_H
#define TBB_SMP_THREAD_LOCAL_IMPL_H

#include "smp/common/smp_thread_local_impl_abstract.h"

#ifdef _MSC_VER
#pragma push_macro("__TBB_NO_IMPLICIT_LINKAGE")
#define __TBB_NO_IMPLICIT_LINKAGE 1
#endif

#include <tbb/enumerable_thread_specific.h>

#ifdef _MSC_VER
#pragma pop_macro("__TBB_NO_IMPLICIT_LINKAGE")
#endif

#include <iterator>
#include <utility>  // For std::move

namespace conductor
{
namespace detail
{
namespace smp
{

template <typename T>
class smp_thread_local_impl<backend_type::TBB, T> : public smp_thread_local_impl_abstract<T>
{
    typedef tbb::enumerable_thread_specific<T>                  tls_type;
    typedef typename tls_type::iterator                         tls_iter;
    typedef typename smp_thread_local_impl_abstract<T>::it_impl it_impl_abstract;

public:
    smp_thread_local_impl() = default;

    explicit smp_thread_local_impl(const T& exemplar) : m_internal(exemplar) {}

    T& local() override { return m_internal.local(); }

    size_t size() const override { return m_internal.size(); }

    class it_impl : public smp_thread_local_impl_abstract<T>::it_impl
    {
    public:
        void increment() override { ++m_iter; }

        bool compare(it_impl_abstract* other) override
        {
            return m_iter == static_cast<it_impl*>(other)->m_iter;
        }

        T& get_content() override { return *m_iter; }

        T* get_content_ptr() override { return &*m_iter; }

    protected:
        it_impl* clone_impl() const override { return new it_impl(*this); };

    private:
        tls_iter m_iter;

        friend class smp_thread_local_impl<backend_type::TBB, T>;
    };

    std::unique_ptr<it_impl_abstract> begin() override
    {
        auto iter    = std::make_unique<it_impl>();
        iter->m_iter = m_internal.begin();
        return iter;
    };

    std::unique_ptr<it_impl_abstract> end() override
    {
        auto iter    = std::make_unique<it_impl>();
        iter->m_iter = m_internal.end();
        return iter;
    }

private:
    tls_type m_internal;

    // disable copying
    smp_thread_local_impl(const smp_thread_local_impl&) = delete;
    void operator=(const smp_thread_local_impl&)        = delete;
};

}  // namespace smp
}  // namespace detail
}  // namespace conductor

#endif
