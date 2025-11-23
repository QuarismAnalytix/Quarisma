#pragma once

#include <memory>
#include <set>
#include <string>

// Skip Kineto dependency on mobile unless explicitly asked for.
// When is it explicitly asked for?
//   KinetoEdgeCPUProfiler uses KinetoProfiler for cpu
//   event profiling. This has a dependency on cpu only libkineto
#if XSIGMA_HAS_KINETO && defined(XSIGMA_MOBILE) && !defined(EDGE_PROFILER_USE_KINETO)
#undef XSIGMA_HAS_KINETO
#endif

#if XSIGMA_HAS_KINETO
#include <ActivityType.h>
#endif

#include "profiler/common/api.h"
#include "util/exception.h"

#if XSIGMA_HAS_KINETO
// Forward declarations so we don't have to include `libkineto.h` in a header.
namespace libkineto
{
class GenericTraceActivity;
struct CpuTraceBuffer;
class ActivityTraceInterface;
}  // namespace libkineto
#endif

namespace xsigma
{
namespace profiler
{

#if XSIGMA_HAS_KINETO
constexpr bool kKinetoAvailable{true};
#else
constexpr bool kKinetoAvailable{false};
#endif

namespace impl::kineto
{

// ----------------------------------------------------------------------------
// -- Interface (Does not require Kineto) -------------------------------------
// ----------------------------------------------------------------------------
struct DeviceAndResource
{
    int32_t device;
    int32_t resource;
};
DeviceAndResource kineto_ids();

#if XSIGMA_HAS_KINETO
using trace_t           = libkineto::CpuTraceBuffer;
using interface_trace_t = libkineto::ActivityTraceInterface;
using activity_t        = libkineto::GenericTraceActivity;
using activity_type_t   = libkineto::ActivityType;
#else
struct DummyTraceBuffer
{
};
struct DummyTraceInterface
{
};

using trace_t           = DummyTraceBuffer;
using interface_trace_t = DummyTraceBuffer;
struct activity_t;
struct activity_type_t;
#endif  // XSIGMA_HAS_KINETO

void addMetadata(activity_t* activity, const std::string& key, const std::string& value);

// Wraps: libkineto::CpuTraceBuffer
struct TraceWrapper
{
    TraceWrapper(const int64_t start_time, const std::string& name);

    // The caller is expected to hold a mutex when calling `addCPUActivity`.
    // TODO: XSigma-specific method commented out
    activity_t* addCPUActivity(
        const std::string&      name,
        const activity_type_t   type,
        const DeviceAndResource device_and_resource,
        const uint64_t          correlation_id,
        const int64_t           start_time,
        const int64_t           end_time);

    void transferCpuTrace(int64_t end_time);

    explicit operator bool() const;

    std::unique_ptr<trace_t>& get() { return cpu_trace_; }

private:
    std::unique_ptr<trace_t> cpu_trace_;
};

// Wraps libkineto::ActivityTraceInterface
struct ActivityTraceWrapper
{
    explicit ActivityTraceWrapper(std::unique_ptr<interface_trace_t>&& trace);
    ActivityTraceWrapper() = default;
    XSIGMA_API explicit operator bool() const;
    XSIGMA_API void     save(const std::string& path);

    const std::unique_ptr<interface_trace_t>& get() { return trace_; }

private:
    std::unique_ptr<interface_trace_t> trace_;
#if XSIGMA_HAS_KINETO
    bool saved_ = false;  // Kineto's save is destructive
#endif
};

// TODO: XSigma-specific types commented out
using ActivitySet = std::set<xsigma::autograd::profiler::ActivityType>;
XSIGMA_API void prepareTrace(
    const bool                                        cpuOnly,
    const ActivitySet&                                activities,
    const xsigma::profiler::impl::ExperimentalConfig& config,
    const std::string&                                trace_id = "");

XSIGMA_API void                 toggleCollectionDynamic(const bool enable);
XSIGMA_API void                 startTrace();
XSIGMA_API ActivityTraceWrapper stopTrace();
XSIGMA_API void                 pushCorrelationId(uint64_t correlation_id);
XSIGMA_API void                 pushUserCorrelationId(uint64_t correlation_id);
XSIGMA_API void                 popCorrelationId();
XSIGMA_API void                 popUserCorrelationId();
XSIGMA_API void                 recordThreadInfo();
XSIGMA_API bool                 collectivesProfilerExists();

XSIGMA_API void logInvariantViolation(
    const std::string& assertion,
    const std::string& error,
    const std::string& profile_id,
    const std::string& group_profile_id);

}  // namespace impl::kineto

}  // namespace profiler

namespace autograd::profiler
{
// TODO: XSigma-specific function commented out
xsigma::device_enum deviceTypeFromActivity(
    xsigma::profiler::impl::kineto::activity_type_t activity_type);

XSIGMA_API void addMetadataJson(const std::string& key, const std::string& value);

XSIGMA_API void profilerStep();

}  // namespace autograd::profiler

}  // namespace xsigma
