# ADR-0001: Three-process architecture with a standalone match engine

- **Status:** Accepted
- **Date:** 2026-08-03
- **Amends:** none (records CLAUDE.md §2 as a decision rather than an assertion)

## Context

PITCHFORGE targets 60+ leagues, ~500k persons, and 20+ seasons of continuous play. A game
week simulates on the order of 40 000 matches and must complete in ≤3 s on 8 cores (§7).
Separately, the product promises three synchronised views of a match — text commentary, 2D
top-down and 3D — and a UI dense enough to scroll 5 000-row tables at 60 fps.

Those two requirements pull hard in opposite directions. The rendering requirement wants a
game engine. The throughput requirement wants no engine at all: 40 000 matches per game
week is ~75 µs of budget each, and an editor, an RHI and a game thread cannot be in that
path.

The question this ADR settles is *where the football lives*.

## Options considered

### 1. Simulation inside UE5 (`UObject`/`AActor` gameplay classes)

The conventional Unreal shape: the match is Actors and Components, the season is a
GameInstance subsystem.

**Cost:** fatal at M4, and not recoverable. Headless season simulation would require
running the engine 40 000 times per game week. `UObject` construction, the reflection
system, and tick management put a hard floor of milliseconds per match against a 75 µs
budget — three orders of magnitude out. Determinism is also unattainable: engine tick
order, `FMath` float paths and editor-vs-shipping differences are not byte-stable, so §6's
"same input ⇒ byte-identical event stream on every OS, compiler and CPU, forever" cannot be
met. This option would be discovered to be wrong at M4, by which point every gameplay
system would need rewriting.

### 2. Two implementations — a fast headless sim, plus a detailed UE5 sim for watched matches

Simulate quickly for the 40 000 background matches, and render a richer independent
simulation for the match the human watches.

**Cost:** two sets of rules that drift. Every future rule change lands twice or the two
disagree. Worse, it breaks the product promise directly: a watched match and a simulated
match would produce different results from the same fixture, so "continue" after watching
would not be reproducible and save/reload would change outcomes. It also makes the §6
golden-replay strategy meaningless, since there would be no single stream to hash.

### 3. Standalone `libpitchsim`, linked by both the server and UE5 *(chosen)*

The match engine is a pure C++20 static library with no Unreal, no SQLite, no I/O and no
threads. The headless server links it. The UE5 plugin links the same library. UE5 consumes
an event stream and renders it.

**Cost:** real, and worth naming. We give up every Unreal convenience inside gameplay code
— no `TArray`, no `FString`, no Blueprint iteration on rules, no engine profiler on the sim.
We take on our own fixed-point maths, our own RNG, and our own serialisation. Onboarding an
Unreal-native engineer is harder because the interesting code is the part that looks least
like Unreal. There is also a permanent boundary to police: every PR is an opportunity for
someone to include `CoreMinimal.h` "just here".

## Decision

Option 3. The match engine is `libpitchsim`, a standalone library. Three processes:

1. **UI shell** — Tauri v2 + React. Zero game logic. Talks loopback HTTP/WebSocket,
   MessagePack, via a generated typed client.
2. **Game server** — headless C++20 native binary. Owns the world, the calendar, the save,
   and the SQLite data layer. Links `libpitchsim`.
3. **UE5 renderer** — separate process, launched on demand, links the same `libpitchsim`.
   Consumes the event stream. Sends nothing back but user input.

The event stream is the single source of truth for commentary, 2D, 3D, stats, highlights,
ratings and analysis. Nothing recomputes anything from anything else.

## Consequences

**Easier:** headless soaks and CI. Golden replays are just hashes of a byte stream.
Parallelism across matches is trivial because a match owns no shared state. The renderer
can crash without touching a save. A future second renderer (2D, web, a replay tool) is
another consumer, not another integration.

**Harder:** everything ergonomic. We write fixed-point arithmetic by hand
(`core/pitchsim/include/pitchsim/fixed.hpp`) because floats are banned from anything that
affects an outcome. We write our own RNG with derived streams because a shared sequence
would couple unrelated systems. Debugging a visual glitch means asking whether the stream
is wrong or the renderer is — two codebases, one symptom.

**The boundary is the thing that will erode.** It erodes in small, reasonable-looking steps:
a `TArray` in a helper, a float for "just a ratio", one SQL query in a loop. The compile
flags in `cmake/Determinism.cmake`, the CI grep for banned constructs, and the `/determinism`
command exist because this decision is only as good as its enforcement.

## Rollback path

Poor. This is close to a one-way door and should be treated as one.

Reverting to option 1 means rewriting every gameplay system as Unreal classes and
abandoning the §7 performance targets — in practice, a different product. Reverting to
option 2 means accepting rule drift and losing golden replays.

The realistic partial rollback is narrower: if the UE5 renderer proves unable to link
`libpitchsim` cleanly, it can consume the event stream over IPC from the server instead of
linking the library directly. That costs latency and a serialisation hop but preserves
everything else, and is the escape hatch we would actually use.

## Verification

M0 demonstrates the shape rather than asserting it: `libpitchsim` builds as a static
library with no dependencies beyond the STL, under `-Werror` with determinism flags pinned,
green on release, ASan and UBSan.
