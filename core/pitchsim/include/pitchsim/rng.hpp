#pragma once

// Deterministic RNG — CLAUDE.md §6.
//
// One generator (SplitMix64), explicit state, always passed in, never global.
// The property that matters more than speed or statistical quality: adding a
// new call site must not shift any other stream. That is achieved by deriving
// a fresh, independent generator per purpose from a root seed, rather than
// drawing every value from one shared sequence.
//
//     Rng root{save.seed};
//     Rng physics = root.derive(RngStream::MatchPhysics, match_id, tick);
//
// TRAP: derive() is a pure function of the *current* state. Deriving from a
// generator that has already been advanced reintroduces exactly the call-order
// coupling this design exists to remove. Always derive from an unadvanced root.
// Enforced by test "derive is independent of root consumption".

#include <cstdint>
#include <limits>

namespace pitchsim {

// One value per independent source of randomness in the game. Append only —
// inserting in the middle renumbers every stream after it and invalidates
// every golden replay and every save.
enum class RngStream : std::uint32_t {
    MatchPhysics = 0,
    MatchDecision = 1,
    MatchInjury = 2,
    MatchReferee = 3,
    TransferMarket = 4,
    PlayerDevelopment = 5,
    YouthIntake = 6,
    MediaNarrative = 7,
    WorldGeneration = 8,
};

class Rng {
public:
    explicit constexpr Rng(std::uint64_t seed) noexcept : state_(seed) {}

    [[nodiscard]] constexpr std::uint64_t state() const noexcept { return state_; }

    // Saves persist this so that load -> continue reproduces the same future.
    static constexpr Rng from_state(std::uint64_t s) noexcept { return Rng{s}; }

    [[nodiscard]] constexpr std::uint64_t next_u64() noexcept
    {
        state_ += kGolden;
        return finalize(state_);
    }

    [[nodiscard]] constexpr std::uint32_t next_u32() noexcept
    {
        return static_cast<std::uint32_t>(next_u64() >> 32);
    }

    // Uniform on [0, bound). Rejection-sampled, so the result is unbiased and
    // the rejection decision is integer-exact on every platform. bound == 0 is
    // a caller error and returns 0.
    [[nodiscard]] constexpr std::uint64_t next_below(std::uint64_t bound) noexcept
    {
        if (bound == 0) {
            return 0;
        }
        // Values below this threshold would make the modulo non-uniform.
        const std::uint64_t threshold =
            (std::numeric_limits<std::uint64_t>::max() - bound + 1) % bound;
        for (;;) {
            const std::uint64_t r = next_u64();
            if (r >= threshold) {
                return r % bound;
            }
        }
    }

    // Uniform on [lo, hi], inclusive. Precondition: lo <= hi.
    [[nodiscard]] constexpr std::int64_t next_range(std::int64_t lo, std::int64_t hi) noexcept
    {
        const std::uint64_t span = static_cast<std::uint64_t>(hi - lo) + 1;
        return lo + static_cast<std::int64_t>(next_below(span));
    }

    // A fresh generator for one purpose. Pure: same (root state, stream, a, b)
    // always yields the same generator, regardless of what else the caller did.
    [[nodiscard]] constexpr Rng derive(RngStream stream, std::uint64_t a, std::uint64_t b) const noexcept
    {
        // Sequential mixing rather than XOR-folding: XOR collides on
        // permutations of the same inputs, and (stream, match, tick) triples
        // are exactly the shape that would collide.
        std::uint64_t h = state_;
        h = finalize(h + (static_cast<std::uint64_t>(stream) + 1) * kGolden);
        h = finalize(h + a);
        h = finalize(h + b);
        return Rng{h};
    }

private:
    static constexpr std::uint64_t kGolden = 0x9E3779B97F4A7C15ULL;

    // SplitMix64 finalizer.
    static constexpr std::uint64_t finalize(std::uint64_t z) noexcept
    {
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }

    std::uint64_t state_;
};

} // namespace pitchsim
