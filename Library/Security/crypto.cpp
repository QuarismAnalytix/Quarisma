#include "crypto.h"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>

// Platform-specific includes for secure random
#ifdef _WIN32
// Must come first - defines base Windows types
#include <windows.h>

// Must come after windows.h - depends on Windows types
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#elif defined(__APPLE__)
#include <Security/SecRandom.h>
#elif defined(__linux__) || defined(__unix__)
#include <fcntl.h>
#include <sys/random.h>
#include <unistd.h>
#endif

namespace xsigma
{
namespace security
{

// ============================================================================
// Secure Random Number Generation
// ============================================================================

bool crypto::generate_random_bytes(uint8_t* buffer, size_t size)
{
    if (buffer == nullptr || size == 0)
    {
        return false;
    }

#ifdef _WIN32
    // Windows: Use BCryptGenRandom
    const NTSTATUS status =
        BCryptGenRandom(nullptr, buffer, static_cast<ULONG>(size), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    return status >= 0;

#elif defined(__APPLE__)
    // macOS: Use SecRandomCopyBytes
    return SecRandomCopyBytes(kSecRandomDefault, size, buffer) == 0;

#elif defined(__linux__) || defined(__unix__)
    // Linux: Try getrandom() first, fall back to /dev/urandom
#ifdef SYS_getrandom
    ssize_t result = getrandom(buffer, size, 0);
    if (result == static_cast<ssize_t>(size))
    {
        return true;
    }
#endif

    // Fallback to /dev/urandom
    const int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0)
    {
        return false;
    }

    size_t total_read = 0;
    while (total_read < size)
    {
        const ssize_t bytes_read = read(fd, buffer + total_read, size - total_read);
        if (bytes_read <= 0)
        {
            close(fd);
            return false;
        }
        total_read += static_cast<size_t>(bytes_read);
    }

    close(fd);
    return true;

#else
#error "Unsupported platform for secure random number generation"
#endif
}

std::optional<std::string> crypto::generate_random_string(size_t length, std::string_view charset)
{
    if (length == 0 || charset.empty())
    {
        return std::nullopt;
    }

    std::string result;
    result.reserve(length);

    for (size_t i = 0; i < length; ++i)
    {
        auto random_idx = generate_random_int_range<size_t>(0, charset.size() - 1);
        if (!random_idx.has_value())
        {
            return std::nullopt;
        }
        result += charset[random_idx.value()];
    }

    return result;
}

// ============================================================================
// SHA-256 Implementation
// ============================================================================

// SHA-256 constants
static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

void crypto::sha256_init(sha256_context* ctx)
{
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
    ctx->count    = 0;
    ctx->buffer.fill(0);
}

void crypto::sha256_transform(sha256_context* ctx, const uint8_t* data)
{
    uint32_t m[64];
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
    uint32_t e;
    uint32_t f;
    uint32_t g;
    uint32_t h;

    // Prepare message schedule
    for (int i = 0; i < 16; ++i)
    {
        const size_t offset = static_cast<size_t>(i) * 4;
        m[i]                = (static_cast<uint32_t>(data[offset]) << 24) |
               (static_cast<uint32_t>(data[offset + 1]) << 16) |
               (static_cast<uint32_t>(data[offset + 2]) << 8) |
               (static_cast<uint32_t>(data[offset + 3]));
    }

    for (int i = 16; i < 64; ++i)
    {
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];
    }

    // Initialize working variables
    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];

    // Main loop
    for (int i = 0; i < 64; ++i)
    {
        uint32_t const t1 = h + EP1(e) + CH(e, f, g) + K[i] + m[i];
        uint32_t const t2 = EP0(a) + MAJ(a, b, c);
        h                 = g;
        g                 = f;
        f                 = e;
        e                 = d + t1;
        d                 = c;
        c                 = b;
        b                 = a;
        a                 = t1 + t2;
    }

    // Update state
    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

void crypto::sha256_update(sha256_context* ctx, const uint8_t* data, size_t size)
{
    size_t buffer_space = 64 - (ctx->count % 64);

    ctx->count += size;

    if (size >= buffer_space)
    {
        std::memcpy(ctx->buffer.data() + (64 - buffer_space), data, buffer_space);
        sha256_transform(ctx, ctx->buffer.data());

        data += buffer_space;
        size -= buffer_space;

        while (size >= 64)
        {
            sha256_transform(ctx, data);
            data += 64;
            size -= 64;
        }

        buffer_space = 64;
    }

    if (size > 0)
    {
        std::memcpy(ctx->buffer.data() + (64 - buffer_space), data, size);
    }
}

void crypto::sha256_final(sha256_context* ctx, uint8_t* hash)
{
    size_t const i        = ctx->count % 64;
    uint8_t*     p        = ctx->buffer.data() + i;
    size_t const pad_size = (i < 56) ? (56 - i) : (120 - i);

    *p++ = 0x80;
    std::memset(p, 0, pad_size - 1);

    if (pad_size < 8)
    {
        sha256_transform(ctx, ctx->buffer.data());
        std::memset(ctx->buffer.data(), 0, 56);
    }

    // Append length in bits
    uint64_t const bit_count = ctx->count * 8;
    for (int j = 7; j >= 0; --j)
    {
        ctx->buffer[56 + j] = static_cast<uint8_t>(bit_count >> ((7 - j) * 8));
    }

    sha256_transform(ctx, ctx->buffer.data());

    // Produce final hash
    for (int j = 0; j < 8; ++j)
    {
        const size_t offset = static_cast<size_t>(j) * 4;
        hash[offset]        = static_cast<uint8_t>(ctx->state[j] >> 24);
        hash[offset + 1]    = static_cast<uint8_t>(ctx->state[j] >> 16);
        hash[offset + 2]    = static_cast<uint8_t>(ctx->state[j] >> 8);
        hash[offset + 3]    = static_cast<uint8_t>(ctx->state[j]);
    }
}

std::array<uint8_t, 32> crypto::sha256(const uint8_t* data, size_t size)
{
    sha256_context          ctx;
    std::array<uint8_t, 32> hash;

    sha256_init(&ctx);
    sha256_update(&ctx, data, size);
    sha256_final(&ctx, hash.data());

    return hash;
}

std::array<uint8_t, 32> crypto::sha256(std::string_view str)
{
    return sha256(reinterpret_cast<const uint8_t*>(str.data()), str.size());
}

std::string crypto::sha256_hex(const uint8_t* data, size_t size)
{
    auto hash = sha256(data, size);
    return bytes_to_hex(hash.data(), hash.size());
}

std::string crypto::sha256_hex(std::string_view str)
{
    return sha256_hex(reinterpret_cast<const uint8_t*>(str.data()), str.size());
}

// ============================================================================
// Secure Comparison
// ============================================================================

bool crypto::constant_time_compare(const uint8_t* a, const uint8_t* b, size_t size)
{
    if (a == nullptr || b == nullptr)
    {
        return false;
    }

    volatile uint8_t result = 0;
    for (size_t i = 0; i < size; ++i)
    {
        result |= a[i] ^ b[i];
    }

    return result == 0;
}

bool crypto::constant_time_compare(std::string_view a, std::string_view b)
{
    if (a.size() != b.size())
    {
        return false;
    }

    return constant_time_compare(
        reinterpret_cast<const uint8_t*>(a.data()),
        reinterpret_cast<const uint8_t*>(b.data()),
        a.size());
}

// ============================================================================
// Utility Functions
// ============================================================================

std::string crypto::bytes_to_hex(const uint8_t* data, size_t size)
{
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');

    for (size_t i = 0; i < size; ++i)
    {
        oss << std::setw(2) << static_cast<int>(data[i]);
    }

    return oss.str();
}

std::optional<std::vector<uint8_t>> crypto::hex_to_bytes(std::string_view hex)
{
    if (hex.length() % 2 != 0)
    {
        return std::nullopt;
    }

    std::vector<uint8_t> bytes;
    bytes.reserve(hex.length() / 2);

    for (size_t i = 0; i < hex.length(); i += 2)
    {
        std::string const byte_str(hex.substr(i, 2));
        char*             end;
        long              value = std::strtol(byte_str.c_str(), &end, 16);

        if (end != byte_str.c_str() + 2)
        {
            return std::nullopt;
        }

        bytes.push_back(static_cast<uint8_t>(value));
    }

    return bytes;
}

void crypto::secure_zero_memory(void* ptr, size_t size)
{
    if (ptr == nullptr || size == 0)
    {
        return;
    }

#ifdef _WIN32
    SecureZeroMemory(ptr, size);
#else
    // Use volatile to prevent compiler optimization
    volatile auto* p = static_cast<volatile uint8_t*>(ptr);
    for (size_t i = 0; i < size; ++i)
    {
        p[i] = 0;
    }
#endif
}

}  // namespace security
}  // namespace xsigma
