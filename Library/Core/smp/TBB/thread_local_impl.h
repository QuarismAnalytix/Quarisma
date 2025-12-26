
// .NAME quarisma_thread_local - A TBB based thread local storage implementation.

// This file incorporates code from the Visualization Toolkit (VTK) and remains subject to the BSD-3-Clause VTK license.

#ifndef TBBthread_local_impl_h
#define TBBthread_local_impl_h

#include "smp/Common/thread_local_impl_abstract.h"
//to avoid tbb12.lib linkage issue
#ifdef _MSC_VER
#pragma push_macro("__TBB_NO_IMPLICIT_LINKAGE")
#define __TBB_NO_IMPLICIT_LINKAGE 1
#endif

#include <tbb/enumerable_thread_specific.h>
#undef min
#undef max
#ifdef _MSC_VER
#pragma pop_macro("__TBB_NO_IMPLICIT_LINKAGE")
#endif

#include <iterator>

namespace quarisma
{
namespace detail
{
namespace smp
{

template <typename T>
class thread_local_impl<BackendType::TBB, T> : public thread_local_impl_abstract<T>
{
    using TLS            = tbb::enumerable_thread_specific<T>;
    using TLSIter        = typename TLS::iterator;
    using ItImplAbstract = typename thread_local_impl_abstract<T>::ItImpl;

public:
    thread_local_impl() = default;

    explicit thread_local_impl(const T& exemplar) : Internal(exemplar) {}

    T& Local() override { return this->Internal.local(); }

    size_t size() const override { return this->Internal.size(); }

    class ItImpl : public thread_local_impl_abstract<T>::ItImpl
    {
    public:
        void Increment() override { ++this->Iter; }

        bool Compare(ItImplAbstract* other) override
        {
            return this->Iter == static_cast<ItImpl*>(other)->Iter;
        }

        T& GetContent() override { return *this->Iter; }

        T* GetContentPtr() override { return &*this->Iter; }

    protected:
        ItImpl* CloneImpl() const override { return new ItImpl(*this); };

    private:
        TLSIter Iter;

        friend class thread_local_impl<BackendType::TBB, T>;
    };

    std::unique_ptr<ItImplAbstract> begin() override
    {
        auto iter  = std::make_unique<ItImpl>();
        iter->Iter = this->Internal.begin();
        return iter;
    };

    std::unique_ptr<ItImplAbstract> end() override
    {
        auto iter  = std::make_unique<ItImpl>();
        iter->Iter = this->Internal.end();
        return iter;
    }

private:
    TLS Internal;

    // disable copying
    thread_local_impl(const thread_local_impl&) = delete;
    void operator=(const thread_local_impl&)    = delete;
};

}  // namespace smp
}  // namespace detail
}  // namespace quarisma

#endif
