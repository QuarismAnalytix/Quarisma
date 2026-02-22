# Quarisma Security Module

The Quarisma Security Module provides comprehensive security utilities for the Quarisma framework, implementing best practices from the [SECURITY.md](../../SECURITY.md) policy.

## Overview

This module provides three main components:

1. **Input Validation** (`input_validator.h`) - Validates and sanitizes user input
2. **Data Sanitization** (`sanitizer.h`) - Cleans and normalizes data to prevent injection attacks
3. **Cryptographic Utilities** (`crypto.h`) - Secure hashing, random generation, and cryptographic operations

## Features

### Input Validation

The `input_validator` class provides functions to validate:

- **String validation**: Length, alphanumeric, printable ASCII, regex patterns, null bytes
- **Numeric validation**: Range checking, positive/non-negative checks, finite value checks
- **Collection validation**: Size validation, element predicate validation
- **Path validation**: Safe path checking, extension validation
- **Safe conversion**: String to integer/float with overflow protection

**Example:**
```cpp
#include "input_validator.h"

using namespace quarisma::security;

// Validate string length
if (!input_validator::validate_string_length(user_input, 1, 100)) {
    // Handle invalid input
}

// Safe string to integer conversion
auto value = input_validator::safe_string_to_int<int>(user_input);
if (!value.has_value()) {
    // Handle conversion failure
}

// Validate file path
if (!input_validator::is_safe_path(file_path)) {
    // Reject path traversal attempt
}
```

### Data Sanitization

The `sanitizer` class provides functions to:

- **Clean strings**: Remove null bytes, non-printable characters, trim whitespace
- **Escape data**: HTML, SQL, shell, JSON, URL encoding
- **Sanitize paths**: Remove traversal sequences, sanitize filenames
- **Sanitize numbers**: Clamp values, handle NaN/Infinity

**Example:**
```cpp
#include "sanitizer.h"

using namespace quarisma::security;

// Escape HTML to prevent XSS
std::string safe_html = sanitizer::escape_html(user_input);

// Sanitize file path
std::string safe_path = sanitizer::sanitize_path(user_path);

// Clamp numeric value
int clamped = sanitizer::clamp(user_value, 0, 100);

// Handle NaN/Infinity
double safe_value = sanitizer::sanitize_float(user_float, 0.0);
```

### Cryptographic Utilities

The `crypto` class provides:

- **Secure random generation**: Cryptographically secure random bytes, integers, strings
- **SHA-256 hashing**: Secure hashing with hex output
- **Constant-time comparison**: Prevents timing attacks
- **Utility functions**: Hex encoding/decoding, secure memory zeroing

**Example:**
```cpp
#include "crypto.h"

using namespace quarisma::security;

// Generate secure random bytes
uint8_t buffer[32];
if (crypto::generate_random_bytes(buffer, sizeof(buffer))) {
    // Use random data
}

// Generate random string (e.g., for tokens)
auto token = crypto::generate_random_string(32);
if (token.has_value()) {
    // Use token
}

// Compute SHA-256 hash
std::string hash_hex = crypto::sha256_hex("data to hash");

// Constant-time comparison (prevents timing attacks)
if (crypto::constant_time_compare(hash1, hash2)) {
    // Hashes match
}

// Secure memory zeroing
crypto::secure_zero_memory(sensitive_data, size);
```

## Security Best Practices

### Input Validation

1. **Always validate input** before processing
2. **Use whitelisting** (allow known good) rather than blacklisting (deny known bad)
3. **Validate data types, formats, and ranges**
4. **Check for null bytes** in strings
5. **Validate file paths** to prevent directory traversal

### Data Sanitization

1. **Escape output** based on context (HTML, SQL, shell, etc.)
2. **Use parameterized queries** for SQL (don't rely solely on escaping)
3. **Avoid shell execution** when possible; use direct API calls
4. **Sanitize filenames** to prevent path traversal
5. **Handle NaN/Infinity** in floating-point calculations

### Cryptographic Operations

1. **Use cryptographically secure random** for security-sensitive operations
2. **Use constant-time comparison** for sensitive data (passwords, tokens, hashes)
3. **Zero sensitive memory** after use
4. **Use SHA-256 or stronger** for hashing (not MD5 or SHA-1)
5. **Never roll your own crypto** - use established algorithms

## Platform Support

The security module is cross-platform and uses platform-specific secure APIs:

- **Windows**: BCryptGenRandom for secure random, SecureZeroMemory for memory zeroing
- **macOS**: SecRandomCopyBytes for secure random, Security framework
- **Linux**: getrandom() or /dev/urandom for secure random

## Testing

Comprehensive tests are provided in:

- `TestInputValidator.cpp` - Input validation tests
- `TestSanitizer.cpp` - Sanitization tests
- `TestCrypto.cpp` - Cryptographic operation tests

Run tests with:
```bash
cd build_ninja
ninja test
```

## Integration with SECURITY.md

This module implements the security guidelines from [SECURITY.md](../../SECURITY.md):

| SECURITY.md Guideline | Implementation |
|----------------------|----------------|
| Input validation | `input_validator` class |
| Sanitization | `sanitizer` class |
| SQL injection prevention | `sanitizer::escape_sql()` |
| XSS prevention | `sanitizer::escape_html()` |
| Path traversal prevention | `input_validator::is_safe_path()`, `sanitizer::sanitize_path()` |
| Secure random generation | `crypto::generate_random_*()` |
| Secure hashing | `crypto::sha256()` |
| Constant-time comparison | `crypto::constant_time_compare()` |

## Error Handling

All security functions follow Quarisma's no-exception policy:

- **Validation functions** return `bool` (true = valid, false = invalid)
- **Conversion functions** return `std::optional<T>` (nullopt on failure)
- **Sanitization functions** always return a sanitized result (never throw)
- **Crypto functions** return `bool` or `std::optional<T>` to indicate success/failure

## Performance Considerations

- **Input validation** is fast (O(n) for most operations)
- **Sanitization** may allocate new strings (use judiciously in hot paths)
- **SHA-256** is optimized but still computationally expensive
- **Secure random** uses OS APIs (may block on low entropy)
- **Constant-time comparison** prevents timing attacks but is slower than memcmp

## Dependencies

The security module has minimal dependencies:

- **Standard Library**: `<string>`, `<vector>`, `<optional>`, `<regex>`, etc.
- **Platform APIs**: Windows (bcrypt.lib), macOS (Security framework), Linux (system calls)
- **Quarisma Core**: Common macros and export definitions

## Future Enhancements

Potential future additions:

- [ ] AES encryption/decryption
- [ ] HMAC support
- [ ] Password hashing (bcrypt, scrypt, Argon2)
- [ ] Digital signatures
- [ ] Certificate validation
- [ ] Rate limiting utilities
- [ ] CSRF token generation

## References

- [OWASP Top 10](https://owasp.org/www-project-top-ten/)
- [CWE Top 25](https://cwe.mitre.org/top25/)
- [NIST Cryptographic Standards](https://csrc.nist.gov/projects/cryptographic-standards-and-guidelines)
- [Quarisma Security Policy](../../../SECURITY.md)

## License

This module is part of the Quarisma project and follows the same license.
