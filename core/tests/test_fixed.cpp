#include <catch2/catch_test_macros.hpp>

#include "pitchsim/fixed.hpp"

#include <cstdint>

using pitchsim::Fixed;

TEST_CASE("integers round-trip exactly", "[fixed]")
{
    REQUIRE(Fixed::from_int(0).raw() == 0);
    REQUIRE(Fixed::from_int(1).raw() == 65536);
    REQUIRE(Fixed::from_int(-1).raw() == -65536);
    REQUIRE(Fixed::from_int(105).to_int_floor() == 105);
    REQUIRE(Fixed::from_int(-105).to_int_floor() == -105);
}

TEST_CASE("from_ratio floors, including below zero", "[fixed]")
{
    // Exactly representable.
    REQUIRE(Fixed::from_ratio(1, 2).raw() == 32768);
    REQUIRE(Fixed::from_ratio(-1, 2).raw() == -32768);
    REQUIRE(Fixed::from_ratio(3, 4).raw() == 49152);

    // Not representable: must round toward negative infinity, not toward zero.
    // Truncation would give -21845 here and introduce a bias discontinuity at 0.
    REQUIRE(Fixed::from_ratio(1, 3).raw() == 21845);
    REQUIRE(Fixed::from_ratio(-1, 3).raw() == -21846);
}

TEST_CASE("to_int_floor floors below zero", "[fixed]")
{
    REQUIRE(Fixed::from_ratio(1, 2).to_int_floor() == 0);
    REQUIRE(Fixed::from_ratio(-1, 2).to_int_floor() == -1);
    REQUIRE(Fixed::from_ratio(-3, 2).to_int_floor() == -2);
}

TEST_CASE("addition and subtraction are exact", "[fixed]")
{
    const Fixed a = Fixed::from_ratio(1, 4);
    const Fixed b = Fixed::from_ratio(3, 4);
    REQUIRE((a + b) == pitchsim::kOne);
    REQUIRE((a - b).raw() == -32768);
    REQUIRE((-a).raw() == -16384);
}

TEST_CASE("multiplication uses an exact 128-bit intermediate", "[fixed]")
{
    REQUIRE((Fixed::from_ratio(1, 2) * Fixed::from_ratio(1, 2)).raw() == 16384); // 0.25
    REQUIRE((Fixed::from_int(7) * pitchsim::kOne) == Fixed::from_int(7));

    // Naive int64 maths overflows here: the raw operands are 6.55e13 and
    // 6.55e5, whose product is 4.29e19 against an int64 ceiling of 9.22e18.
    // A correct 128-bit intermediate lands on exactly 1e10.
    const Fixed big = Fixed::from_int(1'000'000'000) * Fixed::from_int(10);
    REQUIRE(big == Fixed::from_int(10'000'000'000));
}

TEST_CASE("division floors and is exact where it can be", "[fixed]")
{
    REQUIRE((pitchsim::kOne / Fixed::from_int(2)).raw() == 32768);
    REQUIRE((pitchsim::kOne / Fixed::from_int(3)).raw() == 21845);
    REQUIRE(((-pitchsim::kOne) / Fixed::from_int(3)).raw() == -21846);
    REQUIRE((Fixed::from_int(100) / Fixed::from_int(4)) == Fixed::from_int(25));
}

TEST_CASE("integer scaling matches repeated addition", "[fixed]")
{
    const Fixed third = Fixed::from_ratio(1, 3);
    Fixed summed = pitchsim::kZero;
    for (int i = 0; i < 9; ++i) {
        summed += third;
    }
    REQUIRE(summed == (third * std::int64_t{9}));
}

TEST_CASE("ordering is total and matches raw order", "[fixed]")
{
    REQUIRE(Fixed::from_int(-2) < Fixed::from_int(-1));
    REQUIRE(Fixed::from_ratio(-1, 3) < pitchsim::kZero);
    REQUIRE(Fixed::from_ratio(1, 3) > pitchsim::kZero);
    REQUIRE(Fixed::from_int(5) == Fixed::from_raw(5 * 65536));
}
