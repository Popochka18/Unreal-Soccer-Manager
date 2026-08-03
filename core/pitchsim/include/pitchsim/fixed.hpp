#pragma once

// q16.16 fixed-point arithmetic — CLAUDE.md §3.
//
// Every number that can affect a match or season outcome is this type. float
// and double are banned from simulation state; they are permitted only in
// rendering and in UI display formatting.
//
// Representation: value = raw / 2^16, raw is int64. That gives ~1.5e-5
// resolution over a range of roughly +/-1.4e14 — several orders of magnitude
// more headroom than metres-on-a-pitch or currency-in-pennies needs.
//
// Rounding: every lossy operation floors (rounds toward negative infinity),
// including at negative values. Truncation-toward-zero was rejected because it
// introduces a discontinuity at 0 that shows up as a bias in any quantity that
// straddles the origin — relative positions and velocity deltas both do. The
// two shift/divide primitives below are the only places rounding happens.
//
// Portability: C++20 defines `>>` on a negative signed value as a floor
// division by a power of two ([expr.shift]/3), so the arithmetic shift is
// standard-mandated, not implementation-defined. The 128-bit intermediates are
// exact on every supported target.

#include <cstdint>
#include <compare>

namespace pitchsim {

namespace detail {

#if defined(__SIZEOF_INT128__)

// __int128 is a compiler extension, so -Wpedantic objects to the token. The
// warning is correct and we want it everywhere else — suppress it for exactly
// this declaration rather than dropping -Wpedantic from the sim warning bar.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
using wide_t = __int128;
#pragma GCC diagnostic pop

// Floor division on the 128-bit intermediate. C++ integer division truncates
// toward zero; nudge the quotient down when the true result was negative and
// inexact.
constexpr wide_t floor_div_wide(wide_t n, wide_t d) noexcept
{
    const wide_t q = n / d;
    const wide_t r = n % d;
    return (r != 0 && ((r < 0) != (d < 0))) ? q - 1 : q;
}

// (a * b) >> shift, with an exact 128-bit product. Flooring.
constexpr std::int64_t mul_shift_floor(std::int64_t a, std::int64_t b, int shift) noexcept
{
    const wide_t product = static_cast<wide_t>(a) * static_cast<wide_t>(b);
    return static_cast<std::int64_t>(product >> shift);
}

// (a << shift) / b, with an exact 128-bit numerator. Flooring.
constexpr std::int64_t shift_div_floor(std::int64_t a, std::int64_t b, int shift) noexcept
{
    const wide_t numerator = static_cast<wide_t>(a) << shift;
    return static_cast<std::int64_t>(floor_div_wide(numerator, static_cast<wide_t>(b)));
}

#else
// MSVC has no __int128, and its _mul128/_div128 intrinsics are not constexpr,
// so supporting it means a second numeric code path plus a portable constexpr
// fallback — roughly 150 lines of 128-bit arithmetic that would have to be
// exactly bit-equivalent to this one, forever, or §6 breaks.
//
// We build Windows with clang instead. See ADR-0005.
#error "pitchsim requires a compiler with __int128 (GCC or Clang). Windows builds use clang-cl — see docs/adr/0005-one-compiler-family.md"
#endif

} // namespace detail

class Fixed {
public:
    using raw_t = std::int64_t;

    static constexpr int kFractionBits = 16;
    static constexpr raw_t kScale = raw_t{1} << kFractionBits;

    constexpr Fixed() noexcept = default;

    [[nodiscard]] static constexpr Fixed from_raw(raw_t raw) noexcept
    {
        Fixed f;
        f.raw_ = raw;
        return f;
    }

    [[nodiscard]] static constexpr Fixed from_int(std::int64_t whole) noexcept
    {
        return from_raw(whole * kScale);
    }

    // Exact ratio, floored. This is how a tunable loaded from the DB as a
    // numerator/denominator pair becomes a Fixed — never via a float literal.
    [[nodiscard]] static constexpr Fixed from_ratio(std::int64_t num, std::int64_t den) noexcept
    {
        return from_raw(detail::shift_div_floor(num, den, kFractionBits));
    }

    [[nodiscard]] constexpr raw_t raw() const noexcept { return raw_; }

    // Floor, so to_int() of -0.5 is -1. Use for indexing into a grid.
    [[nodiscard]] constexpr std::int64_t to_int_floor() const noexcept
    {
        return raw_ >> kFractionBits;
    }

    [[nodiscard]] constexpr Fixed operator+(Fixed o) const noexcept { return from_raw(raw_ + o.raw_); }
    [[nodiscard]] constexpr Fixed operator-(Fixed o) const noexcept { return from_raw(raw_ - o.raw_); }
    [[nodiscard]] constexpr Fixed operator-() const noexcept { return from_raw(-raw_); }

    [[nodiscard]] constexpr Fixed operator*(Fixed o) const noexcept
    {
        return from_raw(detail::mul_shift_floor(raw_, o.raw_, kFractionBits));
    }

    // Precondition: o != 0. A zero divisor is a logic error in the caller, not
    // a value to be papered over — there is no NaN to fall back to and §11
    // requires finance/ratings invariants to hold exactly.
    [[nodiscard]] constexpr Fixed operator/(Fixed o) const noexcept
    {
        return from_raw(detail::shift_div_floor(raw_, o.raw_, kFractionBits));
    }

    // Scaling by a plain integer is exact and much cheaper than widening.
    [[nodiscard]] constexpr Fixed operator*(std::int64_t k) const noexcept { return from_raw(raw_ * k); }

    constexpr Fixed& operator+=(Fixed o) noexcept { raw_ += o.raw_; return *this; }
    constexpr Fixed& operator-=(Fixed o) noexcept { raw_ -= o.raw_; return *this; }
    constexpr Fixed& operator*=(Fixed o) noexcept { *this = *this * o; return *this; }
    constexpr Fixed& operator/=(Fixed o) noexcept { *this = *this / o; return *this; }

    [[nodiscard]] constexpr auto operator<=>(const Fixed&) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const Fixed&) const noexcept = default;

private:
    raw_t raw_ = 0;
};

inline constexpr Fixed kZero = Fixed::from_raw(0);
inline constexpr Fixed kOne = Fixed::from_int(1);

} // namespace pitchsim
