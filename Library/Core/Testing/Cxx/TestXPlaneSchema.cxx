/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Comprehensive test suite for XPlane Schema
 * Tests schema definitions, enums, and utility functions
 */

#include <string>

#include "Testing/xsigmaTest.h"
#include "profiler/exporters/xplane/xplane_schema.h"

using namespace xsigma;

// ============================================================================
// ContextType Tests
// ============================================================================

XSIGMATEST(XPlaneSchema, context_type_to_string_legacy)
{
    const char* result = GetContextTypeString(ContextType::kLegacy);
    EXPECT_STREQ(result, "");  // kLegacy returns empty string
}

XSIGMATEST(XPlaneSchema, context_type_to_string_threadpool_event)
{
    const char* result = GetContextTypeString(ContextType::kThreadpoolEvent);
    EXPECT_STREQ(result, "threadpool_event");
}

XSIGMATEST(XPlaneSchema, context_type_to_string_gpu_launch)
{
    const char* result = GetContextTypeString(ContextType::kGpuLaunch);
    EXPECT_STREQ(result, "gpu_launch");
}

// Note: GetContextTypeString has no default case, so calling it with invalid
// values causes undefined behavior. We don't test that case.

XSIGMATEST(XPlaneSchema, get_safe_context_type_valid)
{
    ContextType result = GetSafeContextType(static_cast<uint32_t>(ContextType::kGpuLaunch));
    EXPECT_EQ(result, ContextType::kGpuLaunch);
}

XSIGMATEST(XPlaneSchema, get_safe_context_type_invalid)
{
    ContextType result = GetSafeContextType(999);
    EXPECT_EQ(result, ContextType::kGeneric);  // Should default to Generic
}

// ============================================================================
// HostEventType Tests
// ============================================================================

XSIGMATEST(XPlaneSchema, get_host_event_type_str_valid)
{
    std::string_view result = GetHostEventTypeStr(HostEventType::kFirstHostEventType);
    EXPECT_FALSE(result.empty());
}

// Note: GetHostEventTypeStr uses .at() which throws for invalid values.
// We don't test that case since XSigma doesn't use exceptions.

XSIGMATEST(XPlaneSchema, find_host_event_type_valid)
{
    std::optional<int32_t> result = FindHostEventType("UnknownHostEventType");
    EXPECT_TRUE(result.has_value());
    if (result.has_value())
    {
        EXPECT_EQ(result.value(), static_cast<int32_t>(HostEventType::kUnknownHostEventType));
    }
}

XSIGMATEST(XPlaneSchema, find_host_event_type_invalid)
{
    std::optional<int32_t> result = FindHostEventType("NonExistentEventType");
    EXPECT_FALSE(result.has_value());
}

XSIGMATEST(XPlaneSchema, find_host_event_type_empty_string)
{
    std::optional<int32_t> result = FindHostEventType("");
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// StatType Tests
// ============================================================================

XSIGMATEST(XPlaneSchema, get_stat_type_str_valid)
{
    std::string_view result = GetStatTypeStr(StatType::kFirstStatType);
    EXPECT_FALSE(result.empty());
}

// Note: GetStatTypeStr uses .at() which throws for invalid values.
// We don't test that case since XSigma doesn't use exceptions.

XSIGMATEST(XPlaneSchema, find_stat_type_valid)
{
    std::optional<int32_t> result = FindStatType("UnknownStatType");
    EXPECT_TRUE(result.has_value());
    if (result.has_value())
    {
        EXPECT_EQ(result.value(), static_cast<int32_t>(StatType::kUnknownStatType));
    }
}

XSIGMATEST(XPlaneSchema, find_stat_type_invalid)
{
    std::optional<int32_t> result = FindStatType("NonExistentStatType");
    EXPECT_FALSE(result.has_value());
}

XSIGMATEST(XPlaneSchema, find_stat_type_empty_string)
{
    std::optional<int32_t> result = FindStatType("");
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// XFlow Tests
// ============================================================================

XSIGMATEST(XPlaneSchema, xflow_construction_and_encoding)
{
    uint64_t             flow_id   = 12345;
    XFlow::FlowDirection direction = XFlow::kFlowIn;
    ContextType          context   = ContextType::kGpuLaunch;  // Use valid context type

    XFlow flow(flow_id, direction, context);

    EXPECT_EQ(flow.Id(), flow_id);
    EXPECT_EQ(flow.Direction(), direction);
    EXPECT_EQ(flow.Category(), context);
}

XSIGMATEST(XPlaneSchema, xflow_to_stat_value_and_from_stat_value)
{
    uint64_t             flow_id   = 54321;
    XFlow::FlowDirection direction = XFlow::kFlowOut;
    ContextType          context   = ContextType::kGpuLaunch;

    XFlow    original(flow_id, direction, context);
    uint64_t encoded = original.ToStatValue();
    XFlow    decoded = XFlow::FromStatValue(encoded);

    EXPECT_EQ(decoded.Id(), flow_id);
    EXPECT_EQ(decoded.Direction(), direction);
    EXPECT_EQ(decoded.Category(), context);
}

XSIGMATEST(XPlaneSchema, xflow_all_directions)
{
    uint64_t flow_id = 99999;

    std::vector<XFlow::FlowDirection> directions = {
        XFlow::kFlowIn, XFlow::kFlowOut, XFlow::kFlowInOut};

    for (auto direction : directions)
    {
        XFlow    flow(flow_id, direction);
        uint64_t encoded = flow.ToStatValue();
        XFlow    decoded = XFlow::FromStatValue(encoded);

        EXPECT_EQ(decoded.Id(), flow_id);
        EXPECT_EQ(decoded.Direction(), direction);
    }
}

XSIGMATEST(XPlaneSchema, xflow_get_unique_id)
{
    uint64_t id1 = XFlow::GetUniqueId();
    uint64_t id2 = XFlow::GetUniqueId();
    uint64_t id3 = XFlow::GetUniqueId();

    EXPECT_NE(id1, id2);
    EXPECT_NE(id2, id3);
    EXPECT_NE(id1, id3);
    EXPECT_LT(id1, id2);  // Should be monotonically increasing
    EXPECT_LT(id2, id3);
}

XSIGMATEST(XPlaneSchema, xflow_get_flow_id_hash)
{
    // Test that GetFlowId generates consistent hashes
    std::string key1 = "test_key_1";
    std::string key2 = "test_key_2";

    uint64_t hash1a = XFlow::GetFlowId(key1);
    uint64_t hash1b = XFlow::GetFlowId(key1);  // Same key
    uint64_t hash2  = XFlow::GetFlowId(key2);  // Different key

    EXPECT_EQ(hash1a, hash1b);  // Same input should give same hash
    EXPECT_NE(hash1a, hash2);   // Different inputs should give different hashes
}

// ============================================================================
// Hash Function Tests
// ============================================================================

XSIGMATEST(XPlaneSchema, hash_of_int64)
{
    int64_t value = 12345;
    size_t  hash  = HashOf(value);
    EXPECT_NE(hash, 0);  // Hash should be non-zero for non-zero input
}

XSIGMATEST(XPlaneSchema, hash_of_uint64)
{
    uint64_t value = 67890;
    size_t   hash  = HashOf(value);
    EXPECT_NE(hash, 0);
}

XSIGMATEST(XPlaneSchema, hash_of_string)
{
    std::string value = "test_string";
    size_t      hash  = HashOf(value);
    EXPECT_NE(hash, 0);
}

XSIGMATEST(XPlaneSchema, hash_of_empty_string)
{
    std::string value = "";
    size_t      hash  = HashOf(value);
    // Empty string hash may be zero or non-zero depending on implementation
    (void)hash;  // Just verify it doesn't crash
    EXPECT_TRUE(true);
}

XSIGMATEST(XPlaneSchema, hash_of_same_values_equal)
{
    int64_t value1 = 12345;
    int64_t value2 = 12345;
    size_t  hash1  = HashOf(value1);
    size_t  hash2  = HashOf(value2);
    EXPECT_EQ(hash1, hash2);
}

XSIGMATEST(XPlaneSchema, hash_of_different_values_different)
{
    int64_t value1 = 12345;
    int64_t value2 = 54321;
    size_t  hash1  = HashOf(value1);
    size_t  hash2  = HashOf(value2);
    EXPECT_NE(hash1, hash2);
}

XSIGMATEST(XPlaneSchema, hash_combine_basic)
{
    size_t hash1    = HashOf(int64_t(123));
    size_t hash2    = HashOf(int64_t(456));
    size_t combined = hash1;
    HashCombine(combined, hash2);
    EXPECT_NE(combined, hash1);
    EXPECT_NE(combined, hash2);
}

XSIGMATEST(XPlaneSchema, hash_combine_order_matters)
{
    size_t hash1 = HashOf(int64_t(123));
    size_t hash2 = HashOf(int64_t(456));

    size_t combined1 = hash1;
    HashCombine(combined1, hash2);

    size_t combined2 = hash2;
    HashCombine(combined2, hash1);

    EXPECT_NE(combined1, combined2);  // Order should matter
}

XSIGMATEST(XPlaneSchema, hash_combine_multiple)
{
    size_t hash = 0;
    HashCombine(hash, HashOf(int64_t(1)));
    HashCombine(hash, HashOf(int64_t(2)));
    HashCombine(hash, HashOf(int64_t(3)));
    EXPECT_NE(hash, 0);
}

XSIGMATEST(XPlaneSchema, hash_combine_with_zero)
{
    size_t hash = 0;
    HashCombine(hash, HashOf(int64_t(0)));
    // Result may or may not be zero depending on implementation
    (void)hash;
    EXPECT_TRUE(true);
}

// ============================================================================
// Additional ContextType Coverage Tests
// ============================================================================

XSIGMATEST(XPlaneSchema, context_type_to_string_generic)
{
    const char* result = GetContextTypeString(ContextType::kGeneric);
    EXPECT_STREQ(result, "");  // kGeneric returns empty string
}

XSIGMATEST(XPlaneSchema, context_type_to_string_tf_executor)
{
    const char* result = GetContextTypeString(ContextType::kTfExecutor);
    EXPECT_STREQ(result, "tf_exec");
}

XSIGMATEST(XPlaneSchema, context_type_to_string_tfrt_executor)
{
    const char* result = GetContextTypeString(ContextType::kTfrtExecutor);
    EXPECT_STREQ(result, "tfrt_exec");
}

XSIGMATEST(XPlaneSchema, context_type_to_string_shared_batch_scheduler)
{
    const char* result = GetContextTypeString(ContextType::kSharedBatchScheduler);
    EXPECT_STREQ(result, "batch_sched");
}

XSIGMATEST(XPlaneSchema, context_type_to_string_pjrt)
{
    const char* result = GetContextTypeString(ContextType::kPjRt);
    EXPECT_STREQ(result, "PjRt");
}

XSIGMATEST(XPlaneSchema, context_type_to_string_adaptive_shared_batch_scheduler)
{
    const char* result = GetContextTypeString(ContextType::kAdaptiveSharedBatchScheduler);
    EXPECT_STREQ(result, "as_batch_sched");
}

XSIGMATEST(XPlaneSchema, context_type_to_string_tfrt_tpu_runtime)
{
    const char* result = GetContextTypeString(ContextType::kTfrtTpuRuntime);
    EXPECT_STREQ(result, "tfrt_rt");
}

XSIGMATEST(XPlaneSchema, context_type_to_string_tpu_embedding_engine)
{
    const char* result = GetContextTypeString(ContextType::kTpuEmbeddingEngine);
    EXPECT_STREQ(result, "tpu_embed");
}

XSIGMATEST(XPlaneSchema, context_type_to_string_batcher)
{
    const char* result = GetContextTypeString(ContextType::kBatcher);
    EXPECT_STREQ(result, "batcher");
}

XSIGMATEST(XPlaneSchema, context_type_to_string_tpu_stream)
{
    const char* result = GetContextTypeString(ContextType::kTpuStream);
    EXPECT_STREQ(result, "tpu_stream");
}

XSIGMATEST(XPlaneSchema, context_type_to_string_tpu_launch)
{
    const char* result = GetContextTypeString(ContextType::kTpuLaunch);
    EXPECT_STREQ(result, "tpu_launch");
}

XSIGMATEST(XPlaneSchema, context_type_to_string_pathways_executor)
{
    const char* result = GetContextTypeString(ContextType::kPathwaysExecutor);
    EXPECT_STREQ(result, "pathways_exec");
}

XSIGMATEST(XPlaneSchema, context_type_to_string_pjrt_library_call)
{
    const char* result = GetContextTypeString(ContextType::kPjrtLibraryCall);
    EXPECT_STREQ(result, "pjrt_library_call");
}

// ============================================================================
// HashOf Template Specialization Tests
// ============================================================================

XSIGMATEST(XPlaneSchema, hash_of_string_view_additional)
{
    std::string_view sv   = "test_string";
    size_t           hash = HashOf(sv);
    EXPECT_NE(hash, 0);
}

XSIGMATEST(XPlaneSchema, hash_of_string_additional)
{
    std::string str  = "test_string";
    size_t      hash = HashOf(str);
    EXPECT_NE(hash, 0);
}

XSIGMATEST(XPlaneSchema, hash_of_string_view_consistency)
{
    std::string      str = "test";
    std::string_view sv  = str;
    // Hash of string and string_view should be the same
    EXPECT_EQ(HashOf(str), HashOf(sv));
}
