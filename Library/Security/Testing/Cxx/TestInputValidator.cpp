#include <limits>
#include <regex>
#include <vector>

#include "Core/Testing/baseTest.h"
#include "input_validator.h"

using namespace quarisma::security;

// ============================================================================
// String Validation Tests
// ============================================================================

QUARISMATEST(input_validator_test, validate_string_length_valid)
{
    EXPECT_TRUE(input_validator::validate_string_length("hello", 1, 10));
    EXPECT_TRUE(input_validator::validate_string_length("test", 4, 4));
    EXPECT_TRUE(input_validator::validate_string_length("", 0, 5));
}

QUARISMATEST(input_validator_test, validate_string_length_invalid)
{
    EXPECT_FALSE(input_validator::validate_string_length("hello", 10, 20));
    EXPECT_FALSE(input_validator::validate_string_length("test", 1, 3));
    EXPECT_FALSE(input_validator::validate_string_length("abc", 5, 10));
}

QUARISMATEST(input_validator_test, is_alphanumeric_valid)
{
    EXPECT_TRUE(input_validator::is_alphanumeric("abc123"));
    EXPECT_TRUE(input_validator::is_alphanumeric("ABC"));
    EXPECT_TRUE(input_validator::is_alphanumeric("123"));
    EXPECT_TRUE(input_validator::is_alphanumeric("Test123"));
}

QUARISMATEST(input_validator_test, is_alphanumeric_invalid)
{
    EXPECT_FALSE(input_validator::is_alphanumeric(""));
    EXPECT_FALSE(input_validator::is_alphanumeric("hello world"));
    EXPECT_FALSE(input_validator::is_alphanumeric("test@123"));
    EXPECT_FALSE(input_validator::is_alphanumeric("hello-world"));
}

QUARISMATEST(input_validator_test, is_printable_ascii_valid)
{
    EXPECT_TRUE(input_validator::is_printable_ascii("Hello World!"));
    EXPECT_TRUE(input_validator::is_printable_ascii("123 ABC xyz"));
    EXPECT_TRUE(input_validator::is_printable_ascii("~!@#$%^&*()"));
}

QUARISMATEST(input_validator_test, is_printable_ascii_invalid)
{
    EXPECT_FALSE(input_validator::is_printable_ascii("hello\nworld"));
    EXPECT_FALSE(input_validator::is_printable_ascii("test\tvalue"));
    EXPECT_FALSE(input_validator::is_printable_ascii("abc\x01xyz"));
}

QUARISMATEST(input_validator_test, matches_pattern_valid)
{
    std::regex email_pattern(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
    EXPECT_TRUE(input_validator::matches_pattern("test@example.com", email_pattern));
    EXPECT_TRUE(input_validator::matches_pattern("user.name@domain.co.uk", email_pattern));
}

QUARISMATEST(input_validator_test, matches_pattern_invalid)
{
    std::regex email_pattern(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
    EXPECT_FALSE(input_validator::matches_pattern("invalid-email", email_pattern));
    EXPECT_FALSE(input_validator::matches_pattern("@example.com", email_pattern));
    EXPECT_FALSE(input_validator::matches_pattern("test@", email_pattern));
}

QUARISMATEST(input_validator_test, has_no_null_bytes_valid)
{
    EXPECT_TRUE(input_validator::has_no_null_bytes("hello world"));
    EXPECT_TRUE(input_validator::has_no_null_bytes("test123"));
    EXPECT_TRUE(input_validator::has_no_null_bytes(""));
}

QUARISMATEST(input_validator_test, has_no_null_bytes_invalid)
{
    std::string with_null = "hello";
    with_null += '\0';
    with_null += "world";
    EXPECT_FALSE(input_validator::has_no_null_bytes(with_null));
}

// ============================================================================
// Numeric Validation Tests
// ============================================================================

QUARISMATEST(input_validator_test, validate_range_valid)
{
    EXPECT_TRUE(input_validator::validate_range(5, 1, 10));
    EXPECT_TRUE(input_validator::validate_range(1, 1, 10));
    EXPECT_TRUE(input_validator::validate_range(10, 1, 10));
    EXPECT_TRUE(input_validator::validate_range(0.5, 0.0, 1.0));
}

QUARISMATEST(input_validator_test, validate_range_invalid)
{
    EXPECT_FALSE(input_validator::validate_range(0, 1, 10));
    EXPECT_FALSE(input_validator::validate_range(11, 1, 10));
    EXPECT_FALSE(input_validator::validate_range(-5, 0, 10));
}

QUARISMATEST(input_validator_test, is_positive_valid)
{
    EXPECT_TRUE(input_validator::is_positive(1));
    EXPECT_TRUE(input_validator::is_positive(100));
    EXPECT_TRUE(input_validator::is_positive(0.1));
}

QUARISMATEST(input_validator_test, is_positive_invalid)
{
    EXPECT_FALSE(input_validator::is_positive(0));
    EXPECT_FALSE(input_validator::is_positive(-1));
    EXPECT_FALSE(input_validator::is_positive(-0.1));
}

QUARISMATEST(input_validator_test, is_non_negative_valid)
{
    EXPECT_TRUE(input_validator::is_non_negative(0));
    EXPECT_TRUE(input_validator::is_non_negative(1));
    EXPECT_TRUE(input_validator::is_non_negative(100));
}

QUARISMATEST(input_validator_test, is_non_negative_invalid)
{
    EXPECT_FALSE(input_validator::is_non_negative(-1));
    EXPECT_FALSE(input_validator::is_non_negative(-100));
}

QUARISMATEST(input_validator_test, is_finite_valid)
{
    EXPECT_TRUE(input_validator::is_finite(0.0));
    EXPECT_TRUE(input_validator::is_finite(1.5));
    EXPECT_TRUE(input_validator::is_finite(-100.0));
}

QUARISMATEST(input_validator_test, is_finite_invalid)
{
    EXPECT_FALSE(input_validator::is_finite(std::numeric_limits<double>::infinity()));
    EXPECT_FALSE(input_validator::is_finite(-std::numeric_limits<double>::infinity()));
    EXPECT_FALSE(input_validator::is_finite(std::numeric_limits<double>::quiet_NaN()));
}

// ============================================================================
// Collection Validation Tests
// ============================================================================

QUARISMATEST(input_validator_test, validate_collection_size_valid)
{
    std::vector<int> vec = {1, 2, 3, 4, 5};
    EXPECT_TRUE(input_validator::validate_collection_size(vec, 1, 10));
    EXPECT_TRUE(input_validator::validate_collection_size(vec, 5, 5));
}

QUARISMATEST(input_validator_test, validate_collection_size_invalid)
{
    std::vector<int> vec = {1, 2, 3};
    EXPECT_FALSE(input_validator::validate_collection_size(vec, 5, 10));
    EXPECT_FALSE(input_validator::validate_collection_size(vec, 1, 2));
}

QUARISMATEST(input_validator_test, all_elements_satisfy_valid)
{
    std::vector<int> vec = {2, 4, 6, 8};
    EXPECT_TRUE(input_validator::all_elements_satisfy(vec, [](int x) { return x % 2 == 0; }));
}

QUARISMATEST(input_validator_test, all_elements_satisfy_invalid)
{
    std::vector<int> vec = {2, 3, 4, 6};
    EXPECT_FALSE(input_validator::all_elements_satisfy(vec, [](int x) { return x % 2 == 0; }));
}

// ============================================================================
// Path Validation Tests
// ============================================================================

QUARISMATEST(input_validator_test, is_safe_path_valid)
{
    EXPECT_TRUE(input_validator::is_safe_path("file.txt"));
    EXPECT_TRUE(input_validator::is_safe_path("dir/file.txt"));
    EXPECT_TRUE(input_validator::is_safe_path("a/b/c/file.txt"));
}

QUARISMATEST(input_validator_test, is_safe_path_invalid)
{
    EXPECT_FALSE(input_validator::is_safe_path("../file.txt"));
    EXPECT_FALSE(input_validator::is_safe_path("dir/../file.txt"));
    EXPECT_FALSE(input_validator::is_safe_path("/etc/passwd"));
    EXPECT_FALSE(input_validator::is_safe_path("C:/Windows/System32"));
}

QUARISMATEST(input_validator_test, has_allowed_extension_valid)
{
    std::vector<std::string> allowed = {".txt", ".csv", ".json"};
    EXPECT_TRUE(input_validator::has_allowed_extension("file.txt", allowed));
    EXPECT_TRUE(input_validator::has_allowed_extension("data.csv", allowed));
    EXPECT_TRUE(input_validator::has_allowed_extension("config.json", allowed));
}

QUARISMATEST(input_validator_test, has_allowed_extension_invalid)
{
    std::vector<std::string> allowed = {".txt", ".csv"};
    EXPECT_FALSE(input_validator::has_allowed_extension("file.exe", allowed));
    EXPECT_FALSE(input_validator::has_allowed_extension("script.sh", allowed));
    EXPECT_FALSE(input_validator::has_allowed_extension("noextension", allowed));
}

// ============================================================================
// Safe Conversion Tests
// ============================================================================

QUARISMATEST(input_validator_test, safe_string_to_int_valid)
{
    auto result = input_validator::safe_string_to_int("123");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 123);

    auto negative = input_validator::safe_string_to_int("-456");
    EXPECT_TRUE(negative.has_value());
    EXPECT_EQ(negative.value(), -456);
}

QUARISMATEST(input_validator_test, safe_string_to_int_invalid)
{
    EXPECT_FALSE(input_validator::safe_string_to_int("").has_value());
    EXPECT_FALSE(input_validator::safe_string_to_int("abc").has_value());
    EXPECT_FALSE(input_validator::safe_string_to_int("12.34").has_value());
    EXPECT_FALSE(input_validator::safe_string_to_int("123abc").has_value());
}

QUARISMATEST(input_validator_test, safe_string_to_int_overflow)
{
    std::string too_large = "99999999999999999999";
    EXPECT_FALSE(input_validator::safe_string_to_int<int>(too_large).has_value());
}

QUARISMATEST(input_validator_test, safe_string_to_float_valid)
{
    auto result = input_validator::safe_string_to_float("123.45");
    EXPECT_TRUE(result.has_value());
    EXPECT_NEAR(result.value(), 123.45, 0.001);

    auto negative = input_validator::safe_string_to_float("-67.89");
    EXPECT_TRUE(negative.has_value());
    EXPECT_NEAR(negative.value(), -67.89, 0.001);
}

QUARISMATEST(input_validator_test, safe_string_to_float_invalid)
{
    EXPECT_FALSE(input_validator::safe_string_to_float("").has_value());
    EXPECT_FALSE(input_validator::safe_string_to_float("abc").has_value());
    EXPECT_FALSE(input_validator::safe_string_to_float("12.34abc").has_value());
    EXPECT_FALSE(input_validator::safe_string_to_float("inf").has_value());
    EXPECT_FALSE(input_validator::safe_string_to_float("nan").has_value());
}

QUARISMATEST(input_validator_test, safe_string_to_float_edge_cases)
{
    auto zero = input_validator::safe_string_to_float("0.0");
    EXPECT_TRUE(zero.has_value());
    EXPECT_EQ(zero.value(), 0.0);

    auto scientific = input_validator::safe_string_to_float("1.23e-4");
    EXPECT_TRUE(scientific.has_value());
    EXPECT_NEAR(scientific.value(), 0.000123, 0.0000001);
}
