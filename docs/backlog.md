# Backlog

Ideas and deferred work that would otherwise bloat a diff (§0.6). Not a roadmap — §12 is the
roadmap. Entries here are things we decided *not* to do in the commit where we noticed them.

## Amendments applied to CLAUDE.md

All four were applied on 2026-08-03 as mechanical consequences of ADR-0002 and ADR-0004.
Listed here so they are easy to review and revert.

- [x] **§4: added `/tests/{golden,calibration,fixtures}/` rows.** ADR-0004 splits test code
      (`/core/tests/`) from test data, resolving a contradiction between §4 and §3/§6/§7/§11.
- [x] **§4: added `/app/tauri/`.** Recorded in ADR-0002. §9 requires a Tauri shell; §4 did
      not say where it lives.
- [x] **§2: the diagram said "UI SHELL (npm)" while §9 mandates pnpm.** Amended to pnpm.
- [x] **§12: the M0 gate said "build for all three targets".** ADR-0002 defers UE5 to M7, so
      M0 is two targets. Amended rather than left quietly unmet.

## Native core

- [ ] **Property tests for `Fixed`.** Current coverage is eight example-based tests. The
      invariants worth asserting over ranges: `from_ratio` monotonic in the numerator;
      `(a*b)/b` bounded error; floor composition; no overflow within the documented operand
      range. Do this before `libpitchsim` grows arithmetic that relies on them.
- [ ] **`Fixed` overflow policy.** `operator*` and `operator/` document a precondition but do
      not enforce one. Decide between a debug assert, a saturating variant, or a checked
      variant for finance. Finance is where this will actually bite — §11 requires invariants
      that hold exactly.
- [ ] **Division by zero in `Fixed::operator/`.** Currently UB via the 128-bit divide. Needs
      the same decision as above.
- [ ] **`RngStream` is append-only but nothing enforces it.** Inserting a value in the middle
      renumbers every stream after it and invalidates every save and golden replay. Worth a
      static assertion on the enum's size, or a test pinning each value.
- [ ] **Windows-on-ARM.** ADR-0006 supports GCC, Clang and MSVC on x64. ARM64 is untested
      and unclaimed; `wide.hpp` `#error`s there rather than degrading silently. MSVC on ARM64
      would need the portable path at runtime too, since `_mul128` is x64-only — which the
      current structure already supports, so this is mostly a CI question.
- [ ] **Extend `test_wide.cpp` whenever `Fixed` gains an operation.** ADR-0006 makes the
      compile-time path different code from the runtime path on *every* platform, so a bug in
      the portable implementation is invisible to normal testing unless the differential test
      covers the operand range where it shows up.

## Tooling

- [ ] **`determinism-guard.sh` misses mutable static state**, uninitialised reads, and
      pointer-keyed ordered containers. It also cannot see into macros. The sanitizer builds
      and the M2 cross-OS comparison are the real checks; the guard is a cheap first net and
      its README should keep saying so.
- [ ] **No `clang-format` / `clang-tidy` config.** Worth adding before there are enough
      contributors for style to become an argument.
- [ ] **Dependency archives are not mirrored.** ADR-0003 accepts that an upstream archive
      disappearing breaks old commits. Revisit if it happens once.

## Frontend

- [ ] **TanStack Query/Table/Virtual and Zustand are not installed.** §9 names them; they
      arrive with the screens at M6. Adding them at M0 would be unused dependencies.
- [ ] **Playwright is not set up.** §9 requires it for the critical journeys, which do not
      exist until M6.
- [ ] **No theme-loading path for modder-supplied themes.** `parseTheme` validates, but
      nothing reads a theme from disk yet. Needs the asset/mod pipeline (M5) and a decision
      about where user themes live.

## Open questions

- [ ] **Does the UE5 Windows toolchain link a Clang-built `libpitchsim`?** The main risk
      ADR-0005 takes on. Test this early in M7, not late.
- [ ] **Fixed-point range audit.** q16.16 in `int64` gives ~1.5e-5 resolution over ±1.4e14.
      Confirm that is right for currency: at 1e-5 precision, money in pennies overflows the
      *fractional* budget long before the integer one. Finance may want its own integer type
      rather than `Fixed`.
