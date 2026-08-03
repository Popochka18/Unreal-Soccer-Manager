# libpitchsim

The deterministic match engine. CLAUDE.md §7 is the contract; this file is the map.

## What this library is

Pure C++20. STL only. No Unreal, no SQLite, no I/O, no threads, no globals. The headless
server and the UE5 plugin link this *same* static library — that is the entire point of §2.
The football is here; the pixels are in Unreal.

## Current state (M0)

Skeleton. Two primitives, both fully tested, nothing else:

| Header | What it is |
|---|---|
| `include/pitchsim/fixed.hpp` | `Fixed` — q16.16 in `int64`. Every number that can affect an outcome. |
| `include/pitchsim/wide.hpp` | 128-bit intermediates. Portable at compile time, native at runtime. |
| `include/pitchsim/rng.hpp` | `Rng` — SplitMix64, explicit state, per-purpose derived streams. |
| `include/pitchsim/version.hpp` | Ruleset and event-stream contract versions. |

The 22-agent tick loop, the utility decision layer, the ball physics and the event stream
land at M2. Do not add them here without reading §7 first.

## The two rules that break everything if you get them wrong

**1. No `float`, no `double`.** Not in state, not in an intermediate, not "just for this
one ratio". Use `Fixed`. Every lossy operation on it floors, including below zero — see the
comment block at the top of `fixed.hpp` for why truncation-toward-zero was rejected.

**2. Never draw from a shared RNG sequence.** Derive a stream:

```cpp
Rng physics = root.derive(RngStream::MatchPhysics, match_id, tick);
```

`derive()` is a pure function of the root state, so adding a call site to one stream cannot
shift any other. Deriving from an *already-advanced* generator throws that away and
reintroduces call-order coupling — the test `derive is independent of root consumption`
exists to catch exactly that regression.

## Building and testing

```bash
cmake --workflow --preset ci
```

Sanitizers: `cmake --workflow --preset asan` and `--preset ubsan`. Both must be green
before anything merges (§11).

## Compile flags are not decoration

`cmake/Determinism.cmake` pins `-ffp-contract=off`, disables fast-math and auto-
vectorisation, and turns the warning bar up to `-Werror`. These are applied per-target via
`pitchforge_sim_target()`. If you find yourself wanting to relax one to make a build pass,
that is the bug, not the flag.
