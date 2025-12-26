

// This file incorporates code from the Visualization Toolkit (VTK) and remains subject to the BSD-3-Clause VTK license.

#pragma once

#include <array>
#include <cstddef>
#include <iterator>
#include <memory>

#include "common/configure.h"
#include "common/macros.h"
#include "smp/Common/thread_local_impl_abstract.h"
#include "smp/Common/tools_api.h"  // For GetBackendType(), DefaultBackend
#include "smp/STDThread/thread_local_impl.h"
#if QUARISMA_HAS_TBB
#include "smp/TBB/thread_local_impl.h"
#endif

namespace quarisma
{
namespace detail
{
namespace smp
{

template <typename T>
class thread_local_api
{
    using ThreadLocalSTDThread = thread_local_impl<BackendType::STDThread, T>;
#if QUARISMA_HAS_TBB
    using ThreadLocalTBB = thread_local_impl<BackendType::TBB, T>;
#endif

    using ItImplAbstract = typename thread_local_impl_abstract<T>::ItImpl;

public:
    //--------------------------------------------------------------------------------
    thread_local_api()
    {
        this->BackendsImpl[static_cast<std::size_t>(BackendType::STDThread)] =
            std::make_unique<ThreadLocalSTDThread>();
#if QUARISMA_HAS_TBB
        this->BackendsImpl[static_cast<std::size_t>(BackendType::TBB)] =
            std::make_unique<ThreadLocalTBB>();
#endif
    }

    //--------------------------------------------------------------------------------
    explicit thread_local_api(const T& exemplar)
    {
        this->BackendsImpl[static_cast<std::size_t>(BackendType::STDThread)] =
            std::make_unique<ThreadLocalSTDThread>(exemplar);
#if QUARISMA_HAS_TBB
        this->BackendsImpl[static_cast<std::size_t>(BackendType::TBB)] =
            std::make_unique<ThreadLocalTBB>(exemplar);
#endif
    }

    //--------------------------------------------------------------------------------
    T& Local()
    {
        BackendType backendType = this->GetSMPBackendType();
        return this->BackendsImpl[static_cast<std::size_t>(backendType)]->Local();
    }

    //--------------------------------------------------------------------------------
    size_t size()
    {
        BackendType backendType = this->GetSMPBackendType();
        return this->BackendsImpl[static_cast<std::size_t>(backendType)]->size();
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

        iterator(const iterator& other) : ImplAbstract(other.ImplAbstract->Clone()) {}

        iterator& operator=(const iterator& other)
        {
            if (this != &other)
            {
                this->ImplAbstract = other.ImplAbstract->Clone();
            }
            return *this;
        }

        iterator& operator++()
        {
            this->ImplAbstract->Increment();
            return *this;
        }

        iterator operator++(int)
        {
            iterator copy = *this;
            this->ImplAbstract->Increment();
            return copy;
        }

        bool operator==(const iterator& other)
        {
            return this->ImplAbstract->Compare(other.ImplAbstract.get());
        }

        bool operator!=(const iterator& other)
        {
            return !this->ImplAbstract->Compare(other.ImplAbstract.get());
        }

        T& operator*() { return this->ImplAbstract->GetContent(); }

        T* operator->() { return this->ImplAbstract->GetContentPtr(); }

    private:
        std::unique_ptr<ItImplAbstract> ImplAbstract;

        friend class thread_local_api<T>;
    };

    //--------------------------------------------------------------------------------
    iterator begin()
    {
        BackendType backendType = this->GetSMPBackendType();
        iterator    iter;
        iter.ImplAbstract = this->BackendsImpl[static_cast<std::size_t>(backendType)]->begin();
        return iter;
    }

    //--------------------------------------------------------------------------------
    iterator end()
    {
        BackendType backendType = this->GetSMPBackendType();
        iterator    iter;
        iter.ImplAbstract = this->BackendsImpl[static_cast<std::size_t>(backendType)]->end();
        return iter;
    }

    // disable copying
    thread_local_api(const thread_local_api&)            = delete;
    thread_local_api& operator=(const thread_local_api&) = delete;

private:
    std::array<std::unique_ptr<thread_local_impl_abstract<T>>, BackendSlotCount> BackendsImpl;

    //--------------------------------------------------------------------------------
    BackendType GetSMPBackendType()
    {
        auto& SMPToolsAPI = tools_api::GetInstance();
        return SMPToolsAPI.GetBackendType();
    }
};

}  // namespace smp
}  // namespace detail
}  // namespace quarisma

/* QUARISMA-HeaderTest-Exclude: thread_local_api.h */
