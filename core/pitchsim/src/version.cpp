#include "pitchsim/version.hpp"

#include "pitchsim/fixed.hpp"
#include "pitchsim/rng.hpp"

namespace pitchsim {

const char* version_string() noexcept
{
    return "libpitchsim 0.0.0 (M0 skeleton)";
}

// Compile-time guards. These are cheap and they fail the build rather than a
// test, which is the right place to catch a platform that would silently break
// determinism (§6).
static_assert(sizeof(Fixed) == sizeof(std::int64_t),
              "Fixed must be exactly its raw int64 — no padding, no vtable.");
static_assert(Fixed::kScale == 65536, "q16.16 (§3).");

// C++20 mandates arithmetic (flooring) right shift on negative signed values.
// If a future toolchain regresses this, every position in the sim shifts.
static_assert((std::int64_t{-1} >> 1) == std::int64_t{-1}, "Right shift must floor.");

static_assert(Fixed::from_ratio(1, 2).raw() == 32768, "0.5 is exactly representable.");
static_assert(Fixed::from_ratio(-1, 2).raw() == -32768, "-0.5 is exactly representable.");
// 1/3 is not representable; it must floor, not truncate toward zero.
static_assert(Fixed::from_ratio(1, 3).raw() == 21845, "floor(65536/3)");
static_assert(Fixed::from_ratio(-1, 3).raw() == -21846, "floor(-65536/3)");

static_assert(Rng{0}.derive(RngStream::MatchPhysics, 1, 1).state()
                  != Rng{0}.derive(RngStream::MatchDecision, 1, 1).state(),
              "Streams must not collide.");

} // namespace pitchsim
