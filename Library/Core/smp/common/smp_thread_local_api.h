// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef SMP_THREAD_LOCAL_API_H
#define SMP_THREAD_LOCAL_API_H

#include <array>
#include <iterator>
#include <memory>

#include "smp/common/smp_thread_local_impl_abstract.h"
#include "smp/common/smp_tools_api.h"  // For get_backend_type(), default_backend
#include "smp/smp.h"
#include "smp/std_thread/smp_thread_local_impl.h"
#if XSIGMA_HAS_TBB
#include "smp/tbb/smp_thread_local_impl.h"
#endif
#if XSIGMA_HAS_OPENMP
#include "smp/openmp/smp_thread_local_impl.h"
#endif

namespace conductor
{
namespace detail
{
namespace smp
{

template <typename T>
class smp_thread_local_api
{
    using thread_local_std_thread = smp_thread_local_impl<backend_type::std_thread, T>;
#if XSIGMA_HAS_TBB
    using thread_local_tbb = smp_thread_local_impl<backend_type::TBB, T>;
#endif
#if XSIGMA_HAS_OPENMP
    using thread_local_openmp = smp_thread_local_impl<backend_type::OpenMP, T>;
#endif
    typedef typename smp_thread_local_impl_abstract<T>::it_impl it_impl_abstract;

public:
    //--------------------------------------------------------------------------------
    smp_thread_local_api()
    {
        m_backends_impl[static_cast<int>(backend_type::std_thread)] =
            std::make_unique<thread_local_std_thread>();
#if XSIGMA_HAS_TBB
        m_backends_impl[static_cast<int>(backend_type::TBB)] = std::make_unique<thread_local_tbb>();
#endif
#if XSIGMA_HAS_OPENMP
        m_backends_impl[static_cast<int>(backend_type::OpenMP)] =
            std::make_unique<thread_local_openmp>();
#endif
    }

    //--------------------------------------------------------------------------------
    explicit smp_thread_local_api(const T& exemplar)
    {
        m_backends_impl[static_cast<int>(backend_type::std_thread)] =
            std::make_unique<thread_local_std_thread>(exemplar);

#if XSIGMA_HAS_TBB
        m_backends_impl[static_cast<int>(backend_type::TBB)] =
            std::make_unique<thread_local_tbb>(exemplar);
#endif
#if XSIGMA_HAS_OPENMP
        m_backends_impl[static_cast<int>(backend_type::OpenMP)] =
            std::make_unique<thread_local_openmp>(exemplar);
#endif
    }

    //--------------------------------------------------------------------------------
    T& local()
    {
        backend_type backend = this->get_smp_backend_type();
        return m_backends_impl[static_cast<int>(backend)]->local();
    }

    //--------------------------------------------------------------------------------
    size_t size()
    {
        backend_type backend = this->get_smp_backend_type();
        return m_backends_impl[static_cast<int>(backend)]->size();
    }

    //--------------------------------------------------------------------------------
    class iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = T*;
        using reference         = T&;

        iterator() = default;

        iterator(const iterator& other) : m_impl_abstract(other.m_impl_abstract->clone()) {}

        iterator& operator=(const iterator& other)
        {
            if (this != &other)
            {
                m_impl_abstract = other.m_impl_abstract->clone();
            }
            return *this;
        }

        iterator& operator++()
        {
            m_impl_abstract->increment();
            return *this;
        }

        iterator operator++(int)
        {
            iterator copy = *this;
            m_impl_abstract->increment();
            return copy;
        }

        bool operator==(const iterator& other) const
        {
            return m_impl_abstract->compare(other.m_impl_abstract.get());
        }

        bool operator!=(const iterator& other) const
        {
            return !m_impl_abstract->compare(other.m_impl_abstract.get());
        }

        T& operator*() { return m_impl_abstract->get_content(); }

        T* operator->() { return m_impl_abstract->get_content_ptr(); }

    private:
        std::unique_ptr<it_impl_abstract> m_impl_abstract;

        friend class smp_thread_local_api<T>;
    };

    //--------------------------------------------------------------------------------
    iterator begin()
    {
        backend_type backend = this->get_smp_backend_type();
        iterator     iter;
        iter.m_impl_abstract = m_backends_impl[static_cast<int>(backend)]->begin();
        return iter;
    }

    //--------------------------------------------------------------------------------
    iterator end()
    {
        backend_type backend = this->get_smp_backend_type();
        iterator     iter;
        iter.m_impl_abstract = m_backends_impl[static_cast<int>(backend)]->end();
        return iter;
    }

    // disable copying
    smp_thread_local_api(const smp_thread_local_api&)            = delete;
    smp_thread_local_api& operator=(const smp_thread_local_api&) = delete;

private:
    std::array<std::unique_ptr<smp_thread_local_impl_abstract<T>>, SMP_MAX_BACKENDS_NB>
        m_backends_impl;

    //--------------------------------------------------------------------------------
    backend_type get_smp_backend_type()
    {
        auto& smp_tools_api_instance = smp_tools_api::get_instance();
        return smp_tools_api_instance.get_backend_type();
    }
};

}  // namespace smp
}  // namespace detail
}  // namespace conductor

#endif
/* VTK-HeaderTest-Exclude: smp_thread_local_api.h */
