

// This file incorporates code from the Visualization Toolkit (VTK) and remains subject to the BSD-3-Clause VTK license.

#pragma once

#include <memory>

#include "common/configure.h"
// For export macro
#include "common/macros.h"
#include "common/pointer.h"
#include "smp/Common/tools_impl.h"

#if QUARISMA_HAS_TBB
#include "smp/TBB/tools_impl.hxx"
#else
#include "smp/STDThread/tools_impl.hxx"
#endif

namespace quarisma
{
namespace detail
{
namespace smp
{

class QUARISMA_VISIBILITY tools_api
{
public:
    //--------------------------------------------------------------------------------
    QUARISMA_API static tools_api& GetInstance();

    //--------------------------------------------------------------------------------
    BackendType GetBackendType();

    //--------------------------------------------------------------------------------
    static const char* GetBackend();

    //--------------------------------------------------------------------------------
    bool SetBackend(const char* type);

    //--------------------------------------------------------------------------------
    void Initialize(int numThreads = 0);

    //--------------------------------------------------------------------------------
    int GetEstimatedNumberOfThreads();

    //--------------------------------------------------------------------------------
    int GetEstimatedDefaultNumberOfThreads();

    //------------------------------------------------------------------------------
    void SetNestedParallelism(bool isNested);

    //--------------------------------------------------------------------------------
    bool GetNestedParallelism();

    //--------------------------------------------------------------------------------
    bool IsParallelScope();

    //--------------------------------------------------------------------------------
    bool GetSingleThread();

    //--------------------------------------------------------------------------------
    int GetInternalDesiredNumberOfThread() { return this->DesiredNumberOfThread; }

    //------------------------------------------------------------------------------
    template <typename Config, typename T>
    void LocalScope(Config const& config, T&& lambda)
    {
        const Config oldConfig(*this);
        *this << config;

        struct ConfigGuard
        {
            tools_api*    api;
            const Config& old_config;
            ConfigGuard(tools_api* a, const Config& c) : api(a), old_config(c) {}
            ~ConfigGuard() { *api << old_config; }
        } guard(this, oldConfig);

        lambda();
    }

    //--------------------------------------------------------------------------------
    template <typename FunctorInternal>
    void For(int first, int last, int grain, FunctorInternal& fi)
    {
#if QUARISMA_HAS_TBB
        this->TBBBackend->For(first, last, grain, fi);
#else
        this->STDThreadBackend->For(first, last, grain, fi);
#endif
    }

    //--------------------------------------------------------------------------------
    template <typename InputIt, typename OutputIt, typename Functor>
    void Transform(InputIt inBegin, InputIt inEnd, OutputIt outBegin, Functor& transform)
    {
#if QUARISMA_HAS_TBB
        this->TBBBackend->Transform(inBegin, inEnd, outBegin, transform);
#else
        this->STDThreadBackend->Transform(inBegin, inEnd, outBegin, transform);
#endif
    }

    //--------------------------------------------------------------------------------
    template <typename InputIt1, typename InputIt2, typename OutputIt, typename Functor>
    void Transform(
        InputIt1 inBegin1, InputIt1 inEnd, InputIt2 inBegin2, OutputIt outBegin, Functor& transform)
    {
#if QUARISMA_HAS_TBB
        this->TBBBackend->Transform(inBegin1, inEnd, inBegin2, outBegin, transform);
#else
        this->STDThreadBackend->Transform(inBegin1, inEnd, inBegin2, outBegin, transform);
#endif
    }

    //--------------------------------------------------------------------------------
    template <typename Iterator, typename T>
    void Fill(Iterator begin, Iterator end, const T& value)
    {
#if QUARISMA_HAS_TBB
        this->TBBBackend->Fill(begin, end, value);
#else
        this->STDThreadBackend->Fill(begin, end, value);
#endif
    }

    //--------------------------------------------------------------------------------
    template <typename RandomAccessIterator>
    void Sort(RandomAccessIterator begin, RandomAccessIterator end)
    {
#if QUARISMA_HAS_TBB
        this->TBBBackend->Sort(begin, end);
#else
        this->STDThreadBackend->Sort(begin, end);
#endif
    }

    //--------------------------------------------------------------------------------
    template <typename RandomAccessIterator, typename Compare>
    void Sort(RandomAccessIterator begin, RandomAccessIterator end, Compare comp)
    {
#if QUARISMA_HAS_TBB
        this->TBBBackend->Sort(begin, end, comp);
#else
        this->STDThreadBackend->Sort(begin, end, comp);
#endif
    }

    // disable copying
    tools_api(tools_api const&)      = delete;
    void operator=(tools_api const&) = delete;

protected:
    //--------------------------------------------------------------------------------
    // Address the static initialization order 'fiasco' by implementing
    // the schwarz counter idiom.
    static void ClassInitialize();
    static void ClassFinalize();
    friend class toolsAPIInitialize;

private:
    //--------------------------------------------------------------------------------
    tools_api();

    //--------------------------------------------------------------------------------
    void RefreshNumberOfThread();

    //--------------------------------------------------------------------------------
    // This operator overload is used to unpack Config parameters and set them
    // in tools_api (e.g `*this << config;`)
    template <typename Config>
    tools_api& operator<<(Config const& config)
    {
        this->Initialize(config.MaxNumberOfThreads);
        this->SetBackend(config.Backend.c_str());
        this->SetNestedParallelism(config.NestedParallelism);
        return *this;
    }

    /**
   * Indicate which backend to use.
   */
    BackendType ActivatedBackend = DefaultBackend;

    /**
   * Max threads number
   */
    int DesiredNumberOfThread = 0;

#if QUARISMA_HAS_TBB
    /**
   * TBB backend
   */
    std::unique_ptr<tools_impl<BackendType::TBB>> TBBBackend;
#else
    /**
   * STDThread backend
   */
    std::unique_ptr<tools_impl<BackendType::STDThread>> STDThreadBackend;
#endif
};

//--------------------------------------------------------------------------------
class QUARISMA_VISIBILITY toolsAPIInitialize
{
public:
    QUARISMA_API toolsAPIInitialize();
    QUARISMA_API ~toolsAPIInitialize();
};

//--------------------------------------------------------------------------------
// This instance will show up in any translation unit that uses tools_api singleton.
// It will make sure tools_api is initialized before it is used and finalized when it
// is done being used.
static toolsAPIInitialize toolsAPIInitializer;

}  // namespace smp
}  // namespace detail
}  // namespace quarisma

/* QUARISMA-HeaderTest-Exclude: tools_api.h */
