#include "profiler/itt/itt_observer.h"

#include "parallel/thread_local_debug_info.h"
#include "profiler/base/base.h"
#include "profiler/common/util.h"

namespace xsigma::profiler::impl
{

struct ITTThreadLocalState : ProfilerStateBase
{
    explicit ITTThreadLocalState(const ProfilerConfig& config) : ProfilerStateBase(config)
    {
        // Only `report_input_shapes` makes sense in this context.
        XSIGMA_CHECK(!config.profile_memory);
        XSIGMA_CHECK(!config.with_stack);
        XSIGMA_CHECK(!config.with_flops);
        XSIGMA_CHECK(!config.with_modules);
    }
    ~ITTThreadLocalState() override = default;

    ActiveProfilerType profilerType() override { return ActiveProfilerType::ITT; }

    void reportMemoryUsage(
        void* /*ptr*/,
        int64_t /*alloc_size*/,
        size_t /*total_allocated*/,
        size_t /*total_reserved*/,
        xsigma::device_option /*device*/) override
    {
    }

    static ITTThreadLocalState* getTLS()
    {
        auto* tls = ProfilerStateBase::get(/*global=*/false);
        return static_cast<ITTThreadLocalState*>(tls);
    }
};

template <bool report_input_shapes>
static std::unique_ptr<xsigma::ObserverContext> enterITT(const xsigma::RecordFunction& fn)
{
    if (ITTThreadLocalState::getTLS() != nullptr)
    {
        xsigma::profiler::impl::ittStubs()->rangePush(fn.name());
    }
    return nullptr;
}

void pushITTCallbacks(
    const ProfilerConfig& config, const std::unordered_set<xsigma::RecordScope>& scopes)
{
    XSIGMA_CHECK(
        xsigma::profiler::impl::ittStubs()->enabled(),
        "Can't use ITT profiler - XSigma was compiled without ITT");

    xsigma::thread_local_debug_info::_push(
        xsigma::DebugInfoKind::PROFILER_STATE, std::make_shared<ITTThreadLocalState>(config));

    auto* state_ptr = ITTThreadLocalState::getTLS();
    XSIGMA_CHECK(state_ptr, "Expected profiler state set");

    auto handle = xsigma::addThreadLocalCallback(
        xsigma::RecordFunctionCallback(
            state_ptr->config().report_input_shapes ? &enterITT</*report_input_shapes=*/true>
                                                    : &enterITT</*report_input_shapes=*/false>,
            [](const xsigma::RecordFunction&, xsigma::ObserverContext*)
            { xsigma::profiler::impl::ittStubs()->rangePop(); })
            .needsInputs(config.report_input_shapes)
            .scopes(scopes));
    state_ptr->setCallbackHandle(handle);
}

}  // namespace xsigma::profiler::impl
