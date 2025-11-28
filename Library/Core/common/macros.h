/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * This file is part of XSigma and is licensed under a dual-license model:
 *
 *   - Open-source License (GPLv3):
 *       Free for personal, academic, and research use under the terms of
 *       the GNU General Public License v3.0 or later.
 *
 *   - Commercial License:
 *       A commercial license is required for proprietary, closed-source,
 *       or SaaS usage. Contact us to obtain a commercial agreement.
 *
 * Contact: licensing@xsigma.co.uk
 * Website: https://www.xsigma.co.uk
 */

#pragma once

#include <cstring>
#include <string>
#include <type_traits>  // for std::underlying_type
#include <typeinfo>

#include "common/configure.h"

//----------------------------------------------------------------------------
// Check for unsupported old compilers - updated minimum requirements
#if defined(_MSC_VER) && _MSC_VER < 1910  // VS2017 15.0 minimum for C++17
#error XSIGMA requires MSVC++ 15.0 (Visual Studio 2017) or newer for C++17 support
#endif

#if !defined(__clang__) && defined(__GNUC__) && \
    (__GNUC__ < 7 || (__GNUC__ == 7 && __GNUC_MINOR__ < 1))  // GCC 7.1+ for C++17
#error XSIGMA requires GCC 7.1 or newer for C++17 support
#endif

#if defined(__clang__) && (__clang_major__ < 5)  // Clang 5.0+ for C++17
#error XSIGMA requires Clang 5.0 or newer for C++17 support
#endif

//------------------------------------------------------------------------
#ifdef XSIGMA_MOBILE
// Use 16-byte alignment on mobile
// - ARM NEON AArch32 and AArch64
// - x86[-64] < AVX
inline constexpr size_t XSIGMA_ALIGNMENT = 16;
#else
// Use 64-byte alignment should be enough for computation up to AVX512.
inline constexpr size_t XSIGMA_ALIGNMENT = 64;
#endif

//------------------------------------------------------------------------
// DLL Export/Import macros - include from separate export header
#include "export.h"

//------------------------------------------------------------------------
#if defined(_MSC_VER)
#if (_MSC_VER < 1900)
// Visual studio until 2015 is not supporting standard 'alignas' keyword
#ifdef alignas
// This check can be removed when verified that for all other versions alignas
// works as requested
#error "XSIGMA error: alignas already defined"
#else
#define alignas(alignment) __declspec(align(alignment))
#endif
#endif

#ifdef alignas
#define XSIGMA_ALIGN(alignment) alignas(alignment)
#else
#define XSIGMA_ALIGN(alignment) __declspec(align(alignment))
#endif
#elif defined(__GNUC__)
#define XSIGMA_ALIGN(alignment) __attribute__((aligned(alignment)))
#elif defined(__ICC) || defined(__INTEL_COMPILER)
#endif

//------------------------------------------------------------------------
#if defined(_MSC_VER)
#define XSIGMA_RESTRICT __restrict
#elif defined(__cplusplus)
// C++ doesn't have standard restrict, use compiler extensions
#if defined(__GNUC__) || defined(__clang__)
#define XSIGMA_RESTRICT __restrict__
#else
#define XSIGMA_RESTRICT
#endif
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
// C99 and later
#define XSIGMA_RESTRICT restrict
#else
// Fallback for older compilers
#define XSIGMA_RESTRICT
#endif

//------------------------------------------------------------------------
inline constexpr int XSIGMA_COMPILE_TIME_MAX_GPUS = 16;

//------------------------------------------------------------------------
/* Various compiler-specific performance hints. keep compiler order! */
#ifdef __XSIGMA_WRAP__
#define XSIGMA_VECTORCALL
#define XSIGMA_FORCE_INLINE inline

#elif defined(_MSC_VER)
#define XSIGMA_VECTORCALL __vectorcall
#define XSIGMA_FORCE_INLINE __forceinline

#elif defined(__INTEL_COMPILER)
#define XSIGMA_VECTORCALL
#define XSIGMA_FORCE_INLINE inline __attribute__((always_inline))

#elif defined(__clang__)
#define XSIGMA_VECTORCALL
#define XSIGMA_FORCE_INLINE inline __attribute__((always_inline))

#elif defined(__GNUC__)
#define XSIGMA_VECTORCALL
#define XSIGMA_FORCE_INLINE inline __attribute__((always_inline))

#else
#define XSIGMA_VECTORCALL
#define XSIGMA_FORCE_INLINE inline
#endif

//------------------------------------------------------------------------
#if (defined(__GNUC__) || defined(__APPLE__)) && !defined(SWIG)
// Compiler supports GCC-style attributes
#define XSIGMA_NORETURN __attribute__((noreturn))
#define XSIGMA_NOINLINE __attribute__((noinline))
#define XSIGMA_COLD __attribute__((cold))
#elif defined(_MSC_VER)
// Non-GCC equivalents
#define XSIGMA_NORETURN __declspec(noreturn)
#define XSIGMA_NOINLINE
#define XSIGMA_COLD
#else
// Non-GCC equivalents
#define XSIGMA_NORETURN
#define XSIGMA_NOINLINE
#define XSIGMA_COLD
#endif

//------------------------------------------------------------------------
#ifdef NDEBUG
#define XSIGMA_SIMD_RETURN_TYPE XSIGMA_FORCE_INLINE static void XSIGMA_VECTORCALL
#else
#define XSIGMA_SIMD_RETURN_TYPE static void
#endif

//------------------------------------------------------------------------
// set function attribute for CUDA
#if defined(__CUDACC__) || defined(__HIPCC__)
#define XSIGMA_CUDA_DEVICE __device__
#define XSIGMA_CUDA_HOST __host__
#define XSIGMA_CUDA_FUNCTION_TYPE __host__ __device__
#else
#define XSIGMA_CUDA_DEVICE
#define XSIGMA_CUDA_HOST
#define XSIGMA_CUDA_FUNCTION_TYPE
#endif

#ifdef NDEBUG
#define XSIGMA_FUNCTION_ATTRIBUTE XSIGMA_FORCE_INLINE XSIGMA_CUDA_FUNCTION_TYPE
#else
#define XSIGMA_FUNCTION_ATTRIBUTE XSIGMA_CUDA_FUNCTION_TYPE
#endif

//----------------------------------------------------------------------------
#if defined(__GNUC__) && \
    (defined(__i386__) || defined(__x86_64__) || defined(__arm__) || defined(__aarch64__))
#define XSIGMA_ASM_COMMENT(X) __asm__("#" X)
#else
#define XSIGMA_ASM_COMMENT(X)
#endif

//----------------------------------------------------------------------------
#define XSIGMA_CONCATENATE_IMPL(s1, s2) s1##s2
#define XSIGMA_CONCATENATE(s1, s2) XSIGMA_CONCATENATE_IMPL(s1, s2)

//----------------------------------------------------------------------------
#ifdef __COUNTER__
#define XSIGMA_UID __COUNTER__
#define XSIGMA_ANONYMOUS_VARIABLE(str) XSIGMA_CONCATENATE(str, __COUNTER__)
#else
#define XSIGMA_UID __LINE__
#define XSIGMA_ANONYMOUS_VARIABLE(str) XSIGMA_CONCATENATE(str, __LINE__)
#endif

//----------------------------------------------------------------------------
#if !XSIGMA_HAS_THREE_WAY_COMPARISON
#if defined(__cpp_impl_three_way_comparison) && __cpp_impl_three_way_comparison >= 201907L && \
    defined(__cpp_lib_three_way_comparison) && __cpp_lib_three_way_comparison >= 201907L
#define XSIGMA_HAS_THREE_WAY_COMPARISON 1
#else
#define XSIGMA_HAS_THREE_WAY_COMPARISON 0
#endif
#endif

//----------------------------------------------------------------------------
// A function level attribute to disable checking for use of uninitialized
// memory when built with MemorySanitizer.
#if defined(__clang__)
#if __has_feature(memory_sanitizer)
#define XSIGMA_NO_SANITIZE_MEMORY __attribute__((no_sanitize_memory))
#else
#define XSIGMA_NO_SANITIZE_MEMORY
#endif  // __has_feature(memory_sanitizer)
#else
#define XSIGMA_NO_SANITIZE_MEMORY
#endif  // __clang__

//----------------------------------------------------------------------------
#define MACRO_CORE_TYPE_ID_NAME(x) typeid(x).name()

#define XSIGMA_DELETE_CLASS(type)            \
    type()                         = delete; \
    type(const type&)              = delete; \
    type& operator=(const type& a) = delete; \
    type(type&&)                   = delete; \
    type& operator=(type&&)        = delete; \
    ~type()                        = delete;

#define XSIGMA_DELETE_COPY_AND_MOVE(type)    \
private:                                     \
    type(const type&)              = delete; \
    type& operator=(const type& a) = delete; \
    type(type&&)                   = delete; \
    type& operator=(type&&)        = delete; \
                                             \
public:

#define XSIGMA_DELETE_COPY(type)             \
    type(const type&)              = delete; \
    type& operator=(const type& a) = delete;

//----------------------------------------------------------------------------
// format string checking.
#if !defined(MACRO_CORE_PRINTF_FORMAT)
#if defined(__GNUC__)
#define MACRO_CORE_PRINTF_FORMAT(a, b) __attribute__((format(printf, a, b)))
#else
#define MACRO_CORE_PRINTF_FORMAT(a, b)
#endif
#endif

//----------------------------------------------------------------------------
namespace xsigma
{
template <typename...>
using void_t = std::void_t<>;
}  // namespace xsigma

//----------------------------------------------------------------------------
#if (!defined(__INTEL_COMPILER)) & defined(_MSC_VER)
#define xsigma_int __int64
#define xsigma_long unsigned __int64
#else
#define xsigma_int long long int
#define xsigma_long unsigned long long int
#endif

//----------------------------------------------------------------------------
#if defined(__cplusplus) && defined(__has_cpp_attribute)
#define XSIGMA_HAVE_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
#define XSIGMA_HAVE_CPP_ATTRIBUTE(x) 0
#endif

//----------------------------------------------------------------------------
#ifdef __has_attribute
#define XSIGMA_HAVE_ATTRIBUTE(x) __has_attribute(x)
#else
#define XSIGMA_HAVE_ATTRIBUTE(x) 0
#endif

//------------------------------------------------------------------------
// C++20 [[likely]] and [[unlikely]] attributes are only available in C++20 and later
#if __cplusplus >= 202002L && XSIGMA_HAVE_CPP_ATTRIBUTE(likely) && \
    XSIGMA_HAVE_CPP_ATTRIBUTE(unlikely)
#define XSIGMA_LIKELY(expr) (expr) [[likely]]
#define XSIGMA_UNLIKELY(expr) (expr) [[unlikely]]
#elif defined(__GNUC__) || defined(__ICL) || defined(__clang__)
#define XSIGMA_LIKELY(expr) (__builtin_expect(static_cast<bool>(expr), 1))
#define XSIGMA_UNLIKELY(expr) (__builtin_expect(static_cast<bool>(expr), 0))
#else
#define XSIGMA_LIKELY(expr) (expr)
#define XSIGMA_UNLIKELY(expr) (expr)
#endif

//----------------------------------------------------------------------------
#if __cplusplus >= 202002L
#define XSIGMA_FUNCTION_CONSTEXPR constexpr
#else
#define XSIGMA_FUNCTION_CONSTEXPR
#endif

//----------------------------------------------------------------------------
#if __cplusplus >= 201703L
#define XSIGMA_NODISCARD [[nodiscard]]
#else
#define XSIGMA_NODISCARD
#endif

//----------------------------------------------------------------------------
#if __cplusplus >= 201703L
#define XSIGMA_UNUSED [[maybe_unused]]
#elif defined(__GNUC__) || defined(__clang__)
// For GCC or Clang: use __attribute__
#define XSIGMA_UNUSED __attribute__((unused))
#elif defined(_MSC_VER)
// For MSVC
#define XSIGMA_UNUSED __pragma(warning(suppress : 4100))
#else
// Fallback for other compilers
#define XSIGMA_UNUSED
#endif

// Check for MSVC first, then Clang/GCC
#if defined(_MSC_VER)
// MSVC doesn't support __attribute__((used))
// Use __pragma(comment(linker, "/include:symbol")) or just leave empty
#define XSIGMA_USED
#elif defined(__has_attribute)
#if __has_attribute(used)
#define XSIGMA_USED __attribute__((__used__))
#else
#define XSIGMA_USED
#endif
#else
#define XSIGMA_USED
#endif

//----------------------------------------------------------------------------
// Demangle
#if defined(__ANDROID__) && (defined(__i386__) || defined(__x86_64__))
#define XSIGMA_HAS_CXA_DEMANGLE 0
#elif (__GNUC__ >= 4 || (__GNUC__ >= 3 && __GNUC_MINOR__ >= 4)) && !defined(__mips__)
#define XSIGMA_HAS_CXA_DEMANGLE 1
#elif defined(__clang__) && !defined(_MSC_VER)
#define XSIGMA_HAS_CXA_DEMANGLE 1
#else
#define XSIGMA_HAS_CXA_DEMANGLE 0
#endif

//----------------------------------------------------------------------------
// Printf-style format checking
#if XSIGMA_HAVE_ATTRIBUTE(format)
#define XSIGMA_PRINTF_ATTRIBUTE(format_index, first_to_check) \
    __attribute__((format(printf, format_index, first_to_check)))
#else
#define XSIGMA_PRINTF_ATTRIBUTE(format_index, first_to_check)
#endif

//----------------------------------------------------------------------------
// Thread safety analysis disable
#if XSIGMA_HAVE_ATTRIBUTE(no_thread_safety_analysis)
#define XSIGMA_NO_THREAD_SAFETY_ANALYSIS __attribute__((no_thread_safety_analysis))
#else
#define XSIGMA_NO_THREAD_SAFETY_ANALYSIS
#endif

//----------------------------------------------------------------------------
// Thread safety - guarded by mutex
#if XSIGMA_HAVE_ATTRIBUTE(guarded_by)
#define XSIGMA_GUARDED_BY(x) __attribute__((guarded_by(x)))
#else
#define XSIGMA_GUARDED_BY(x)
#endif

//----------------------------------------------------------------------------
// Thread safety - exclusive locks required
#if XSIGMA_HAVE_ATTRIBUTE(exclusive_locks_required)
#define XSIGMA_EXCLUSIVE_LOCKS_REQUIRED(...) __attribute__((exclusive_locks_required(__VA_ARGS__)))
#else
#define XSIGMA_EXCLUSIVE_LOCKS_REQUIRED(...)
#endif

//----------------------------------------------------------------------------
#if XSIGMA_HAVE_CPP_ATTRIBUTE(clang::lifetimebound)
#define XSIGMA_LIFETIME_BOUND [[clang::lifetimebound]]
#elif XSIGMA_HAVE_CPP_ATTRIBUTE(msvc::lifetimebound)
#define XSIGMA_LIFETIME_BOUND [[msvc::lifetimebound]]
#elif XSIGMA_HAVE_ATTRIBUTE(lifetimebound)
#define XSIGMA_LIFETIME_BOUND __attribute__((lifetimebound))
#else
#define XSIGMA_LIFETIME_BOUND
#endif

//----------------------------------------------------------------------------
#if XSIGMA_HAVE_ATTRIBUTE(locks_excluded)
#define XSIGMA_LOCKS_EXCLUDED(...) __attribute__((locks_excluded(__VA_ARGS__)))
#else
#define XSIGMA_LOCKS_EXCLUDED(...)
#endif

//----------------------------------------------------------------------------
#if XSIGMA_HAVE_CPP_ATTRIBUTE(clang::require_constant_initialization)
#define XSIGMA_CONST_INIT [[clang::require_constant_initialization]]
#else
#define XSIGMA_CONST_INIT
#endif

//----------------------------------------------------------------------------
// Define missing Darwin types for Homebrew Clang (simplified for C++17)
#if defined(__APPLE__) && !defined(__DEFINED_DARWIN_TYPES)
#define __DEFINED_DARWIN_TYPES
using __uint32_t = std::uint32_t;
using __uint64_t = std::uint64_t;
using __int32_t  = std::int32_t;
using __int64_t  = std::int64_t;
using __uint8_t  = std::uint8_t;
using __uint16_t = std::uint16_t;
using __int8_t   = std::int8_t;
using __int16_t  = std::int16_t;
#endif