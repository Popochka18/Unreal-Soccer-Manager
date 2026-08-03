# ADR-0006: Support MSVC via a portable/native 128-bit split

- **Status:** Accepted
- **Date:** 2026-08-03
- **Supersedes:** [ADR-0005](0005-one-compiler-family.md)
- **Amends:** nothing; restores the compiler support that ADR-0005 removed

## Context

ADR-0005 dropped MSVC. Its argument was that `Fixed` needs an exact 128-bit intermediate,
that GCC and Clang provide `__int128` while MSVC provides neither it nor a constexpr route
to the same answer, and that maintaining a second implementation which must stay
bit-identical forever is the least detectable way to violate §6.

Two things changed after it was written.

First, CI proved the *technical* premise sound: `clang-cl` builds the whole core on Windows
(Clang 20.1.8, 17/17). So the choice was never "Clang or nothing" — it was a genuine
trade-off with MSVC support on the other side.

Second, the project owner weighed that trade-off and reversed it. The deciding factor is
M7: Unreal's Windows toolchain defaults to MSVC, and ADR-0005's own risk section named
"Unreal cannot link a Clang-built `libpitchsim` on Windows" as the scenario that would
force a reversal. Discovering that at M7 means implementing this change *after* the golden
replay corpus exists, when proving bit-equivalence costs a corpus-wide differential
investigation rather than an afternoon.

Doing it now is strictly cheaper. That is the whole argument.

## Options considered

### 1. Stay Clang-only (ADR-0005 unchanged)

**Cost:** carries an unquantified M7 risk on the strength of a guess about Unreal's build
system. If the guess is wrong, the fix lands at the most expensive possible moment.

### 2. MSVC path selected per compiler, each using its own code in both contexts

The obvious implementation: `#if _MSC_VER` picks intrinsics, `#else` picks `__int128`.

**Cost:** this is the version ADR-0005 was right to fear. Compile-time evaluation on MSVC
would use one implementation and compile-time evaluation on GCC another, so a constant-folded
tunable could differ from platform to platform with nothing comparing them. Every existing
test would still pass, because they all check one implementation against expected values.

### 3. One portable implementation at compile time everywhere, native only at runtime, with a differential test *(chosen)*

Split by *context* rather than by compiler:

- `mul_shift_floor_portable` / `shift_div_floor_portable` — constexpr, no extensions, no
  intrinsics. Used by **every** compiler during constant evaluation.
- `mul_shift_floor_native` / `shift_div_floor_native` — `__int128` on GCC/Clang, `_mul128`
  on MSVC x64. Used only at runtime, where §7's 1.5 ms budget makes it matter.
- `std::is_constant_evaluated()` dispatches between them.
- `core/tests/test_wide.cpp` asserts the two agree bit-for-bit.

**Cost:** two implementations still exist, and the portable one is intricate — 32-bit limb
multiplication, two's-complement sign correction, and a restoring 128/64 division with a
65-bit remainder carry. It is more code than option 2 and harder to read. What it buys is
that divergence is *detectable*: there is exactly one axis of possible disagreement, and a
test sits on it.

## Decision

Option 3, in `core/pitchsim/include/pitchsim/wide.hpp`.

Supported compilers: **GCC, Clang, and MSVC on x64.** A compiler matching none of these
fails with an `#error` rather than silently selecting a degraded path.

### Division is portable on every platform, including Linux

Multiplication splits portable/native as described. **Division does not** — the portable
path runs everywhere, at compile time and at runtime, on all four compilers.

This was not the original design. The first implementation used `__int128` division at
runtime on GCC and Clang, and CI rejected it:

```
lld-link: error: undefined symbol: __divti3
```

128-bit division is a compiler-rt/libgcc *call*, not an instruction, and that runtime
library is not linked when targeting the MSVC ABI. Multiplication is unaffected — a
64×64→128 widening multiply is a single instruction with no libcall.

The find is worth more than the fix. That path had been in the tree since M0 and passed the
Windows CI leg, because every divide in the existing tests was constant-folded. It would
have failed the first time the engine performed a fixed-point division with runtime
operands. The differential test surfaced it only because it calls the native functions in a
loop with values the optimiser cannot fold.

Three reasons the portable divide is now used everywhere, in order of weight:

1. It has no runtime-library dependency, so it cannot fail to link on any target.
2. MSVC's `_div128` raises `SIGFPE` on quotient overflow rather than returning, converting
   a precondition violation into a process kill instead of a testable wrong answer.
3. Division is far colder than multiplication in the tick loop, so the portable path costs
   little — and removing an entire axis of platform variance is worth more to §6 than the
   cycles are worth to §7.

`shift_div_floor_native` still exists where it can link, purely as the differential test's
oracle. Nothing in the engine calls it. Verified: `nm -u libpitchsim.a` reports zero
`__divti3` references.

### The differential test is the load-bearing part

Without `test_wide.cpp` this ADR is a mistake and ADR-0005 was right. It compares the two
implementations over:

- operands at the sign, limb (2^32) and 64-bit overflow boundaries,
- 400 000 randomized operand pairs drawn from our own `Rng`, so any failure is reproducible
  from the seed alone,
- constexpr-vs-runtime evaluation of the same expression,
- reconstruction of `quotient * divisor + remainder == numerator` for a divisor above 2^63,
  which exercises the remainder-carry case.

Its value was verified by mutation, not assumed. Deleting one sign-correction term from the
portable multiply leaves every `static_assert` passing and is caught **only** by this test.
Deleting the division floor correction is caught at compile time by the `static_assert`s in
`version.cpp`. The two layers catch different faults, which is the point.

**If this test ever fails, do not adjust an expected value.** One of the two implementations
is wrong and every golden replay is meaningless until you know which.

## Consequences

**Easier:** M7 loses its largest unknown — Unreal can use whatever Windows toolchain it
prefers. Windows contributors need no LLVM install. Both Windows compilers are verified in
CI on every push.

**Harder:** three compilers and four codegen paths to keep in agreement, up from three.
CI's `core` matrix grows to four legs. Every future addition to `wide.hpp` needs a portable
implementation *and* a differential test case, and a reviewer who does not know that will
approve a change that silently breaks §6 on one platform.

**The specific thing to watch:** `is_constant_evaluated()` makes the compile-time path
different code from the runtime path *on every platform, including Linux*. A bug in the
portable implementation is therefore invisible to normal testing on any machine unless the
differential test covers the operand range where it manifests. Extend the ranges in
`test_wide.cpp` whenever `Fixed` gains an operation.

## Rollback path

Cheap, and it stays cheap. Reverting to ADR-0005 means deleting the MSVC branch in
`wide.hpp`, restoring the `#error`, and dropping the `windows-msvc` CI leg. The portable
implementation would remain valuable regardless — it is what makes the compile-time path
identical across platforms, which is worth having even with a single compiler.

## Verification

Linux, GCC 15.2, from a clean build directory: 24/24 tests pass on release, ASan and UBSan.
The differential test's ability to fail was confirmed by two deliberate mutations, both
reverted.

**The MSVC leg has never been compiled at the time of writing** — no MSVC toolchain is
available locally. CI is the verification, and the `windows-msvc` matrix entry added
alongside this ADR is what makes that ongoing rather than one-off.
