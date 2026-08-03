# ADR-0004: Test code lives in /core/tests, test data lives in /tests

- **Status:** Accepted
- **Date:** 2026-08-03
- **Amends:** CLAUDE.md §4 (repository layout)

## Context

CLAUDE.md contradicts itself about where tests live.

§4 declares one location and instructs that no new top-level folders be invented:

> `/core/tests/` — Unit + golden-replay + property tests. Catch2.

Four other sections reference a *top-level* `/tests/` that §4 does not list:

- §3: "Fixtures live under `/tests/fixtures/`."
- §6: "`/tests/golden/` holds ~200 recorded matches."
- §7 and §11: "the bands in `/tests/calibration/bands.json5`."

Both readings cannot be followed. This had to be settled before creating directories,
because golden replay paths are referenced by tooling and moving them later invalidates
committed hashes' provenance.

## Options considered

### 1. Everything under `/core/tests/`

Honour §4 literally; move golden, calibration and fixture data into `/core/tests/`.

**Cost:** those three directories are consumed by tools outside `/core/` — `/tools/soak`,
`/tools/replaydiff`, the 10 000-match statistical harness, and eventually the pack
compiler's validation suite. Burying shared data inside the C++ unit-test directory makes
`/tools/` reach into `/core/tests/`, which inverts the dependency: tools would depend on the
core's test layout. It also puts ~200 golden match blobs inside the directory a C++ engineer
greps daily.

### 2. Everything under a top-level `/tests/`

Honour §3/§6/§7/§11; move the Catch2 sources out of `/core/`.

**Cost:** separates the tests from the code they test, which is the arrangement §0.1
("read its README, its ADRs, and its tests") is worst served by. It also means `/core/`'s
CMake reaches upward out of its own subtree.

### 3. Split by kind — code in `/core/tests/`, data in `/tests/` *(chosen)*

`/core/tests/` holds Catch2 translation units. Top-level `/tests/` holds the data that both
the core tests and the `/tools/` binaries consume.

**Cost:** two places called "tests", which someone will find confusing at least once. That
is why this ADR exists and why `core/tests/README.md` states the split explicitly.

## Decision

| Path | Contents | Consumed by |
|---|---|---|
| `/core/tests/` | Catch2 unit and property tests (`.cpp`) | `pitchsim_tests` |
| `/tests/golden/` | ~200 recorded matches: input blob + BLAKE3 of the event stream | core tests, `/tools/replaydiff`, CI |
| `/tests/calibration/` | `bands.json5` and statistical expectations | core tests, `/tools/soak`, the 10k-match harness |
| `/tests/fixtures/` | shared test data — the only legitimate home for hardcoded test content (§3) | everything |

The rule of thumb: **if a `/tools/` binary reads it, it belongs in `/tests/`.**

`/tests/**` is marked `-text` in `.gitattributes`. Golden inputs and hashes are byte-exact;
a CRLF normalisation on a Windows checkout would silently invalidate every one of them.

## Consequences

**Easier:** `/tools/` and `/core/` share fixture data without either depending on the
other's internal layout. Golden replays sit at a stable, tool-facing path from M0, so the
hashes committed at M2 will not need relocating.

**Harder:** two directories share a name. New contributors will put a `.cpp` in `/tests/`
or a `.json5` in `/core/tests/` at least once.

**Amendment required.** §4's layout table should gain a `/tests/` row. Until CLAUDE.md is
edited, §4 and this ADR disagree on paper; per §0, this ADR is the resolution of that
disagreement and the amendment is tracked in `/docs/backlog.md`.

## Rollback path

Cheap right now, expensive after M2. Today the directories are empty — moving them is a
`git mv`. Once `/tests/golden/` holds committed hashes, a move rewrites the provenance of
every one of them, and the `golden:` commit convention in §6 exists precisely so that
changes to those files are reviewable in isolation. Decide before M2 or live with it.

## Verification

Directories exist and `cmake --workflow --preset ci` passes with `/core/tests/` as the only
CMake test directory. `/tests/` has no consumers until M2 by construction.
