#include <cmath>
#include <limits>

#include "Core/Testing/baseTest.h"
#include "sanitizer.h"

using namespace quarisma::security;

// ============================================================================
// String Sanitization Tests
// ============================================================================

QUARISMATEST(sanitizer_test, remove_null_bytes)
{
    std::string input = "hello";
    input += '\0';
    input += "world";

    std::string result = sanitizer::remove_null_bytes(input);
    EXPECT_EQ(result, "helloworld");
}

QUARISMATEST(sanitizer_test, remove_null_bytes_no_nulls)
{
    std::string result = sanitizer::remove_null_bytes("hello world");
    EXPECT_EQ(result, "hello world");
}

QUARISMATEST(sanitizer_test, remove_non_printable)
{
    // Use string concatenation to avoid \x01c being interpreted as \x1c
    std::string input =
        "hello\nworld\ttab\x01"
        "control";
    std::string result = sanitizer::remove_non_printable(input);
    EXPECT_EQ(result, "helloworldtabcontrol");
}

QUARISMATEST(sanitizer_test, remove_non_printable_all_printable)
{
    std::string result = sanitizer::remove_non_printable("Hello World 123!");
    EXPECT_EQ(result, "Hello World 123!");
}

QUARISMATEST(sanitizer_test, trim_whitespace)
{
    EXPECT_EQ(sanitizer::trim("  hello  "), "hello");
    EXPECT_EQ(sanitizer::trim("\t\ntest\r\n"), "test");
    EXPECT_EQ(sanitizer::trim("   "), "");
}

QUARISMATEST(sanitizer_test, trim_no_whitespace)
{
    EXPECT_EQ(sanitizer::trim("hello"), "hello");
    EXPECT_EQ(sanitizer::trim(""), "");
}

QUARISMATEST(sanitizer_test, truncate_string)
{
    EXPECT_EQ(sanitizer::truncate("hello world", 5), "hello");
    EXPECT_EQ(sanitizer::truncate("test", 10), "test");
    EXPECT_EQ(sanitizer::truncate("abc", 3), "abc");
}

QUARISMATEST(sanitizer_test, truncate_empty_string)
{
    EXPECT_EQ(sanitizer::truncate("", 5), "");
}

// ============================================================================
// Escape Function Tests
// ============================================================================

QUARISMATEST(sanitizer_test, escape_html)
{
    EXPECT_EQ(
        sanitizer::escape_html("<script>alert('XSS')</script>"),
        "&lt;script&gt;alert(&#39;XSS&#39;)&lt;/script&gt;");
    EXPECT_EQ(sanitizer::escape_html("A & B"), "A &amp; B");
    EXPECT_EQ(sanitizer::escape_html("\"quoted\""), "&quot;quoted&quot;");
}

QUARISMATEST(sanitizer_test, escape_html_no_special_chars)
{
    EXPECT_EQ(sanitizer::escape_html("hello world"), "hello world");
}

QUARISMATEST(sanitizer_test, escape_sql)
{
    EXPECT_EQ(sanitizer::escape_sql("O'Reilly"), "O''Reilly");
    EXPECT_EQ(sanitizer::escape_sql("test\\value"), "test\\\\value");
}

QUARISMATEST(sanitizer_test, escape_sql_null_bytes)
{
    std::string input = "test";
    input += '\0';
    input += "value";
    std::string result = sanitizer::escape_sql(input);
    EXPECT_EQ(result, "testvalue");
}

QUARISMATEST(sanitizer_test, escape_shell)
{
    std::string result = sanitizer::escape_shell("rm -rf /");
    EXPECT_NE(result.find('\\'), std::string::npos);  // Should contain escapes
}

QUARISMATEST(sanitizer_test, escape_shell_safe_string)
{
    std::string result = sanitizer::escape_shell("filename");
    EXPECT_EQ(result, "filename");
}

QUARISMATEST(sanitizer_test, escape_json)
{
    EXPECT_EQ(sanitizer::escape_json("hello\nworld"), "hello\\nworld");
    EXPECT_EQ(sanitizer::escape_json("tab\there"), "tab\\there");
    EXPECT_EQ(sanitizer::escape_json("\"quoted\""), "\\\"quoted\\\"");
    EXPECT_EQ(sanitizer::escape_json("back\\slash"), "back\\\\slash");
}

QUARISMATEST(sanitizer_test, escape_json_control_chars)
{
    std::string input  = "test\x01value";
    std::string result = sanitizer::escape_json(input);
    EXPECT_NE(result.find("\\u"), std::string::npos);
}

QUARISMATEST(sanitizer_test, escape_url)
{
    EXPECT_EQ(sanitizer::escape_url("hello world"), "hello%20world");
    EXPECT_EQ(sanitizer::escape_url("test@example.com"), "test%40example.com");
}

QUARISMATEST(sanitizer_test, escape_url_safe_chars)
{
    std::string result = sanitizer::escape_url("hello-world_123.txt");
    EXPECT_EQ(result, "hello-world_123.txt");
}

// ============================================================================
// Path Sanitization Tests
// ============================================================================

QUARISMATEST(sanitizer_test, sanitize_path_traversal)
{
    EXPECT_EQ(sanitizer::sanitize_path("../etc/passwd"), "etc/passwd");
    EXPECT_EQ(sanitizer::sanitize_path("dir/../file.txt"), "dir/file.txt");
}

QUARISMATEST(sanitizer_test, sanitize_path_absolute)
{
    EXPECT_EQ(sanitizer::sanitize_path("/etc/passwd"), "etc/passwd");
    EXPECT_EQ(sanitizer::sanitize_path("\\Windows\\System32"), "Windows/System32");
}

QUARISMATEST(sanitizer_test, sanitize_path_drive_letter)
{
    EXPECT_EQ(sanitizer::sanitize_path("C:/Windows"), "Windows");
}

QUARISMATEST(sanitizer_test, sanitize_path_multiple_slashes)
{
    EXPECT_EQ(sanitizer::sanitize_path("dir//file.txt"), "dir/file.txt");
}

QUARISMATEST(sanitizer_test, sanitize_filename)
{
    EXPECT_EQ(sanitizer::sanitize_filename("file name.txt"), "file_name.txt");
    EXPECT_EQ(sanitizer::sanitize_filename("test@#$.doc"), "test___.doc");
}

QUARISMATEST(sanitizer_test, sanitize_filename_hidden_file)
{
    EXPECT_EQ(sanitizer::sanitize_filename(".hidden"), "_hidden");
}

QUARISMATEST(sanitizer_test, sanitize_filename_safe)
{
    EXPECT_EQ(sanitizer::sanitize_filename("file-name_123.txt"), "file-name_123.txt");
}

// ============================================================================
// Numeric Sanitization Tests
// ============================================================================

QUARISMATEST(sanitizer_test, clamp_value)
{
    EXPECT_EQ(sanitizer::clamp(5, 1, 10), 5);
    EXPECT_EQ(sanitizer::clamp(0, 1, 10), 1);
    EXPECT_EQ(sanitizer::clamp(15, 1, 10), 10);
}

QUARISMATEST(sanitizer_test, clamp_float)
{
    EXPECT_DOUBLE_EQ(sanitizer::clamp(0.5, 0.0, 1.0), 0.5);
    EXPECT_DOUBLE_EQ(sanitizer::clamp(-0.5, 0.0, 1.0), 0.0);
    EXPECT_DOUBLE_EQ(sanitizer::clamp(1.5, 0.0, 1.0), 1.0);
}

QUARISMATEST(sanitizer_test, sanitize_float_finite)
{
    EXPECT_DOUBLE_EQ(sanitizer::sanitize_float(1.5), 1.5);
    EXPECT_DOUBLE_EQ(sanitizer::sanitize_float(-2.5), -2.5);
}

QUARISMATEST(sanitizer_test, sanitize_float_nan)
{
    double nan_value = std::numeric_limits<double>::quiet_NaN();
    EXPECT_DOUBLE_EQ(sanitizer::sanitize_float(nan_value, 0.0), 0.0);
}

QUARISMATEST(sanitizer_test, sanitize_float_infinity)
{
    double inf_value = std::numeric_limits<double>::infinity();
    EXPECT_DOUBLE_EQ(sanitizer::sanitize_float(inf_value, 0.0), 0.0);

    double neg_inf = -std::numeric_limits<double>::infinity();
    EXPECT_DOUBLE_EQ(sanitizer::sanitize_float(neg_inf, 0.0), 0.0);
}

QUARISMATEST(sanitizer_test, sanitize_float_custom_default)
{
    double nan_value = std::numeric_limits<double>::quiet_NaN();
    EXPECT_DOUBLE_EQ(sanitizer::sanitize_float(nan_value, 42.0), 42.0);
}

// ============================================================================
// Edge Cases and Boundary Tests
// ============================================================================

QUARISMATEST(sanitizer_test, empty_string_handling)
{
    EXPECT_EQ(sanitizer::remove_null_bytes(""), "");
    EXPECT_EQ(sanitizer::remove_non_printable(""), "");
    EXPECT_EQ(sanitizer::trim(""), "");
    EXPECT_EQ(sanitizer::escape_html(""), "");
    EXPECT_EQ(sanitizer::escape_sql(""), "");
    EXPECT_EQ(sanitizer::escape_json(""), "");
    EXPECT_EQ(sanitizer::escape_url(""), "");
}

QUARISMATEST(sanitizer_test, large_string_handling)
{
    std::string large(10000, 'a');
    std::string result = sanitizer::remove_non_printable(large);
    EXPECT_EQ(result.length(), 10000);
}

QUARISMATEST(sanitizer_test, unicode_handling)
{
    // Basic ASCII should pass through
    std::string ascii = "Hello World";
    EXPECT_EQ(sanitizer::remove_non_printable(ascii), ascii);
}

QUARISMATEST(sanitizer_test, all_special_chars_html)
{
    std::string input  = "<>&\"'";
    std::string result = sanitizer::escape_html(input);
    EXPECT_EQ(result, "&lt;&gt;&amp;&quot;&#39;");
}

QUARISMATEST(sanitizer_test, path_with_null_bytes)
{
    std::string path = "dir";
    path += '\0';
    path += "file.txt";
    std::string result = sanitizer::sanitize_path(path);
    EXPECT_EQ(result.find('\0'), std::string::npos);
}
