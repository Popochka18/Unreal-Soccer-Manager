# Status

**Current milestone: M0 — repo skeleton, build, CI, ADR-0001**
Last updated: 2026-08-03

Nothing below is marked PASS that was not executed. Items that were not run say so.

## M0 gate: "one command builds everything"

| Item | Verdict | Evidence |
|---|---|---|
| Canonical layout (§4) | PASS | `/core/{pitchsim,world,server,db,tests}`, `/app/{ui,ipc,design,tauri}`, `/data`, `/tools`, `/docs`, `/tests` on disk. `/ue5/` deliberately absent — ADR-0002. |
| Native core builds | PASS | `cmake --workflow --preset ci` — configure, build, 17/17 tests, **0 warnings**. GCC 15.2, Ubuntu 26.04. |
| ASan green (§11) | PASS | `cmake --workflow --preset asan` — 17/17, 0.23 s. |
| UBSan green (§11) | PASS | `cmake --workflow --preset ubsan` — 17/17, 0.11 s. |
| `/app` typecheck | PASS | `pnpm -r typecheck` — design, ipc, ui all clean under `strict` + `noUncheckedIndexedAccess` + `exactOptionalPropertyTypes`. |
| `/app` lint | PASS | `pnpm lint` — clean. |
| `/app` tests | PASS | `pnpm -r test` — 3 vitest tests pass; `ipc:check` confirms the generated client matches the contract. |
| `/app` build | PASS | `pnpm -r build` — vite 6.4.3, 32 modules, 147.42 kB (47.59 kB gzip). |
| Determinism guard | PASS | `./tools/determinism-guard.sh` clean; verified it *fails* on a planted `double` and ignores the same word in a comment. |
| Tauri shell builds | **NOT-VERIFIED** | `cargo check` fails at `gio-sys`: `gio-2.0.pc` missing. Needs `apt` and therefore sudo. Crate is scaffolded, never compiled. |
| Windows / macOS core | **NOT-VERIFIED** | Only ever built with GCC 15.2 on Linux in this session. The first CI run is the verification. ADR-0005 puts Windows on clang-cl. |
| CI runs green | **NOT-VERIFIED** | `.github/workflows/ci.yml` is written but has never executed — no remote is configured. |
| ADR-0001 written | PASS | `docs/adr/0001-architecture.md`, plus 0002–0005. |

### Verdict

**M0 is not complete.** The build and test story is real and verified on Linux; the
cross-platform and CI claims are not. M0 closes when the first CI run is green and
`cargo check` passes in `app/tauri`.

### To unblock the Tauri leg

```bash
sudo apt install libwebkit2gtk-4.1-dev build-essential curl wget file libxdo-dev libssl-dev libayatana-appindicator3-dev librsvg2-dev
```

## Top three risks carried into M1

1. **Cross-OS determinism is entirely unproven.** §6 promises byte-identical output on every
   OS, compiler and CPU. We have one compiler on one OS. ADR-0005 reduces the risk by
   standardising on one compiler family, but the claim stays untested until CI runs on all
   three — and genuinely untested until M2 puts real golden replays behind it.
2. **The fixed-point core has no property tests.** `Fixed` has 8 example-based tests. The
   invariants that matter — associativity bounds, round-trip error, monotonicity of the
   floor under composition — are asserted at specific values, not over ranges. A property
   test harness should land before `libpitchsim` grows arithmetic that depends on them.
3. **`clang-cl` + `Fixed` has never been compiled.** ADR-0005 commits the project to it on
   the strength of Clang supporting `__int128` on Windows. If the ClangCL toolset in the VS
   generator behaves differently than expected, the Windows leg needs rework before M1
   rather than after.

## Milestone ledger

| M | Deliverable | State |
|---|---|---|
| M0 | Repo skeleton, build, CI, ADR-0001 | in progress — see above |
| M1 | Schema + pack compiler + fictional base pack | not started |
| M2 | `libpitchsim` v1 + golden replays + `replaydiff` | not started |
| M3 | Headless server: calendar, fixtures, results, save/load | not started |
| M4 | Fast-sim calibration + game week ≤ 3 s | not started |
| M5 | Asset pipeline: crests, faces, kits | not started |
| M6 | UI shell + 11 screens + save/continue loop | not started |
| M7 | UE5 renderer (deferred here by ADR-0002) | not started |
| M8 | AI managers, scouting, media, youth, board | not started |
