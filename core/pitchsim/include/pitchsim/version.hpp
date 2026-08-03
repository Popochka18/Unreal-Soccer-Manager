#pragma once

#include <cstdint>

namespace pitchsim {

// Versioned contracts — CLAUDE.md §0.3. These are build metadata, not gameplay
// tunables, so they live in source rather than the DB.
//
// kRulesetVersion is part of the match input tuple (§6): a change to it is a
// declaration that the same seed is allowed to produce a different result, and
// must land with regenerated goldens in a `golden: <reason>` commit.
inline constexpr std::uint32_t kRulesetVersion = 0;

// Wire format of the append-only match event stream (§7). Consumers negotiate
// on this; a bump requires a compatibility note in /docs/contracts/events.md.
inline constexpr std::uint32_t kEventStreamVersion = 0;

// Returns the compiled-in library version string. Exists so that libpitchsim
// has at least one non-header translation unit: a header-only library would
// silently not be built, and the determinism compile flags in
// cmake/Determinism.cmake would never actually be applied to anything.
[[nodiscard]] const char* version_string() noexcept;

} // namespace pitchsim
