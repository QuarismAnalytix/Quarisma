
// .NAME thread_local_impl - A thread local storage implementation using
// platform specific facilities.

// This file incorporates code from the Visualization Toolkit (VTK) and remains subject to the BSD-3-Clause VTK license.

#pragma once

#include <iterator>

#include "smp/Common/thread_local_impl_abstract.h"
#include "smp/STDThread/thread_local_backend.h"
#include "smp/STDThread/tools_impl.hxx"

namespace quarisma
{
namespace detail
{
namespace smp
{

template <typename T>
class thread_local_impl<BackendType::STDThread, T> : public thread_local_impl_abstract<T>
{
    using ItImplAbstract = typename thread_local_impl_abstract<T>::ItImpl;

public:
    thread_local_impl() : Backend(GetNumberOfThreadsSTDThread()) {}

    explicit thread_local_impl(const T& exemplar)
        : Backend(GetNumberOfThreadsSTDThread()), Exemplar(exemplar)
    {
    }

    ~thread_local_impl() override
    {
        quarisma::detail::smp::STDThread::ThreadSpecificStorageIterator it;
        it.SetThreadSpecificStorage(this->Backend);
        for (it.SetToBegin(); !it.GetAtEnd(); it.Forward())
        {
            delete reinterpret_cast<T*>(it.GetStorage());
        }
    }

    T& Local() override
    {
        quarisma::detail::smp::STDThread::StoragePointerType& ptr   = this->Backend.GetStorage();
        auto*                                               local = reinterpret_cast<T*>(ptr);
        if (ptr == nullptr)
        {
            ptr = local = new T(this->Exemplar);
        }
        return *local;
    }

    size_t size() const override { return this->Backend.GetSize(); }

    class ItImpl : public thread_local_impl_abstract<T>::ItImpl
    {
    public:
        void Increment() override { this->Impl.Forward(); }

        bool Compare(ItImplAbstract* other) override
        {
            return this->Impl == static_cast<ItImpl*>(other)->Impl;
        }

        T& GetContent() override { return *reinterpret_cast<T*>(this->Impl.GetStorage()); }

        T* GetContentPtr() override { return reinterpret_cast<T*>(this->Impl.GetStorage()); }

    protected:
        ItImpl* CloneImpl() const override { return new ItImpl(*this); }

    private:
        quarisma::detail::smp::STDThread::ThreadSpecificStorageIterator Impl;

        friend class thread_local_impl<BackendType::STDThread, T>;
    };

    std::unique_ptr<ItImplAbstract> begin() override
    {
        auto it = std::make_unique<ItImpl>();
        it->Impl.SetThreadSpecificStorage(this->Backend);
        it->Impl.SetToBegin();
        return it;
    }

    std::unique_ptr<ItImplAbstract> end() override
    {
        auto it = std::make_unique<ItImpl>();
        it->Impl.SetThreadSpecificStorage(this->Backend);
        it->Impl.SetToEnd();
        return it;
    }

private:
    quarisma::detail::smp::STDThread::ThreadSpecific Backend;
    T                                              Exemplar;

    // disable copying
    thread_local_impl(const thread_local_impl&) = delete;
    void operator=(const thread_local_impl&)    = delete;
};

}  // namespace smp
}  // namespace detail
}  // namespace quarisma

/* QUARISMA-HeaderTest-Exclude: thread_local_impl.h */
