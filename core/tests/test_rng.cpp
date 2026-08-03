#include <catch2/catch_test_macros.hpp>

#include "pitchsim/rng.hpp"

#include <array>
#include <cstdint>

using pitchsim::Rng;
using pitchsim::RngStream;

TEST_CASE("SplitMix64 matches the reference vectors", "[rng][determinism]")
{
    // Reference output of splitmix64 seeded with 0. If this test ever fails,
    // every golden replay in the repository is invalid — do not "fix" it by
    // updating the constants.
    Rng rng{0};
    REQUIRE(rng.next_u64() == 0xE220A8397B1DCDAFULL);
    REQUIRE(rng.next_u64() == 0x6E789E6AA1B965F4ULL);
    REQUIRE(rng.next_u64() == 0x06C45D188009454FULL);
}

TEST_CASE("a saved state resumes an identical future", "[rng][determinism]")
{
    Rng original{0xDEADBEEF};
    for (int i = 0; i < 32; ++i) {
        (void)original.next_u64();
    }

    // This is what a save file stores (§6).
    Rng restored = Rng::from_state(original.state());

    for (int i = 0; i < 64; ++i) {
        REQUIRE(restored.next_u64() == original.next_u64());
    }
}

TEST_CASE("derive is pure", "[rng][determinism]")
{
    const Rng root{12345};
    REQUIRE(root.derive(RngStream::MatchPhysics, 7, 9).state()
            == root.derive(RngStream::MatchPhysics, 7, 9).state());
}

TEST_CASE("derive is independent of root consumption", "[rng][determinism]")
{
    // The §6 invariant: adding a call site to one stream must not shift any
    // other stream. Draw heavily from MatchPhysics, then check MatchDecision
    // produced exactly what it would have produced had MatchPhysics never run.
    Rng control{42};
    const std::uint64_t expected =
        control.derive(RngStream::MatchDecision, 7, 9).next_u64();

    Rng root{42};
    Rng physics = root.derive(RngStream::MatchPhysics, 1, 1);
    for (int i = 0; i < 1000; ++i) {
        (void)physics.next_u64();
    }

    REQUIRE(root.derive(RngStream::MatchDecision, 7, 9).next_u64() == expected);
}

TEST_CASE("distinct streams and distinct keys do not collide", "[rng][determinism]")
{
    const Rng root{999};
    const std::uint64_t physics = root.derive(RngStream::MatchPhysics, 1, 1).state();
    const std::uint64_t decision = root.derive(RngStream::MatchDecision, 1, 1).state();
    const std::uint64_t swapped = root.derive(RngStream::MatchPhysics, 1, 2).state();
    const std::uint64_t swapped2 = root.derive(RngStream::MatchPhysics, 2, 1).state();

    REQUIRE(physics != decision);
    REQUIRE(physics != swapped);
    // XOR-folding the key would make these two identical. Sequential mixing
    // must not be permutation-symmetric: (match, tick) is exactly this shape.
    REQUIRE(swapped != swapped2);
}

TEST_CASE("next_below stays in range and reaches every bucket", "[rng]")
{
    Rng rng{7};
    constexpr std::uint64_t kBound = 6;
    std::array<int, kBound> hits{};

    for (int i = 0; i < 20000; ++i) {
        const std::uint64_t v = rng.next_below(kBound);
        REQUIRE(v < kBound);
        hits[static_cast<std::size_t>(v)] += 1;
    }

    for (const int count : hits) {
        // 20000/6 is ~3333; a bucket under 3000 would mean a biased sampler.
        REQUIRE(count > 3000);
    }
}

TEST_CASE("next_below handles degenerate bounds", "[rng]")
{
    Rng rng{7};
    REQUIRE(rng.next_below(0) == 0);
    REQUIRE(rng.next_below(1) == 0);
}

TEST_CASE("next_range is inclusive at both ends", "[rng]")
{
    Rng rng{2024};
    bool saw_lo = false;
    bool saw_hi = false;

    for (int i = 0; i < 5000; ++i) {
        const std::int64_t v = rng.next_range(-3, 3);
        REQUIRE(v >= -3);
        REQUIRE(v <= 3);
        saw_lo = saw_lo || (v == -3);
        saw_hi = saw_hi || (v == 3);
    }

    REQUIRE(saw_lo);
    REQUIRE(saw_hi);
}

TEST_CASE("next_range handles a single-value span", "[rng]")
{
    Rng rng{1};
    REQUIRE(rng.next_range(5, 5) == 5);
}
