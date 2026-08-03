# PITCHFORGE

A football management simulation. Single-player, offline-first, fictional world, moddable to
the bone.

**[CLAUDE.md](CLAUDE.md) is the highest authority in this repository.** Read it before
changing anything. [`/docs/status.md`](docs/status.md) says what actually works today;
current milestone is **M0**.

## Build

Native core — needs CMake ≥3.27, Ninja, and GCC, Clang or MSVC on x64
([ADR-0006](docs/adr/0006-support-msvc-with-a-dual-128-bit-path.md)):

```bash
cmake --workflow --preset ci
```

UI shell — needs Node ≥22 and pnpm 11:

```bash
cd app && pnpm install && pnpm -r typecheck && pnpm lint && pnpm -r test && pnpm -r build
```

Sanitizers, both of which must be green before anything merges:

```bash
cmake --workflow --preset asan
```

```bash
cmake --workflow --preset ubsan
```

Determinism guard:

```bash
./tools/determinism-guard.sh
```

## Shape of the thing

Three processes, and the reason for the split is
[ADR-0001](docs/adr/0001-architecture.md):

- **`/app`** — Tauri v2 + React UI. Zero game logic. It formats numbers; it never computes
  them.
- **`/core/server`** — headless native binary. Owns the world, the calendar and the save.
- **`/ue5`** — match renderer, separate process, launched on demand. Deferred to M7.

The match engine is **`/core/pitchsim`** — a standalone C++20 library with no Unreal
dependency, linked by both the server and the renderer. The football is in the library; the
pixels are in Unreal. That is what makes 40 000 matches per game week possible, and it is the
decision everything else hangs from.

## Three rules worth knowing before you read any code

1. **No `float` or `double` anywhere an outcome depends on it.** Fixed-point q16.16 in
   `int64` — see `core/pitchsim/include/pitchsim/fixed.hpp`.
2. **No gameplay constant in source.** Club names, attribute weights, competition rules and
   formula coefficients live in the database. A number typed into a `.cpp` is a revert.
3. **Same input, byte-identical output — on every OS, compiler and CPU, forever.** Golden
   replays enforce it from M2.

## Layout

`/core` native · `/app` UI · `/ue5` renderer · `/data` packs · `/tools` pipeline ·
`/tests` golden and calibration data · `/docs` ADRs and contracts

## Commands

Project workflows live in `.claude/commands/`: `/bootstrap` `/adr` `/schema` `/seed` `/sim`
`/match` `/determinism` `/ue` `/api` `/ui` `/assets` `/bench` `/review` `/fix` `/mod`
`/ship` `/soak`

## Content

Fictional world only. No real player names, club names, crests, kits or competition names,
and no scraped datasets. Every asset row carries a `license` field and CI fails on `unknown`.
