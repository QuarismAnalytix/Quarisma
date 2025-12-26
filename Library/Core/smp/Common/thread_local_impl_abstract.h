

// This file incorporates code from the Visualization Toolkit (VTK) and remains subject to the BSD-3-Clause VTK license.

#pragma once

#include <memory>

#include "smp/Common/tools_impl.h"

namespace quarisma
{
namespace detail
{
namespace smp
{

template <typename T>
class thread_local_impl_abstract
{
public:
    virtual ~thread_local_impl_abstract() = default;

    virtual T& Local() = 0;

    virtual size_t size() const = 0;

    class ItImpl
    {
    public:
        ItImpl()                             = default;
        virtual ~ItImpl()                    = default;
        ItImpl(const ItImpl&)                = default;
        ItImpl(ItImpl&&) noexcept            = default;
        ItImpl& operator=(const ItImpl&)     = default;
        ItImpl& operator=(ItImpl&&) noexcept = default;

        virtual void Increment() = 0;

        virtual bool Compare(ItImpl* other) = 0;

        virtual T& GetContent() = 0;

        virtual T* GetContentPtr() = 0;

        std::unique_ptr<ItImpl> Clone() const { return std::unique_ptr<ItImpl>(CloneImpl()); }

    protected:
        virtual ItImpl* CloneImpl() const = 0;
    };

    virtual std::unique_ptr<ItImpl> begin() = 0;

    virtual std::unique_ptr<ItImpl> end() = 0;
};

template <BackendType Backend, typename T>
class thread_local_impl : public thread_local_impl_abstract<T>
{
};

}  // namespace smp
}  // namespace detail
}  // namespace quarisma

/* QUARISMA-HeaderTest-Exclude: thread_local_impl_abstract.h */
