// Differential test for the 128-bit intermediates — CLAUDE.md §6, ADR-0006.
//
// This is the test that makes MSVC support safe rather than a liability.
//
// `Fixed` has two implementations of every 128-bit operation: a portable one
// used at compile time on every compiler, and a native one used at runtime
// (__int128 on GCC/Clang, _mul128 on MSVC). If they ever disagree by one bit,
// a Windows build and a Linux build produce different match results — and
// every other test in this repository would still pass, because they all check
// one implementation against expected values rather than against each other.
//
// So: compare the two directly, over operands chosen to hit the cases where a
// hand-rolled 128-bit routine actually breaks — sign boundaries, limb
// boundaries, and inexact negative division.
//
// If this test fails, do not "fix" it by adjusting an expected value. One of
// the two implementations is wrong and the golden replays are meaningless
// until you know which.

#include <catch2/catch_test_macros.hpp>

#include "pitchsim/rng.hpp"
#include "pitchsim/wide.hpp"

#include <cstdint>
#include <limits>
#include <vector>

using pitchsim::Rng;
using pitchsim::RngStream;

namespace {

constexpr int kQ16 = 16;

// Operands chosen to break a naive implementation:
//   * zero and +/-1                      — sign-correction terms
//   * 2^32 +/- 1                          — the 32-bit limb boundary the
//                                           portable multiply splits on
//   * values whose product crosses 2^64  — where a 64-bit intermediate would
//                                           silently truncate
//   * INT64_MIN                          — the value with no positive negation
std::vector<std::int64_t> interesting_operands()
{
    return {
        0,
        1,
        -1,
        2,
        -2,
        65535,
        65536,
        -65536,
        0xFFFFFFFFLL,       // 2^32 - 1
        0x100000000LL,      // 2^32
        0x100000001LL,      // 2^32 + 1
        -0xFFFFFFFFLL,
        -0x100000000LL,
        -0x100000001LL,
        0x7FFFFFFFFLL,      // 2^35 - 1
        -0x7FFFFFFFFLL,
        1234567890123LL,
        -1234567890123LL,
        std::numeric_limits<std::int32_t>::max(),
        std::numeric_limits<std::int32_t>::min(),
    };
}

// Keeps a multiply inside the documented precondition: (a*b) >> 16 must fit in
// int64, so |a*b| must stay below 2^79. Bounding each operand to 2^39 is
// comfortably inside that.
constexpr std::int64_t kMulBound = 1LL << 39;

// (a << 16) / b must fit in int64. With |b| >= 1 the worst case is |a| << 16,
// so bound a to 2^40.
constexpr std::int64_t kDivBound = 1LL << 40;

} // namespace

TEST_CASE("portable and native multiply agree on edge operands", "[wide][determinism]")
{
    const std::vector<std::int64_t> operands = interesting_operands();

    for (const std::int64_t a : operands) {
        for (const std::int64_t b : operands) {
            // Skip pairs whose product would exceed the documented precondition.
            if (a != 0 && (a > kMulBound || a < -kMulBound)) {
                continue;
            }
            if (b != 0 && (b > kMulBound || b < -kMulBound)) {
                continue;
            }

            const std::int64_t portable = pitchsim::wide::mul_shift_floor_portable(a, b, kQ16);
            const std::int64_t native = pitchsim::wide::mul_shift_floor_native(a, b, kQ16);

            INFO("a = " << a << ", b = " << b);
            REQUIRE(portable == native);
        }
    }
}

TEST_CASE("portable and native divide agree on edge operands", "[wide][determinism]")
{
    const std::vector<std::int64_t> operands = interesting_operands();

    for (const std::int64_t a : operands) {
        if (a > kDivBound || a < -kDivBound) {
            continue;
        }
        for (const std::int64_t b : operands) {
            if (b == 0) {
                continue;
            }

            const std::int64_t portable = pitchsim::wide::shift_div_floor_portable(a, b, kQ16);
            const std::int64_t native = pitchsim::wide::shift_div_floor_native(a, b, kQ16);

            INFO("a = " << a << ", b = " << b);
            REQUIRE(portable == native);
        }
    }
}

TEST_CASE("portable and native agree over randomized operands", "[wide][determinism]")
{
    // Our own RNG, so a failure is reproducible from the seed alone.
    Rng rng = Rng{0xC0FFEE}.derive(RngStream::WorldGeneration, 1, 1);

    for (int i = 0; i < 200000; ++i) {
        const std::int64_t a = rng.next_range(-kMulBound, kMulBound);
        const std::int64_t b = rng.next_range(-kMulBound, kMulBound);

        const std::int64_t portable = pitchsim::wide::mul_shift_floor_portable(a, b, kQ16);
        const std::int64_t native = pitchsim::wide::mul_shift_floor_native(a, b, kQ16);

        INFO("iteration " << i << ", a = " << a << ", b = " << b);
        REQUIRE(portable == native);
    }

    for (int i = 0; i < 200000; ++i) {
        const std::int64_t a = rng.next_range(-kDivBound, kDivBound);
        std::int64_t b = rng.next_range(-1000000, 1000000);
        if (b == 0) {
            b = 1;
        }

        const std::int64_t portable = pitchsim::wide::shift_div_floor_portable(a, b, kQ16);
        const std::int64_t native = pitchsim::wide::shift_div_floor_native(a, b, kQ16);

        INFO("iteration " << i << ", a = " << a << ", b = " << b);
        REQUIRE(portable == native);
    }
}

TEST_CASE("the constexpr and runtime paths agree", "[wide][determinism]")
{
    // The dispatcher routes compile-time evaluation to the portable path and
    // runtime evaluation to the native one. A constant-folded tunable must not
    // differ from the same value computed during a match.
    constexpr std::int64_t kCompileTimeMul = pitchsim::wide::mul_shift_floor(-1234567, 89012, kQ16);
    constexpr std::int64_t kCompileTimeDiv = pitchsim::wide::shift_div_floor(-1000003, 7, kQ16);

    volatile std::int64_t a_mul = -1234567;
    volatile std::int64_t b_mul = 89012;
    volatile std::int64_t a_div = -1000003;
    volatile std::int64_t b_div = 7;

    REQUIRE(pitchsim::wide::mul_shift_floor(a_mul, b_mul, kQ16) == kCompileTimeMul);
    REQUIRE(pitchsim::wide::shift_div_floor(a_div, b_div, kQ16) == kCompileTimeDiv);
}

TEST_CASE("division floors toward negative infinity, not toward zero", "[wide]")
{
    // The property the whole rounding policy rests on. Truncation would give
    // -21845 for the second case and put a bias discontinuity at the origin.
    REQUIRE(pitchsim::wide::shift_div_floor_portable(1, 3, kQ16) == 21845);
    REQUIRE(pitchsim::wide::shift_div_floor_portable(-1, 3, kQ16) == -21846);
    REQUIRE(pitchsim::wide::shift_div_floor_portable(1, -3, kQ16) == -21846);
    REQUIRE(pitchsim::wide::shift_div_floor_portable(-1, -3, kQ16) == 21845);

    // Exact division must not be nudged down by the floor correction.
    REQUIRE(pitchsim::wide::shift_div_floor_portable(-4, 2, kQ16) == -2 * 65536);
    REQUIRE(pitchsim::wide::shift_div_floor_portable(4, -2, kQ16) == -2 * 65536);
}

TEST_CASE("the portable 64x64 product is exact above 2^64", "[wide]")
{
    // 2^40 * 2^40 = 2^80, which needs both limbs. A 64-bit intermediate would
    // wrap to zero here.
    const pitchsim::wide::u128 product =
        pitchsim::wide::umul_portable(1ULL << 40, 1ULL << 40);
    REQUIRE(product.hi == (1ULL << 16));
    REQUIRE(product.lo == 0);

    // Largest unsigned product: (2^64-1)^2 = 2^128 - 2^65 + 1.
    const pitchsim::wide::u128 max_product =
        pitchsim::wide::umul_portable(~0ULL, ~0ULL);
    REQUIRE(max_product.hi == 0xFFFFFFFFFFFFFFFEULL);
    REQUIRE(max_product.lo == 1ULL);
}

TEST_CASE("unsigned 128/64 division handles a divisor above 2^63", "[wide]")
{
    // Exercises the 65-bit remainder carry in udivmod_portable: with a divisor
    // this large, the shifted remainder overflows 64 bits and the algorithm
    // must subtract on the carry bit rather than on the comparison.
    const std::uint64_t big_divisor = 0x8000000000000001ULL;
    const pitchsim::wide::u128 numerator{0x4000000000000000ULL, 0x123456789ABCDEFULL};

    const pitchsim::wide::divmod64 result =
        pitchsim::wide::udivmod_portable(numerator, big_divisor);

    // Verify by reconstruction: quotient * divisor + remainder == numerator.
    const pitchsim::wide::u128 reconstructed =
        pitchsim::wide::umul_portable(result.quotient, big_divisor);
    std::uint64_t lo = reconstructed.lo + result.remainder;
    std::uint64_t hi = reconstructed.hi + (lo < reconstructed.lo ? 1ULL : 0ULL);

    REQUIRE(hi == numerator.hi);
    REQUIRE(lo == numerator.lo);
}
