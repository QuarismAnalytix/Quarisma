#pragma once

#include <cmath>
#include <string>
#include <string_view>
#include <unordered_map>

#include "Core/common/export.h"
#include "Core/common/macros.h"

namespace quarisma
{
namespace security
{

/**
 * @brief Data sanitization utilities for secure data processing
 *
 * This module provides functions to clean and normalize data to prevent
 * injection attacks, XSS, and other security vulnerabilities.
 *
 * All sanitization functions:
 * - Return sanitized copies (do not modify input)
 * - Are safe to call with any input
 * - Never throw exceptions
 */
class QUARISMA_VISIBILITY sanitizer
{
public:
    // ========================================================================
    // String Sanitization
    // ========================================================================

    /**
     * @brief Removes all null bytes from a string
     * @param str Input string
     * @return String with null bytes removed
     */
    static QUARISMA_API std::string remove_null_bytes(std::string_view str);

    /**
     * @brief Removes all non-printable ASCII characters
     * @param str Input string
     * @return String with only printable ASCII characters
     */
    static QUARISMA_API std::string remove_non_printable(std::string_view str);

    /**
     * @brief Trims whitespace from both ends of a string
     * @param str Input string
     * @return Trimmed string
     */
    static QUARISMA_API std::string trim(std::string_view str);

    /**
     * @brief Truncates string to maximum length
     * @param str Input string
     * @param max_length Maximum allowed length
     * @return Truncated string
     */
    static QUARISMA_API std::string truncate(std::string_view str, size_t max_length);

    // ========================================================================
    // Escape/Unescape Functions
    // ========================================================================

    /**
     * @brief Escapes HTML special characters to prevent XSS
     * @param str Input string
     * @return HTML-escaped string
     *
     * Escapes: < > & " '
     */
    static QUARISMA_API std::string escape_html(std::string_view str);

    /**
     * @brief Escapes SQL special characters to prevent SQL injection
     * @param str Input string
     * @return SQL-escaped string
     *
     * Note: This is a basic escape. Use parameterized queries when possible.
     */
    static QUARISMA_API std::string escape_sql(std::string_view str);

    /**
     * @brief Escapes shell special characters to prevent command injection
     * @param str Input string
     * @return Shell-escaped string
     *
     * Note: Avoid shell execution when possible. Use direct API calls instead.
     */
    static QUARISMA_API std::string escape_shell(std::string_view str);

    /**
     * @brief Escapes JSON special characters
     * @param str Input string
     * @return JSON-escaped string
     */
    static QUARISMA_API std::string escape_json(std::string_view str);

    /**
     * @brief Escapes URL special characters (percent encoding)
     * @param str Input string
     * @return URL-encoded string
     */
    static QUARISMA_API std::string escape_url(std::string_view str);

    // ========================================================================
    // Path Sanitization
    // ========================================================================

    /**
     * @brief Sanitizes file path by removing traversal sequences
     * @param path Input path
     * @return Sanitized path
     *
     * Removes: .. sequences, leading slashes, drive letters
     */
    static QUARISMA_API std::string sanitize_path(std::string_view path);

    /**
     * @brief Sanitizes filename by removing special characters
     * @param filename Input filename
     * @return Sanitized filename with only safe characters
     *
     * Keeps: alphanumeric, underscore, hyphen, dot
     */
    static QUARISMA_API std::string sanitize_filename(std::string_view filename);

    // ========================================================================
    // Numeric Sanitization
    // ========================================================================

    /**
     * @brief Clamps numeric value to specified range
     * @tparam T Numeric type
     * @param value Input value
     * @param min_value Minimum allowed value
     * @param max_value Maximum allowed value
     * @return Clamped value
     */
    template <typename T>
    static T clamp(T value, T min_value, T max_value)
    {
        if (value < min_value)
            return min_value;
        if (value > max_value)
            return max_value;
        return value;
    }

    /**
     * @brief Sanitizes floating-point value (replaces NaN/Inf with default)
     * @tparam T Floating-point type
     * @param value Input value
     * @param default_value Value to use if input is NaN or Inf
     * @return Sanitized value
     */
    template <typename T>
    static T sanitize_float(T value, T default_value = T(0))
    {
        static_assert(std::is_floating_point_v<T>, "T must be a floating-point type");
        return std::isfinite(value) ? value : default_value;
    }

private:
    // Helper function for character replacement
    static std::string replace_chars(
        std::string_view str, const std::unordered_map<char, std::string_view>& replacements);
};

}  // namespace security
}  // namespace quarisma
