#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "Core/common/export.h"
#include "Core/common/macros.h"

namespace quarisma
{
namespace security
{

/**
 * @brief Input validation utilities for secure data processing
 *
 * This module provides comprehensive input validation functions to prevent
 * security vulnerabilities such as buffer overflows, injection attacks,
 * and malformed data processing.
 *
 * All validation functions follow a consistent pattern:
 * - Return bool for simple validation checks
 * - Return std::optional<T> for validated and converted values
 * - Never throw exceptions (use return values for error handling)
 */
class QUARISMA_VISIBILITY input_validator
{
public:
    // ========================================================================
    // String Validation
    // ========================================================================

    /**
     * @brief Validates string length is within acceptable bounds
     * @param str String to validate
     * @param min_length Minimum acceptable length (inclusive)
     * @param max_length Maximum acceptable length (inclusive)
     * @return true if length is valid, false otherwise
     */
    static QUARISMA_API bool validate_string_length(
        std::string_view str, size_t min_length, size_t max_length);

    /**
     * @brief Validates string contains only alphanumeric characters
     * @param str String to validate
     * @return true if string is alphanumeric, false otherwise
     */
    static QUARISMA_API bool is_alphanumeric(std::string_view str);

    /**
     * @brief Validates string contains only ASCII printable characters
     * @param str String to validate
     * @return true if string contains only printable ASCII, false otherwise
     */
    static QUARISMA_API bool is_printable_ascii(std::string_view str);

    /**
     * @brief Validates string matches a given regex pattern
     * @param str String to validate
     * @param pattern Regex pattern to match
     * @return true if string matches pattern, false otherwise
     */
    static QUARISMA_API bool matches_pattern(std::string_view str, const std::regex& pattern);

    /**
     * @brief Validates string does not contain null bytes
     * @param str String to validate
     * @return true if no null bytes found, false otherwise
     */
    static QUARISMA_API bool has_no_null_bytes(std::string_view str);

    // ========================================================================
    // Numeric Validation
    // ========================================================================

    /**
     * @brief Validates numeric value is within range
     * @tparam T Numeric type
     * @param value Value to validate
     * @param min_value Minimum acceptable value (inclusive)
     * @param max_value Maximum acceptable value (inclusive)
     * @return true if value is in range, false otherwise
     */
    template <typename T>
    static bool validate_range(T value, T min_value, T max_value)
    {
        static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");
        return value >= min_value && value <= max_value;
    }

    /**
     * @brief Validates value is positive (> 0)
     * @tparam T Numeric type
     * @param value Value to validate
     * @return true if value is positive, false otherwise
     */
    template <typename T>
    static bool is_positive(T value)
    {
        static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");
        return value > T(0);
    }

    /**
     * @brief Validates value is non-negative (>= 0)
     * @tparam T Numeric type
     * @param value Value to validate
     * @return true if value is non-negative, false otherwise
     */
    template <typename T>
    static bool is_non_negative(T value)
    {
        static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");
        return value >= T(0);
    }

    /**
     * @brief Validates floating-point value is finite (not NaN or infinity)
     * @tparam T Floating-point type
     * @param value Value to validate
     * @return true if value is finite, false otherwise
     */
    template <typename T>
    static bool is_finite(T value)
    {
        static_assert(std::is_floating_point_v<T>, "T must be a floating-point type");
        return std::isfinite(value);
    }

    // ========================================================================
    // Collection Validation
    // ========================================================================

    /**
     * @brief Validates collection size is within bounds
     * @tparam Container Container type
     * @param container Container to validate
     * @param min_size Minimum acceptable size (inclusive)
     * @param max_size Maximum acceptable size (inclusive)
     * @return true if size is valid, false otherwise
     */
    template <typename Container>
    static bool validate_collection_size(
        const Container& container, size_t min_size, size_t max_size)
    {
        const size_t size = container.size();
        return size >= min_size && size <= max_size;
    }

    /**
     * @brief Validates all elements in collection satisfy a predicate
     * @tparam Container Container type
     * @tparam Predicate Predicate function type
     * @param container Container to validate
     * @param pred Predicate function
     * @return true if all elements satisfy predicate, false otherwise
     */
    template <typename Container, typename Predicate>
    static bool all_elements_satisfy(const Container& container, Predicate pred)
    {
        return std::all_of(container.begin(), container.end(), pred);
    }

    // ========================================================================
    // Path Validation
    // ========================================================================

    /**
     * @brief Validates file path does not contain path traversal sequences
     * @param path File path to validate
     * @return true if path is safe, false if it contains traversal sequences
     */
    static QUARISMA_API bool is_safe_path(std::string_view path);

    /**
     * @brief Validates file extension is in allowed list
     * @param filename Filename to validate
     * @param allowed_extensions List of allowed extensions (e.g., {".txt", ".csv"})
     * @return true if extension is allowed, false otherwise
     */
    static QUARISMA_API bool has_allowed_extension(
        std::string_view filename, const std::vector<std::string>& allowed_extensions);

    // ========================================================================
    // Safe Conversion
    // ========================================================================

    /**
     * @brief Safely converts string to integer with validation
     * @tparam T Integer type
     * @param str String to convert
     * @return Optional containing converted value, or nullopt on failure
     */
    template <typename T = int>
    static std::optional<T> safe_string_to_int(std::string_view str)
    {
        static_assert(std::is_integral_v<T>, "T must be an integral type");

        if (str.empty())
        {
            return std::nullopt;
        }

        try
        {
            size_t      pos = 0;
            std::string s(str);

            if constexpr (std::is_signed_v<T>)
            {
                long long value = std::stoll(s, &pos);
                if (pos != s.length())
                {
                    return std::nullopt;
                }
                if (value < std::numeric_limits<T>::min() || value > std::numeric_limits<T>::max())
                {
                    return std::nullopt;
                }
                return static_cast<T>(value);
            }
            else
            {
                unsigned long long value = std::stoull(s, &pos);
                if (pos != s.length())
                {
                    return std::nullopt;
                }
                if (value > std::numeric_limits<T>::max())
                {
                    return std::nullopt;
                }
                return static_cast<T>(value);
            }
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    /**
     * @brief Safely converts string to floating-point with validation
     * @tparam T Floating-point type
     * @param str String to convert
     * @return Optional containing converted value, or nullopt on failure
     */
    template <typename T = double>
    static std::optional<T> safe_string_to_float(std::string_view str)
    {
        static_assert(std::is_floating_point_v<T>, "T must be a floating-point type");

        if (str.empty())
        {
            return std::nullopt;
        }

        try
        {
            size_t      pos = 0;
            std::string s(str);
            double      value = std::stod(s, &pos);

            if (pos != s.length())
            {
                return std::nullopt;
            }

            if (!std::isfinite(value))
            {
                return std::nullopt;
            }

            return static_cast<T>(value);
        }
        catch (...)
        {
            return std::nullopt;
        }
    }
};

}  // namespace security
}  // namespace quarisma
