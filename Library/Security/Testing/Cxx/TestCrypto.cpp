#include <algorithm>
#include <cstring>

#include "Core/Testing/xsigmaTest.h"
#include "crypto.h"

using namespace xsigma::security;

// ============================================================================
// Secure Random Number Generation Tests
// ============================================================================

XSIGMATEST(crypto_test, generate_random_bytes_success)
{
    uint8_t buffer[32];
    std::memset(buffer, 0, sizeof(buffer));

    EXPECT_TRUE(crypto::generate_random_bytes(buffer, sizeof(buffer)));

    // Check that buffer is not all zeros (extremely unlikely with random data)
    bool has_non_zero = false;
    for (size_t i = 0; i < sizeof(buffer); ++i)
    {
        if (buffer[i] != 0)
        {
            has_non_zero = true;
            break;
        }
    }
    EXPECT_TRUE(has_non_zero);
}

XSIGMATEST(crypto_test, generate_random_bytes_null_buffer)
{
    EXPECT_FALSE(crypto::generate_random_bytes(nullptr, 32));
}

XSIGMATEST(crypto_test, generate_random_bytes_zero_size)
{
    uint8_t buffer[32];
    EXPECT_FALSE(crypto::generate_random_bytes(buffer, 0));
}

XSIGMATEST(crypto_test, generate_random_int_success)
{
    auto result = crypto::generate_random_int<uint64_t>();
    EXPECT_TRUE(result.has_value());
}

XSIGMATEST(crypto_test, generate_random_int_different_values)
{
    auto val1 = crypto::generate_random_int<uint64_t>();
    auto val2 = crypto::generate_random_int<uint64_t>();

    EXPECT_TRUE(val1.has_value());
    EXPECT_TRUE(val2.has_value());

    // Extremely unlikely to be equal
    EXPECT_NE(val1.value(), val2.value());
}

XSIGMATEST(crypto_test, generate_random_int_range_valid)
{
    for (int i = 0; i < 100; ++i)
    {
        auto result = crypto::generate_random_int_range(1, 10);
        EXPECT_TRUE(result.has_value());
        EXPECT_GE(result.value(), 1);
        EXPECT_LE(result.value(), 10);
    }
}

XSIGMATEST(crypto_test, generate_random_int_range_invalid)
{
    auto result = crypto::generate_random_int_range(10, 1);
    EXPECT_FALSE(result.has_value());
}

XSIGMATEST(crypto_test, generate_random_string_success)
{
    auto result = crypto::generate_random_string(16);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value().length(), 16);
}

XSIGMATEST(crypto_test, generate_random_string_custom_charset)
{
    auto result = crypto::generate_random_string(10, "ABC");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value().length(), 10);

    // All characters should be from charset
    for (char c : result.value())
    {
        EXPECT_TRUE(c == 'A' || c == 'B' || c == 'C');
    }
}

XSIGMATEST(crypto_test, generate_random_string_zero_length)
{
    auto result = crypto::generate_random_string(0);
    EXPECT_FALSE(result.has_value());
}

XSIGMATEST(crypto_test, generate_random_string_empty_charset)
{
    auto result = crypto::generate_random_string(10, "");
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// SHA-256 Hashing Tests
// ============================================================================

XSIGMATEST(crypto_test, sha256_empty_string)
{
    auto hash = crypto::sha256("");
    EXPECT_EQ(hash.size(), 32);

    // Known SHA-256 hash of empty string
    std::array<uint8_t, 32> expected = {0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14,
                                        0x9a, 0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24,
                                        0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c,
                                        0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55};

    EXPECT_EQ(hash, expected);
}

XSIGMATEST(crypto_test, sha256_known_value)
{
    auto hash = crypto::sha256("hello");
    EXPECT_EQ(hash.size(), 32);

    // Known SHA-256 hash of "hello"
    std::array<uint8_t, 32> expected = {0x2c, 0xf2, 0x4d, 0xba, 0x5f, 0xb0, 0xa3, 0x0e,
                                        0x26, 0xe8, 0x3b, 0x2a, 0xc5, 0xb9, 0xe2, 0x9e,
                                        0x1b, 0x16, 0x1e, 0x5c, 0x1f, 0xa7, 0x42, 0x5e,
                                        0x73, 0x04, 0x33, 0x62, 0x93, 0x8b, 0x98, 0x24};

    EXPECT_EQ(hash, expected);
}

XSIGMATEST(crypto_test, sha256_hex_empty_string)
{
    std::string hash_hex = crypto::sha256_hex("");
    EXPECT_EQ(hash_hex.length(), 64);  // 32 bytes = 64 hex chars
    EXPECT_EQ(hash_hex, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

XSIGMATEST(crypto_test, sha256_hex_known_value)
{
    std::string hash_hex = crypto::sha256_hex("hello");
    EXPECT_EQ(hash_hex.length(), 64);
    EXPECT_EQ(hash_hex, "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824");
}

XSIGMATEST(crypto_test, sha256_different_inputs)
{
    auto hash1 = crypto::sha256("test1");
    auto hash2 = crypto::sha256("test2");

    EXPECT_NE(hash1, hash2);
}

XSIGMATEST(crypto_test, sha256_deterministic)
{
    auto hash1 = crypto::sha256("test");
    auto hash2 = crypto::sha256("test");

    EXPECT_EQ(hash1, hash2);
}

// ============================================================================
// Constant-Time Comparison Tests
// ============================================================================

XSIGMATEST(crypto_test, constant_time_compare_equal)
{
    uint8_t a[] = {1, 2, 3, 4, 5};
    uint8_t b[] = {1, 2, 3, 4, 5};

    EXPECT_TRUE(crypto::constant_time_compare(a, b, 5));
}

XSIGMATEST(crypto_test, constant_time_compare_not_equal)
{
    uint8_t a[] = {1, 2, 3, 4, 5};
    uint8_t b[] = {1, 2, 3, 4, 6};

    EXPECT_FALSE(crypto::constant_time_compare(a, b, 5));
}

XSIGMATEST(crypto_test, constant_time_compare_null_pointers)
{
    uint8_t a[] = {1, 2, 3};
    EXPECT_FALSE(crypto::constant_time_compare(nullptr, a, 3));
    EXPECT_FALSE(crypto::constant_time_compare(a, nullptr, 3));
    EXPECT_FALSE(crypto::constant_time_compare(nullptr, nullptr, 3));
}

XSIGMATEST(crypto_test, constant_time_compare_strings_equal)
{
    EXPECT_TRUE(crypto::constant_time_compare("hello", "hello"));
    EXPECT_TRUE(crypto::constant_time_compare("", ""));
}

XSIGMATEST(crypto_test, constant_time_compare_strings_not_equal)
{
    EXPECT_FALSE(crypto::constant_time_compare("hello", "world"));
    EXPECT_FALSE(crypto::constant_time_compare("test", "testing"));
}

XSIGMATEST(crypto_test, constant_time_compare_strings_different_length)
{
    EXPECT_FALSE(crypto::constant_time_compare("hello", "hello world"));
}

// ============================================================================
// Utility Function Tests
// ============================================================================

XSIGMATEST(crypto_test, bytes_to_hex)
{
    uint8_t     data[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
    std::string hex    = crypto::bytes_to_hex(data, sizeof(data));
    EXPECT_EQ(hex, "0123456789abcdef");
}

XSIGMATEST(crypto_test, bytes_to_hex_empty)
{
    std::string hex = crypto::bytes_to_hex(nullptr, 0);
    EXPECT_EQ(hex, "");
}

XSIGMATEST(crypto_test, hex_to_bytes_valid)
{
    auto result = crypto::hex_to_bytes("0123456789abcdef");
    EXPECT_TRUE(result.has_value());

    std::vector<uint8_t> expected = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
    EXPECT_EQ(result.value(), expected);
}

XSIGMATEST(crypto_test, hex_to_bytes_uppercase)
{
    auto result = crypto::hex_to_bytes("ABCDEF");
    EXPECT_TRUE(result.has_value());

    std::vector<uint8_t> expected = {0xab, 0xcd, 0xef};
    EXPECT_EQ(result.value(), expected);
}

XSIGMATEST(crypto_test, hex_to_bytes_invalid_length)
{
    auto result = crypto::hex_to_bytes("123");  // Odd length
    EXPECT_FALSE(result.has_value());
}

XSIGMATEST(crypto_test, hex_to_bytes_invalid_chars)
{
    auto result = crypto::hex_to_bytes("12GH");
    EXPECT_FALSE(result.has_value());
}

XSIGMATEST(crypto_test, hex_to_bytes_empty)
{
    auto result = crypto::hex_to_bytes("");
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

XSIGMATEST(crypto_test, hex_roundtrip)
{
    uint8_t     original[] = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0};
    std::string hex        = crypto::bytes_to_hex(original, sizeof(original));
    auto        result     = crypto::hex_to_bytes(hex);

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), sizeof(original));
    EXPECT_TRUE(
        std::equal(
            result.value().begin(), result.value().end(), original, original + sizeof(original)));
}

// ============================================================================
// Secure Zero Memory Tests
// ============================================================================

XSIGMATEST(crypto_test, secure_zero_memory)
{
    uint8_t buffer[32];
    std::memset(buffer, 0xFF, sizeof(buffer));

    crypto::secure_zero_memory(buffer, sizeof(buffer));

    // Verify all bytes are zero
    for (size_t i = 0; i < sizeof(buffer); ++i)
    {
        EXPECT_EQ(buffer[i], 0);
    }
}

XSIGMATEST(crypto_test, secure_zero_memory_null_pointer)
{
    // Should not crash
    crypto::secure_zero_memory(nullptr, 32);
}

XSIGMATEST(crypto_test, secure_zero_memory_zero_size)
{
    uint8_t buffer[32];
    std::memset(buffer, 0xFF, sizeof(buffer));

    // Should not crash or modify buffer
    crypto::secure_zero_memory(buffer, 0);
}

// ============================================================================
// Integration Tests
// ============================================================================

XSIGMATEST(crypto_test, hash_random_data)
{
    auto random_data = crypto::generate_random_string(100);
    EXPECT_TRUE(random_data.has_value());

    auto hash = crypto::sha256(random_data.value());
    EXPECT_EQ(hash.size(), 32);
}

XSIGMATEST(crypto_test, compare_hashes)
{
    std::string data1 = "test data";
    std::string data2 = "test data";
    std::string data3 = "different";

    auto hash1 = crypto::sha256(data1);
    auto hash2 = crypto::sha256(data2);
    auto hash3 = crypto::sha256(data3);

    EXPECT_TRUE(crypto::constant_time_compare(hash1.data(), hash2.data(), 32));
    EXPECT_FALSE(crypto::constant_time_compare(hash1.data(), hash3.data(), 32));
}
