#pragma once

#include <set>
#include <string>
#include <vector>

#include "profiler/base/base.h"
#include "profiler/common/api.h"
#include "profiler/common/events.h"
#include "profiler/common/util.h"

namespace quarisma
{
constexpr bool hasCUDA()
{
    return QUARISMA_HAS_CUDA == 1;
}

namespace profiler::impl
{
struct Result;
namespace kineto
{
struct ActivityTraceWrapper;
}  // namespace kineto
}  // namespace profiler::impl

namespace autograd::profiler
{
using experimental_event_t = std::shared_ptr<quarisma::profiler::impl::Result>;
using extra_meta_t         = std::unordered_map<std::string, std::string>;

struct QUARISMA_VISIBILITY KinetoEvent
{
    QUARISMA_API KinetoEvent(
        const std::shared_ptr<const quarisma::profiler::impl::Result>& /*result*/,
        const bool verbose);

    QUARISMA_API uint64_t startThreadId() const;
    QUARISMA_API uint64_t endThreadId() const;
    QUARISMA_API uint8_t  activityType() const;
    QUARISMA_API uint64_t fwdThreadId() const;
    QUARISMA_API bool     hasShapes() const;
    QUARISMA_API quarisma::array_ref<std::vector<int64_t>> shapes() const;
    QUARISMA_API bool                                    hasTypes() const;
    QUARISMA_API quarisma::array_ref<std::string> dtypes() const;
    QUARISMA_API bool                           hasConcreteInputs() const;
    QUARISMA_API quarisma::array_ref<quarisma::IValue> concreteInputs() const;
    QUARISMA_API bool                              hasKwinputs() const;
    QUARISMA_API bool                              isHiddenEvent() const;
    QUARISMA_API std::unordered_map<std::string, quarisma::IValue> kwinputs() const;
    QUARISMA_API uint64_t                                        flops() const;
    QUARISMA_API int64_t                                         sequenceNr() const;
    QUARISMA_API bool                                            hasStack() const;
    QUARISMA_API quarisma::array_ref<std::string> stack() const;
    QUARISMA_API uint8_t                        scope() const;
    QUARISMA_API bool                           hasModuleHierarchy() const;
    QUARISMA_API quarisma::array_ref<std::string> moduleHierarchy() const;
    QUARISMA_API int64_t                        debugHandle() const;
    QUARISMA_API std::string name() const;
    QUARISMA_API std::string overload_name() const;
    QUARISMA_API quarisma::device_enum deviceType() const;
    QUARISMA_API int                 deviceIndex() const;
    QUARISMA_API int64_t             nBytes() const;
    QUARISMA_API uint64_t            startNs() const;
    QUARISMA_API uint64_t            endNs() const;
    QUARISMA_API uint64_t            durationNs() const;
    QUARISMA_API bool                isAsync() const;
    QUARISMA_API uint64_t            correlationId() const;
    QUARISMA_API uint64_t            linkedCorrelationId() const;
    QUARISMA_API int64_t             deviceResourceId() const;
    QUARISMA_API std::string  backend() const;
    static QUARISMA_API bool  isPythonFunction();
    QUARISMA_API int64_t      cudaElapsedUs() const;
    QUARISMA_API int64_t      privateuse1ElapsedUs() const;
    QUARISMA_API void         getPerfEventCounters(quarisma::profiler::perf_counters_t& /*in*/) const;
    QUARISMA_API extra_meta_t extraMeta() const;
    QUARISMA_API std::string metadataJson() const;

private:
    quarisma::profiler::impl::ProfilerVoidEventStub fallbackStart() const;
    quarisma::profiler::impl::ProfilerVoidEventStub fallbackEnd() const;

    std::shared_ptr<const quarisma::profiler::impl::Result> result_;
    std::vector<std::string>                              python_stack_;

    // Copy fields from result so we can return ArrayRefs.
    std::vector<std::vector<int64_t>>               shapes_;
    std::vector<std::string>                        dtypes_;
    std::vector<quarisma::IValue>                     concrete_inputs_;
    std::unordered_map<std::string, quarisma::IValue> kwinputs_;
};

// Consolidating events returned directly from Kineto
// with events manually created by us (e.g. start/stop marks,
// memory allocation events)
struct QUARISMA_VISIBILITY ProfilerResult
{
    QUARISMA_API ProfilerResult();
    QUARISMA_API ProfilerResult(
        uint64_t                                                                start_time,
        std::vector<KinetoEvent>                                                events,
        std::unique_ptr<quarisma::profiler::impl::kineto::ActivityTraceWrapper>&& trace,
        std::vector<experimental_event_t>&&                                     event_tree);
    QUARISMA_API ~ProfilerResult();

    uint64_t trace_start_ns() const { return trace_start_ns_; }

    const std::vector<KinetoEvent>& events() const { return events_; }

    const std::vector<experimental_event_t>& event_tree() const { return event_tree_; }

    QUARISMA_API void save(const std::string& path);

private:
    uint64_t                                                              trace_start_ns_ = 0;
    std::vector<KinetoEvent>                                              events_;
    std::unique_ptr<quarisma::profiler::impl::kineto::ActivityTraceWrapper> trace_;
    std::vector<experimental_event_t>                                     event_tree_;
};

/*
 * This API is used by backends to record latency of events that
 * happened in the backend but were not visible to pytorch runtime.
 * For example, if part of the model is lowered to a dsp backend, then
 * the execution of that part of the model is delegated to the backend.
 * When backend finishes execution it has an option to provide profiling
 * information (latency only at the moment) corresponding to different operators
 * that were executed in the backend.
 * When such events are recorded by backend using this API, the event
 * records will be collected by active kineto profiler. If no kineto profiler
 * is active then the event is ignored.
 * This provides us with a way to generate all the profiling information
 * for a model regardless of where model (or part of it) executed.
 * @param start_time_us: start time in us of the event
 * @param end_time_us: end time in us of the event
 * @param debug_handle: debug handle to correlate this event/op with
 * model level module/source information
 * @param scope: scope of the event, e.g. LITE_INTERPRETER, RECORD_FN etc.
 * @param event_name: name of the event, e.g. op name
 * @param backend_name: name of the backend where the event took place.
 */
QUARISMA_API void reportBackendEventToActiveKinetoProfiler(
    const int64_t             start_time_us,
    const int64_t             end_time_us,
    const int64_t             debug_handle,
    const quarisma::RecordScope scope,
    const std::string&        event_name,
    const std::string&        backend_name);

QUARISMA_API void enableProfiler(
    const quarisma::profiler::impl::ProfilerConfig&         config,
    const std::set<quarisma::profiler::impl::ActivityType>& activities,
    const std::unordered_set<quarisma::RecordScope>&        scopes = {});

/*
 * Same as enableProfiler but with callback to do post-processing of
 * KinetoEvents.
 * enableProfilerWithEventPostProcess enables profiler to capture
 * specified activities, with specified RecordFunction scope, if any.
 * Additionally, it takes a functor that does in-place post processing of
 * events, e.g. populate stack trace or module hierarchy information lazily
 * using debug_handle.
 * Example usage is with lite interpreter that has recording scope of
 * LITE_INTERPRETER. In this case lite interpreter runtime, records debug
 * handles in RecordFunction, along with other information. Debug handles are
 * eventually passed down to KinetoEvent and recorded as part of the event.
 * KinetoEdgeCPUProfiler, in quarisma/csrc/jit/mobile/profiler_edge.cpp, enables
 * profiler using post-processing callback, via
 * enableProfilerWithEventPostProcess, that takes these debug handles and
 * generates stack trace and module hierarchy information, once profiling is
 * done.
 */
using post_process_t = std::function<void(
    /*debug_handle */ int64_t,
    /*jit_stack    */ std::vector<std::string>&,
    /*jit_modules  */ std::vector<std::string>&)>;
QUARISMA_API void enableProfilerWithEventPostProcess(
    const quarisma::profiler::impl::ProfilerConfig&         config,
    const std::set<quarisma::profiler::impl::ActivityType>& activities,
    post_process_t&&                                      cb,
    const std::unordered_set<quarisma::RecordScope>&        scopes = {});

QUARISMA_API std::unique_ptr<ProfilerResult> disableProfiler();

QUARISMA_API void prepareProfiler(
    const quarisma::profiler::impl::ProfilerConfig&         config,
    const std::set<quarisma::profiler::impl::ActivityType>& activities);

QUARISMA_API void toggleCollectionDynamic(
    const bool enable, const std::set<quarisma::profiler::impl::ActivityType>& activities);

QUARISMA_API void startMemoryProfile();
QUARISMA_API void stopMemoryProfile();
QUARISMA_API void exportMemoryProfile(const std::string& path);

/**
 * When a C++ thread really has no control over how the profiler was enabled,
 * for example, by some unreachable Python code, it can call these functions
 * to test/join/unjoin itself into the collection set of a profiler, if any.
 * Without calling these functions, the symptom may be "not seeing GPU events
 * from some child C++ threads". This is an example on how to use them,
 *
 *    using namespace quarisma::autograd::profiler;
 *    bool enabled = isProfilerEnabledInMainThread();
 *    if (enabled != saved_enabled_state) {
 *      if (enabled) {
 *        enableProfilerInChildThread();
 *      } else {
 *        disableProfilerInChildThread();
 *      }
 *      saved_enabled_state = enabled;
 *    }
 */
QUARISMA_API bool isProfilerEnabledInMainThread();
QUARISMA_API void enableProfilerInChildThread();
QUARISMA_API void disableProfilerInChildThread();

}  // namespace autograd::profiler

namespace profiler::impl
{

// Experimental.
QUARISMA_API void _reportVulkanEventToProfiler(vulkan_id_t id);

}  // namespace profiler::impl

}  // namespace quarisma
