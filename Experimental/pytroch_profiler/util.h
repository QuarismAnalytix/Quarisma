#pragma once

#if 0
#include "common/export.h"
#include "common/macros.h"
#include "record_function.h"
#include "util/hash.h"
//#include <xsigma/csrc/jit/frontend/source_range.h>

#include <cstddef>
#include <cstdint>
#include <list>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// TODO: replace with pytorch/rfcs#43 when it is ready.
#define SOFT_ASSERT(cond, ...)                              \
    [&]() -> bool                                           \
    {                                                       \
        if XSIGMA_UNLIKELY (!(cond))                        \
        {                                                   \
            xsigma::profiler::impl::logSoftAssert(          \
                __func__,                                   \
                __FILE__,                                   \
                static_cast<uint32_t>(__LINE__),            \
                #cond,                                      \
                ::xsigma::str(__VA_ARGS__));                \
            if (xsigma::profiler::impl::softAssertRaises()) \
            {                                               \
                XSIGMA_CHECK(cond, __VA_ARGS__);            \
            }                                               \
            else                                            \
            {                                               \
                XSIGMA_WARN_ONCE(__VA_ARGS__);              \
            }                                               \
            return false;                                   \
        }                                                   \
        return true;                                        \
    }()
    
namespace xsigma::detail 
{
struct CompileTimeEmptyString {
  operator const std::string&() const {
    static const std::string empty_string_literal;
    return empty_string_literal;
  }
  operator const char*() const {
    return "";
  }
};
} // namespace xsigma::detail

namespace xsigma::profiler::impl
{
XSIGMA_API bool softAssertRaises();
XSIGMA_API void setSoftAssertRaises(std::optional<bool> value);
XSIGMA_API void logSoftAssert(
    const char* func, const char* file, uint32_t line, const char* cond, const char* args);
inline void logSoftAssert(
    const char*                              func,
    const char*                              file,
    uint32_t                                 line,
    const char*                              cond,
    ::xsigma::detail::CompileTimeEmptyString args)
{
    logSoftAssert(func, file, line, cond, (const char*)args);
}
XSIGMA_API void logSoftAssert(
    const char* func, const char* file, uint32_t line, const char* cond, const std::string& args);

using shape = std::variant<std::vector<int64_t>, std::vector<std::vector<int64_t>>>;
constexpr int TENSOR_LIST_DISPLAY_LENGTH_LIMIT = 30;

std::string getNvtxStr(
    const char*                                                name,
    int64_t                                                    sequence_nr,
    const std::vector<std::vector<int64_t>>&                   shapes,
    xsigma::RecordFunctionHandle                                   op_id        = 0,
    const std::list<std::pair<xsigma::RecordFunctionHandle, int>>& input_op_ids = {});

struct XSIGMA_API FileLineFunc
{
    std::string filename;
    size_t      line;
    std::string funcname;
};

struct XSIGMA_API SaveNcclMetaConfig
{
    bool truncate;
    bool introspectMetadata;
    bool introspectInputs;
    bool introspectOutputs;

    // Default constructor with default values
    SaveNcclMetaConfig()
        : truncate(true),
          introspectMetadata(true),
          introspectInputs(false),
          introspectOutputs(false)
    {
    }

    SaveNcclMetaConfig(
        bool truncate, bool introspectMetadata, bool introspectInputs, bool introspectOutputs)
        : truncate(truncate),
          introspectMetadata(introspectMetadata),
          introspectInputs(introspectInputs),
          introspectOutputs(introspectOutputs)
    {
    }
};

XSIGMA_API std::vector<FileLineFunc> prepareCallstack(const std::vector<jit::StackEntry>& cs);
XSIGMA_API std::vector<std::string> callstackStr(const std::vector<FileLineFunc>& cs);
XSIGMA_API std::string stacksToStr(const std::vector<std::string>& stacks, const char* delim);
XSIGMA_API std::vector<std::vector<int64_t>> inputSizes(
    const xsigma::RecordFunction& fn, const bool flatten_list_enabled = false);
XSIGMA_API std::string variantShapesToStr(const std::vector<shape>& shapes);
XSIGMA_API std::string shapesToStr(const std::vector<std::vector<int64_t>>& shapes);
XSIGMA_API std::string strListToStr(const std::vector<std::string>& types);
XSIGMA_API std::string inputOpIdsToStr(
    const std::list<std::pair<xsigma::RecordFunctionHandle, int>>& input_op_ids);
XSIGMA_API std::string ivalueToStr(const xsigma::IValue& val, bool isString);
XSIGMA_API std::string ivalueListToStr(const std::vector<xsigma::IValue>& list);
XSIGMA_API std::vector<std::string> inputTypes(const xsigma::RecordFunction& fn);

std::unordered_map<std::string, xsigma::IValue> XSIGMA_API
saveExtraArgs(const xsigma::RecordFunction& fn);
std::unordered_map<std::string, std::string> XSIGMA_API
saveNcclMeta(const xsigma::RecordFunction& fn, const SaveNcclMetaConfig& config = SaveNcclMetaConfig());
int  getTensorStartHint(const xsigma::Tensor& t);
bool checkFunctionOutputsForLogging(const xsigma::RecordFunction& fn);
bool checkFunctionInputsForLogging(const xsigma::RecordFunction& fn);
std::pair<bool, std::variant<int, std::vector<int>>> findStartAddrForTensors(
    const xsigma::IValue& val);
uint64_t XSIGMA_API computeFlops(
    const std::string& op_name, const std::unordered_map<std::string, xsigma::IValue>& extra_args);

std::string shapeToStr(const std::vector<int64_t>& shape);

template <typename T>
class XSIGMA_API GlobalStateManager
{
public:
    static GlobalStateManager& singleton()
    {
        /* library-local */ static GlobalStateManager singleton_;
        return singleton_;
    }

    static void push(std::shared_ptr<T>&& state)
    {
        if (singleton().state_)
        {
            LOG(WARNING) << "GlobalStatePtr already exists!";
        }
        else
        {
            singleton().state_ = std::move(state);
        }
    }

    static auto* get() { return singleton().state_.get(); }

    static std::shared_ptr<T> pop()
    {
        auto out = singleton().state_;
        singleton().state_.reset();
        return out;
    }

private:
    GlobalStateManager() = default;

    std::shared_ptr<T> state_;
};

class XSIGMA_VISIBILITY HashCombine
{
    template <typename T0, typename T1>
    size_t operator()(const std::pair<T0, T1>& i)
    {
        return xsigma::get_hash((*this)(i.first), (*this)(i.second));
    }

    template <typename... Args>
    size_t operator()(const std::tuple<Args...>& i)
    {
        return xsigma::get_hash(i);
    }

    template <typename T>
    size_t operator()(const T& i)
    {
        return xsigma::get_hash(i);
    }
};

#ifdef USE_DISTRIBUTED
constexpr auto kCommsName        = "Collective name";
constexpr auto kDtype            = "dtype";
constexpr auto kInMsgNelems      = "In msg nelems";
constexpr auto kOutMsgNelems     = "Out msg nelems";
constexpr auto kInSplit          = "In split size";
constexpr auto kOutSplit         = "Out split size";
constexpr auto kGlobalRankStart  = "Global rank start";
constexpr auto kGlobalRankStride = "Global rank stride";
constexpr auto kGroupSize        = "Group size";
constexpr auto kProcessGroupName = "Process Group Name";
constexpr auto kProcessGroupDesc = "Process Group Description";
constexpr auto kGroupRanks       = "Process Group Ranks";
constexpr auto kRank             = "Rank";
constexpr auto kP2pSrc           = "Src Rank";
constexpr auto kP2pDst           = "Dst Rank";
constexpr auto kInTensorsStart   = "Input Tensors start";
constexpr auto kOutTensorsStart  = "Output Tensors start";
#endif  // USE_DISTRIBUTED

}  // namespace xsigma::profiler::impl
#endif
