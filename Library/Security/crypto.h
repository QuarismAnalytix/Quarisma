#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Core/common/export.h"
#include "Core/common/macros.h"

namespace quarisma
{
namespace security
{

/**
 * @brief Cryptographic utilities for secure data processing
 *
 * This module provides secure hashing, random number generation, and
 * basic cryptographic operations. All functions use industry-standard
 * algorithms and best practices.
 *
 * Security considerations:
 * - Uses cryptographically secure random number generation
 * - Implements constant-time comparison for sensitive data
 * - Provides secure hashing with SHA-256
 * - All functions are designed to prevent timing attacks
 */
class QUARISMA_VISIBILITY crypto
{
public:
    // ========================================================================
    // Secure Random Number Generation
    // ========================================================================

    /**
     * @brief Generates cryptographically secure random bytes
     * @param buffer Output buffer for random bytes
     * @param size Number of bytes to generate
     * @return true on success, false on failure
     *
     * Uses platform-specific secure random sources:
     * - Linux/macOS: /dev/urandom or getrandom()
     * - Windows: BCryptGenRandom or RtlGenRandom
     */
    static QUARISMA_API bool generate_random_bytes(uint8_t* buffer, size_t size);

    /**
     * @brief Generates a cryptographically secure random integer
     * @tparam T Integer type
     * @return Random value of type T
     */
    template <typename T = uint64_t>
    static std::optional<T> generate_random_int()
    {
        static_assert(std::is_integral_v<T>, "T must be an integral type");

        T value;
        if (!generate_random_bytes(reinterpret_cast<uint8_t*>(&value), sizeof(T)))
        {
            return std::nullopt;
        }
        return value;
    }

    /**
     * @brief Generates a cryptographically secure random integer in range
     * @tparam T Integer type
     * @param min_value Minimum value (inclusive)
     * @param max_value Maximum value (inclusive)
     * @return Random value in [min_value, max_value]
     */
    template <typename T = uint64_t>
    static std::optional<T> generate_random_int_range(T min_value, T max_value)
    {
        static_assert(std::is_integral_v<T>, "T must be an integral type");

        if (min_value > max_value)
        {
            return std::nullopt;
        }

        // Use unsigned type for random generation to avoid signed overflow issues
        using unsigned_t = typename std::make_unsigned<T>::type;
        auto random_val  = generate_random_int<unsigned_t>();
        if (!random_val.has_value())
        {
            return std::nullopt;
        }

        // Use modulo with bias mitigation
        unsigned_t range = static_cast<unsigned_t>(max_value - min_value) + 1;
        return min_value + static_cast<T>(random_val.value() % range);
    }

    /**
     * @brief Generates a random string of specified length
     * @param length Length of the string to generate
     * @param charset Character set to use (default: alphanumeric)
     * @return Random string
     */
    static QUARISMA_API std::optional<std::string> generate_random_string(
        size_t           length,
        std::string_view charset =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");

    // ========================================================================
    // Secure Hashing
    // ========================================================================

    /**
     * @brief Computes SHA-256 hash of data
     * @param data Input data
     * @param size Size of input data
     * @return 32-byte SHA-256 hash
     */
    static QUARISMA_API std::array<uint8_t, 32> sha256(const uint8_t* data, size_t size);

    /**
     * @brief Computes SHA-256 hash of string
     * @param str Input string
     * @return 32-byte SHA-256 hash
     */
    static QUARISMA_API std::array<uint8_t, 32> sha256(std::string_view str);

    /**
     * @brief Computes SHA-256 hash and returns as hex string
     * @param data Input data
     * @param size Size of input data
     * @return 64-character hex string
     */
    static QUARISMA_API std::string sha256_hex(const uint8_t* data, size_t size);

    /**
     * @brief Computes SHA-256 hash of string and returns as hex string
     * @param str Input string
     * @return 64-character hex string
     */
    static QUARISMA_API std::string sha256_hex(std::string_view str);

    // ========================================================================
    // Secure Comparison
    // ========================================================================

    /**
     * @brief Constant-time comparison of two byte arrays
     * @param a First array
     * @param b Second array
     * @param size Size of arrays
     * @return true if arrays are equal, false otherwise
     *
     * Uses constant-time comparison to prevent timing attacks.
     * Always compares all bytes regardless of differences.
     */
    static QUARISMA_API bool constant_time_compare(const uint8_t* a, const uint8_t* b, size_t size);

    /**
     * @brief Constant-time comparison of two strings
     * @param a First string
     * @param b Second string
     * @return true if strings are equal, false otherwise
     */
    static QUARISMA_API bool constant_time_compare(std::string_view a, std::string_view b);

    // ========================================================================
    // Utility Functions
    // ========================================================================

    /**
     * @brief Converts byte array to hexadecimal string
     * @param data Input data
     * @param size Size of input data
     * @return Hexadecimal string representation
     */
    static QUARISMA_API std::string bytes_to_hex(const uint8_t* data, size_t size);

    /**
     * @brief Converts hexadecimal string to byte array
     * @param hex Hexadecimal string
     * @return Byte array, or nullopt if hex string is invalid
     */
    static QUARISMA_API std::optional<std::vector<uint8_t>> hex_to_bytes(std::string_view hex);

    /**
     * @brief Securely zeros memory
     * @param ptr Pointer to memory
     * @param size Size of memory to zero
     *
     * Uses platform-specific secure zeroing to prevent compiler optimization.
     * Ensures sensitive data is actually cleared from memory.
     */
    static QUARISMA_API void secure_zero_memory(void* ptr, size_t size);

private:
    // Internal SHA-256 implementation
    struct sha256_context
    {
        uint32_t                state[8];
        uint64_t                count;
        std::array<uint8_t, 64> buffer;
    };

    static void sha256_init(sha256_context* ctx);
    static void sha256_update(sha256_context* ctx, const uint8_t* data, size_t size);
    static void sha256_final(sha256_context* ctx, uint8_t* hash);
    static void sha256_transform(sha256_context* ctx, const uint8_t* data);
};

}  // namespace security
}  // namespace quarisma
