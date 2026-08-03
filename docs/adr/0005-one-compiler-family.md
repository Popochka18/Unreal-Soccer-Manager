# ADR-0005: One compiler family — Clang on Windows, not MSVC

- **Status:** Accepted
- **Date:** 2026-08-03
- **Amends:** nothing explicitly; makes a §6 obligation concrete

## Context

§6 requires that the same match input produce a byte-identical event stream "on every OS,
compiler and CPU, forever". §3 requires all outcome-affecting arithmetic to be fixed-point
`q16.16` in `int64`.

Fixed-point multiply and divide need an exact 128-bit intermediate: a q16.16 product needs
128 bits before the shift, and a naive `int64` multiply overflows at operand magnitudes the
game reaches easily (the test `multiplication uses an exact 128-bit intermediate` uses
1e9 × 10, whose raw product is 4.29e19 against an `int64` ceiling of 9.22e18).

GCC and Clang provide `__int128`, which is exact, constant-evaluable, and one line.
MSVC provides neither `__int128` nor a constexpr path to the same result.

This surfaced at M0: `core/pitchsim/include/pitchsim/fixed.hpp` does not compile under MSVC,
so the Windows leg of CI had to be decided before it could be written.

## Options considered

### 1. Implement an MSVC path with `_mul128` / `_div128`

**Cost:** the intrinsics are not `constexpr`, so every compile-time use — the `static_assert`
block in `version.cpp`, and every `constexpr Fixed` tunable we will want later — needs a
*second*, portable, hand-rolled 128-bit implementation selected via
`std::is_constant_evaluated()`. That is roughly 150 lines of long multiplication and
restoring division which must be **bit-identical** to both the intrinsic path and the
`__int128` path, forever, or §6 is violated in the least detectable way possible: a Windows
build that agrees with Linux on 99.99% of matches. `_div128` is also x64-only, so
Windows-on-ARM would need a third path. None of it can be verified on this machine.

### 2. Drop `constexpr` from `Fixed` so the intrinsics suffice

**Cost:** loses compile-time verification of the fixed-point invariants — the `static_assert`s
that currently catch a floor-vs-truncate regression at build time — and forecloses
compile-time tunables. It makes every platform worse to solve a problem on one.

### 3. Build Windows with Clang *(chosen)*

`clang-cl` (or plain `clang`) on Windows x64 supports `__int128` and targets the MSVC ABI, so
it links against the Windows SDK and, later, against Unreal's Windows toolchain.

**Cost:** we give up MSVC-specific tooling on the native core — some Visual Studio debugger
integration is smoother with MSVC codegen, and any future dependency that ships MSVC-only
build files becomes friction. Contributors on Windows need LLVM installed, which is one more
setup step. UE5's Windows toolchain defaults to MSVC, and while Unreal supports building with
Clang on Windows, M7 will have to configure it — that risk is real and is the main cost here.

## Decision

The native core is built with **GCC or Clang only**. Windows CI uses `clang-cl`. MSVC is not
a supported compiler for `/core/`, and `fixed.hpp` `#error`s on it rather than silently
degrading.

This is narrower than it sounds. It constrains `/core/` only:

- `/app` is TypeScript and Rust and is unaffected.
- `/ue5/` renderer code (M7) is *rendering*, not simulation. It may build with whatever
  Unreal prefers, provided it links a `libpitchsim` built by Clang. If that turns out to be
  awkward, ADR-0001's escape hatch applies: the renderer consumes the event stream over IPC
  instead of linking the library.

The secondary benefit is the one that actually motivates it: **one compiler family across all
three platforms is the single largest reduction in cross-OS divergence risk available to us.**
Two codegen backends means two sets of optimisation decisions to keep byte-identical for the
life of the project. §6 is a twenty-year promise; every compiler we support is a permanent
tax on it.

## Consequences

**Easier:** one implementation of the fixed-point core, `constexpr` throughout, verifiable
compile-time invariants. Cross-OS determinism becomes mostly a question of libc and
floating-point environment rather than of codegen. The `/determinism` cross-OS hash
comparison is far more likely to be green for real reasons.

**Harder:** Windows contributors need LLVM. Visual Studio debugging of the core is less
seamless. **M7 must configure Unreal's Windows build to consume a Clang-built static
library** — put that on the M7 risk list, not on a list nobody reads.

**What would reverse this:** discovering at M7 that Unreal cannot link a Clang-built
`libpitchsim` on Windows and cannot itself be built with Clang. That is the scenario to test
early in M7, not late.

## Rollback path

Moderate cost, and it gets worse over time. Reverting means implementing option 1 — the dual
constexpr/intrinsic MSVC path — and then proving bit-equivalence against the `__int128` path
across the full golden replay corpus. At M0, with ~40 lines of arithmetic and no goldens, that
is a day's work. After M2, it is a day's work plus a corpus-wide differential test, and any
mismatch is a genuine determinism bug to hunt.

## Verification

CI builds `/core/` on `ubuntu-latest` (GCC), `macos-latest` (Apple Clang) and
`windows-latest` (clang-cl), running the same test suite on each. The determinism job
compares golden hashes across all three from M2 onward.

**As of M0 the Windows leg is unverified in this session** — it has only ever been built with
GCC 15.2 on Linux. The first CI run is the verification.
