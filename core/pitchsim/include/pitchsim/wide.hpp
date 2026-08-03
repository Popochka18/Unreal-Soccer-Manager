#pragma once

// 128-bit intermediates for fixed-point arithmetic — CLAUDE.md §3, §6.
//
// q16.16 multiply needs 128 bits before the shift, and q16.16 divide needs 128
// bits after it. GCC and Clang have __int128; MSVC has neither that nor a
// constexpr route to the same answer.
//
// The design that keeps §6 honest:
//
//   * ONE portable implementation, used at compile time by EVERY compiler.
//   * Native fast paths used only at runtime, where they matter for the §7
//     budget (fixed-point multiply is the hottest operation in the engine).
//   * A differential test that proves the two agree bit-for-bit.
//
// The alternative — each compiler using its own path in both contexts — is how
// you get a Windows build that agrees with Linux on 99.99% of matches. That
// failure is invisible in every test that does not compare the two directly,
// which is why test_wide.cpp exists and why it is not optional.
//
// Everything here floors (rounds toward negative infinity), matching fixed.hpp.

#include <cstdint>
#include <type_traits>

#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#endif

namespace pitchsim::wide {

/// A 128-bit value as two 64-bit limbs, two's complement when interpreted signed.
struct u128 {
    std::uint64_t hi = 0;
    std::uint64_t lo = 0;
};

/// Quotient and remainder of an unsigned 128/64 division.
struct divmod64 {
    std::uint64_t quotient = 0;
    std::uint64_t remainder = 0;
};

// ---------------------------------------------------------------------------
// Portable reference implementation. constexpr, no compiler extensions, no
// intrinsics. This is the definition of correct behaviour; every other path in
// this header is an optimisation that must reproduce it exactly.
// ---------------------------------------------------------------------------

/// Unsigned 64x64 -> 128, by 32-bit limbs.
[[nodiscard]] constexpr u128 umul_portable(std::uint64_t a, std::uint64_t b) noexcept
{
    constexpr std::uint64_t kMask32 = 0xFFFFFFFFULL;

    const std::uint64_t a_lo = a & kMask32;
    const std::uint64_t a_hi = a >> 32;
    const std::uint64_t b_lo = b & kMask32;
    const std::uint64_t b_hi = b >> 32;

    const std::uint64_t p_ll = a_lo * b_lo;
    const std::uint64_t p_lh = a_lo * b_hi;
    const std::uint64_t p_hl = a_hi * b_lo;
    const std::uint64_t p_hh = a_hi * b_hi;

    // Carry out of the low 32 bits, accumulated in 64 bits so it cannot itself
    // overflow: three values each below 2^32 sum to below 2^34.
    const std::uint64_t mid = (p_ll >> 32) + (p_lh & kMask32) + (p_hl & kMask32);

    u128 result;
    result.lo = (p_ll & kMask32) | (mid << 32);
    result.hi = p_hh + (p_lh >> 32) + (p_hl >> 32) + (mid >> 32);
    return result;
}

/// Signed 64x64 -> 128. Computes the unsigned product of the bit patterns and
/// corrects the high limb, which is exact in two's complement.
[[nodiscard]] constexpr u128 smul_portable(std::int64_t a, std::int64_t b) noexcept
{
    u128 result = umul_portable(static_cast<std::uint64_t>(a), static_cast<std::uint64_t>(b));
    if (a < 0) {
        result.hi -= static_cast<std::uint64_t>(b);
    }
    if (b < 0) {
        result.hi -= static_cast<std::uint64_t>(a);
    }
    return result;
}

[[nodiscard]] constexpr bool is_negative(u128 value) noexcept
{
    return (value.hi >> 63) != 0;
}

[[nodiscard]] constexpr u128 negate(u128 value) noexcept
{
    u128 result{~value.hi, ~value.lo};
    result.lo += 1;
    if (result.lo == 0) {
        result.hi += 1;
    }
    return result;
}

/// Unsigned 128/64 restoring division.
///
/// Precondition: `d != 0` and the quotient fits in 64 bits. Both hold for every
/// call fixed.hpp makes; a violation silently drops the overflowing high bits
/// rather than trapping, which is why the precondition is the caller's to keep.
[[nodiscard]] constexpr divmod64 udivmod_portable(u128 n, std::uint64_t d) noexcept
{
    if (d == 0) {
        return {};
    }

    std::uint64_t quotient = 0;
    std::uint64_t remainder = 0;

    for (int i = 127; i >= 0; --i) {
        const std::uint64_t bit =
            (i >= 64) ? ((n.hi >> (i - 64)) & 1ULL) : ((n.lo >> i) & 1ULL);

        // The shifted remainder is conceptually 65 bits. Capture the bit that
        // would be shifted out: if it is set, the true value exceeds any 64-bit
        // divisor and the subtraction is unconditional.
        const bool overflowed = (remainder >> 63) != 0;
        remainder = (remainder << 1) | bit;

        if (overflowed || remainder >= d) {
            remainder -= d;
            if (i < 64) {
                quotient |= (1ULL << i);
            }
        }
    }

    return {quotient, remainder};
}

// ---------------------------------------------------------------------------
// Compiler capability detection. Exactly one of these macros is defined on a
// supported target; a compiler matching neither fails to build below, which is
// the intended outcome — a silently-degraded numeric path is worse than a
// broken build.
// ---------------------------------------------------------------------------

#if defined(__SIZEOF_INT128__)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
using native_i128 = __int128;
using native_u128 = unsigned __int128;
#pragma GCC diagnostic pop

#define PITCHSIM_HAS_NATIVE_INT128 1

// 128-bit *division* is a compiler-rt/libgcc call (__divti3), not an
// instruction. That runtime library is not linked by default when targeting the
// MSVC ABI, so on clang-cl a runtime 128-bit divide fails at link with an
// undefined symbol. Multiplication is unaffected: a 64x64->128 widening
// multiply compiles to a single instruction with no libcall.
//
// This is only visible when the divide is *not* constant-folded, which is why
// it went unnoticed until test_wide.cpp started calling it with runtime values.
#if !defined(_MSC_VER)
#define PITCHSIM_HAS_NATIVE_DIV128 1
#endif

#elif defined(_MSC_VER) && defined(_M_X64)

#define PITCHSIM_HAS_MSVC_INTRINSICS 1

#else
#error "pitchsim/wide.hpp: no 128-bit path for this compiler/architecture. Supported: GCC, Clang, MSVC on x64. Windows-on-ARM is unclaimed — see docs/backlog.md."
#endif

// ---------------------------------------------------------------------------
// Complete portable operations. Named entry points rather than inlined into the
// dispatchers so that test_wide.cpp can compare *these exact functions* against
// the native ones. A differential test that reimplements the combination logic
// would be testing its own copy.
// ---------------------------------------------------------------------------

/// (a * b) >> shift, flooring. Precondition: 0 < shift < 64, result fits int64.
[[nodiscard]] constexpr std::int64_t mul_shift_floor_portable(std::int64_t a, std::int64_t b,
                                                              int shift) noexcept
{
    const u128 product = smul_portable(a, b);
    // Low 64 bits of the 128-bit arithmetic right shift. Correct for both signs
    // because the value is two's complement.
    return static_cast<std::int64_t>((product.lo >> shift) | (product.hi << (64 - shift)));
}

/// (a << shift) / b, flooring. Precondition: 0 < shift < 64, b != 0, fits int64.
[[nodiscard]] constexpr std::int64_t shift_div_floor_portable(std::int64_t a, std::int64_t b,
                                                              int shift) noexcept
{
    if (b == 0) {
        return 0;
    }

    u128 numerator;
    numerator.lo = static_cast<std::uint64_t>(a) << shift;
    numerator.hi = static_cast<std::uint64_t>(a >> (64 - shift));

    const bool numerator_negative = is_negative(numerator);
    const bool divisor_negative = b < 0;

    const u128 abs_numerator = numerator_negative ? negate(numerator) : numerator;
    const std::uint64_t abs_divisor =
        divisor_negative ? (~static_cast<std::uint64_t>(b) + 1ULL) : static_cast<std::uint64_t>(b);

    const divmod64 result = udivmod_portable(abs_numerator, abs_divisor);

    if (numerator_negative == divisor_negative) {
        return static_cast<std::int64_t>(result.quotient);
    }

    // Negative quotient: negate, then floor if the division was inexact.
    std::int64_t quotient = -static_cast<std::int64_t>(result.quotient);
    if (result.remainder != 0) {
        quotient -= 1;
    }
    return quotient;
}

// ---------------------------------------------------------------------------
// Native runtime operations.
// ---------------------------------------------------------------------------

#if defined(PITCHSIM_HAS_NATIVE_INT128)

[[nodiscard]] constexpr std::int64_t mul_shift_floor_native(std::int64_t a, std::int64_t b,
                                                            int shift) noexcept
{
    const native_i128 product = static_cast<native_i128>(a) * static_cast<native_i128>(b);
    return static_cast<std::int64_t>(product >> shift);
}

#if defined(PITCHSIM_HAS_NATIVE_DIV128)
/// Native 128-bit division. Used as the differential test's oracle for the
/// portable divide; the dispatcher below never calls it — see shift_div_floor.
[[nodiscard]] constexpr std::int64_t shift_div_floor_native(std::int64_t a, std::int64_t b,
                                                            int shift) noexcept
{
    if (b == 0) {
        return 0;
    }
    const native_i128 numerator = static_cast<native_i128>(a) << shift;
    const native_i128 divisor = static_cast<native_i128>(b);
    native_i128 quotient = numerator / divisor;
    const native_i128 remainder = numerator % divisor;
    // Integer division truncates toward zero; nudge down when the true result
    // was negative and inexact.
    if (remainder != 0 && ((remainder < 0) != (divisor < 0))) {
        quotient -= 1;
    }
    return static_cast<std::int64_t>(quotient);
}
#endif

#elif defined(PITCHSIM_HAS_MSVC_INTRINSICS)

[[nodiscard]] inline std::int64_t mul_shift_floor_native(std::int64_t a, std::int64_t b,
                                                         int shift) noexcept
{
    std::int64_t hi = 0;
    const std::int64_t lo = _mul128(a, b, &hi);
    return static_cast<std::int64_t>((static_cast<std::uint64_t>(lo) >> shift) |
                                     (static_cast<std::uint64_t>(hi) << (64 - shift)));
}

#endif

// ---------------------------------------------------------------------------
// Dispatch.
//
// Multiply: portable at compile time, native at runtime. The native path is a
// single widening-multiply instruction and this is the hottest operation in the
// engine, so the §7 budget justifies the second path.
//
// Divide: portable everywhere, at compile time AND at runtime, on every
// platform. Three reasons, in order of weight:
//
//   1. Native 128-bit division is a runtime-library call (__divti3), not an
//      instruction, and that library is not linked when targeting the MSVC ABI.
//   2. MSVC's _div128 raises SIGFPE on quotient overflow rather than returning,
//      turning a precondition violation into a process kill instead of a
//      testable wrong answer.
//   3. Division is far colder than multiplication in the tick loop, so the
//      portable path costs little and removes an entire axis of platform
//      variance — which is worth more to §6 than the cycles are worth to §7.
//
// shift_div_floor_native still exists on platforms that can link it, purely as
// the differential test's oracle. Nothing in the engine calls it.
// ---------------------------------------------------------------------------

[[nodiscard]] constexpr std::int64_t mul_shift_floor(std::int64_t a, std::int64_t b, int shift) noexcept
{
    if (!std::is_constant_evaluated()) {
        return mul_shift_floor_native(a, b, shift);
    }
    return mul_shift_floor_portable(a, b, shift);
}

[[nodiscard]] constexpr std::int64_t shift_div_floor(std::int64_t a, std::int64_t b, int shift) noexcept
{
    return shift_div_floor_portable(a, b, shift);
}

} // namespace pitchsim::wide
