#include "profiler/base/nvtx_observer.h"

#include "profiler/base/base.h"
#include "profiler/base/thread_local_debug_info.h"
#include "profiler/common/util.h"

namespace xsigma::profiler::impl
{

struct NVTXThreadLocalState : ProfilerStateBase
{
    explicit NVTXThreadLocalState(const ProfilerConfig& config) : ProfilerStateBase(config)
    {
        // Only `report_input_shapes` makes sense in this context.
        XSIGMA_CHECK(!config.profile_memory);
        XSIGMA_CHECK(!config.with_stack);
        XSIGMA_CHECK(!config.with_flops);
        XSIGMA_CHECK(!config.with_modules);
    }
    ~NVTXThreadLocalState() override = default;

    ActiveProfilerType profilerType() override { return ActiveProfilerType::NVTX; }

    void reportMemoryUsage(
        void* /*ptr*/,
        int64_t /*alloc_size*/,
        size_t /*total_allocated*/,
        size_t /*total_reserved*/,
        xsigma::device_option /*device*/) override
    {
    }

    static NVTXThreadLocalState* getTLS()
    {
        auto* tls = ProfilerStateBase::get(/*global=*/false);
        return static_cast<NVTXThreadLocalState*>(tls);
    }
    static std::pair<xsigma::RecordFunctionHandle, int> getOpIdFromInput(
        const xsigma::Tensor& tensor);

    void setProducerTensorMap(
        xsigma::TensorImpl* tensor, xsigma::RecordFunctionHandle op_id, int output_nr)
    {
        producer_tensor_map_[static_cast<void*>(tensor)] =
            std::pair<xsigma::RecordFunctionHandle, int>{op_id, output_nr};
    }

protected:
    // Maps the address of an output Tensor to a unique op id and output
    // index of the tensor.
    // xsigma::TensorImpl* is the actual type of the key, but using void*
    // to indicate the pointer is just being used as a key
    // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
    std::unordered_map<void*, std::pair<xsigma::RecordFunctionHandle, int>> producer_tensor_map_;
};

std::pair<xsigma::RecordFunctionHandle, int> NVTXThreadLocalState::getOpIdFromInput(
    const xsigma::Tensor& /*tensor*/)
{
    std::pair<xsigma::RecordFunctionHandle, int> producer_op_pair(0, -1);
    //if (tensor.defined())
    //{
    //    xsigma::TensorImpl* ten_addr = tensor.unsafeGetTensorImpl();
    //    // See if Address is in the map already
    //    if (producer_tensor_map_.count((void*)ten_addr) > 0)
    //    {
    //        producer_op_pair = producer_tensor_map_[(void*)ten_addr];
    //    }
    //}
    return producer_op_pair;
}

// static std::list<std::pair<xsigma::RecordFunctionHandle, int>> flattenOpIdList(
//     const xsigma::List<xsigma::IValue>& list)
// {
//     std::list<std::pair<xsigma::RecordFunctionHandle, int>> input_op_id_list;
//     auto state_ptr = NVTXThreadLocalState::getTLS();
//     XSIGMA_CHECK(state_ptr, "Expected profiler state set");
//     for (const xsigma::IValue& input : list)
//     {
//         if (input.isTensor())
//         {
//             const xsigma::Tensor& tensor           = input.toTensor();
//             auto                  producer_op_pair = state_ptr->getOpIdFromInput(tensor);
//             input_op_id_list.push_back(producer_op_pair);
//         }
//     }
//     return input_op_id_list;
// }

static std::list<std::pair<xsigma::RecordFunctionHandle, int>> getInputTensorOpIds()
{
    // Note: undefined_op_pair was used in commented-out code below
    // std::pair<xsigma::RecordFunctionHandle, int> const undefined_op_pair(0, -1);
    std::list<std::pair<xsigma::RecordFunctionHandle, int>> input_producer_ops_;
    /*auto state_ptr = NVTXThreadLocalState::getTLS();
    XSIGMA_CHECK(state_ptr, "Expected profiler state set");
    for (const xsigma::IValue& input_item : fn.inputs())
    {
        if (input_item.isTensor())
        {
            const xsigma::Tensor& tensor        = input_item.toTensor();
            auto                  producer_pair = state_ptr->getOpIdFromInput(tensor);
            input_producer_ops_.push_back(producer_pair);
        }
        else
        {
            if (input_item.isList())
            {
                std::list<std::pair<xsigma::RecordFunctionHandle, int>> tmp_op_ids =
                    flattenOpIdList(input_item.toList());
                // Extend the current sizes array by the array returned from input sizes
                if (!tmp_op_ids.empty())
                {
                    input_producer_ops_.splice(input_producer_ops_.end(), tmp_op_ids);
                }
                else
                {
                    input_producer_ops_.emplace_back(undefined_op_pair);
                }
            }
            else
            {
                input_producer_ops_.emplace_back(undefined_op_pair);
            }
        }
    }*/
    return input_producer_ops_;
}

//static void updateOutputTensorTracker(const xsigma::RecordFunction& fn)
//{
//    int  output_nr = 0;
//    auto state_ptr = NVTXThreadLocalState::getTLS();
//    XSIGMA_CHECK(state_ptr, "Expected profiler state set");
//    for (const xsigma::IValue& s_tensor : fn.outputs())
//    {
//        if (s_tensor.isTensor())
//        {
//            const xsigma::Tensor& tensor = s_tensor.toTensor();
//            if (tensor.defined())
//            {
//                auto ten_addr = tensor.unsafeGetTensorImpl();
//                state_ptr->setProducerTensorMap(ten_addr, fn.handle(), output_nr);
//            }
//        }
//        output_nr++;
//    }
//}

template <bool report_input_shapes>
static std::unique_ptr<xsigma::ObserverContext> enterNVTX(const xsigma::RecordFunction& fn)
{
    if (NVTXThreadLocalState::getTLS() != nullptr)
    {
        auto input_op_ids = getInputTensorOpIds();
        xsigma::profiler::impl::cudaStubs()->rangePush(
            xsigma::profiler::impl::getNvtxStr(
                fn.name(),
                fn.seqNr(),
                report_input_shapes ? xsigma::profiler::impl::inputSizes(fn, true)
                                    : std::vector<std::vector<int64_t>>(),
                fn.handle(),
                report_input_shapes ? input_op_ids
                                    : std::list<std::pair<xsigma::RecordFunctionHandle, int>>())
                .c_str());
    }
    return nullptr;
}

void pushNVTXCallbacks(
    const ProfilerConfig& config, const std::unordered_set<xsigma::RecordScope>& scopes)
{
    XSIGMA_CHECK(
        xsigma::profiler::impl::cudaStubs()->enabled(),
        "Can't use NVTX profiler - XSigma was compiled without CUDA");

    xsigma::thread_local_debug_info::_push(
        xsigma::DebugInfoKind::PROFILER_STATE, std::make_shared<NVTXThreadLocalState>(config));

    auto* state_ptr = NVTXThreadLocalState::getTLS();
    XSIGMA_CHECK(state_ptr, "Expected profiler state set");

    auto handle = xsigma::addThreadLocalCallback(
        xsigma::RecordFunctionCallback(
            state_ptr->config().report_input_shapes ? &enterNVTX</*report_input_shapes=*/true>
                                                    : &enterNVTX</*report_input_shapes=*/false>,
            [](const xsigma::RecordFunction& /*fn*/, xsigma::ObserverContext* /*ctx*/)
            {
                xsigma::profiler::impl::cudaStubs()->rangePop();
                //updateOutputTensorTracker(fn);
            })
            .needsInputs(config.report_input_shapes)
            .needsOutputs(config.report_input_shapes)
            .needsIds(true)
            .scopes(scopes));
    state_ptr->setCallbackHandle(handle);
}

}  // namespace xsigma::profiler::impl
