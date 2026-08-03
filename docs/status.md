# Status

**M0 complete. Next: M1 — schema, pack compiler, fictional base pack.**
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
| Tauri shell builds | PASS | `cargo build` links `target/debug/pitchforge-shell` (ELF x86-64) in 1 m 02 s. `cargo clippy --all-targets -- -D warnings` clean. Rust 1.97.1, Tauri 2.11.5. |
| Windows core | PASS | CI run 30832655990, clang-cl / Clang 20.1.8, Release: 17/17 in 0.60 s. ADR-0005 validated — `__int128` and the `constexpr` `Fixed` asserts compile under Clang on Windows. |
| macOS core | PASS | CI run 30832655990, Apple Clang: 17/17 in 56 s. |
| CI runs green | PASS | Run 30832655990: **all 9 jobs green** — core ×3, sanitizers ×2, determinism, app, shell, pack. |
| ADR-0001 written | PASS | `docs/adr/0001-architecture.md`, plus 0002–0005. |

### Verdict

**M0 is complete.** Every gate item is verified by a command that was executed, on all three
platforms, in CI. The gate — "one command builds everything" — is met by
`cmake --workflow --preset ci` plus the `/app` script chain.

Two defects were found and fixed by the first CI runs, both in CI configuration rather than
in project code:

1. The Windows leg pinned the generator to `Visual Studio 17 2022`; the runner ships VS 18.
   Replaced with Ninja + clang-cl and a version-agnostic MSVC environment setup, so an image
   refresh cannot break it the same way again.
2. `/fp:precise` and `-ffp-contract=off` conflict under clang-cl, and `/WX` promoted the
   warning to an error. `/fp:precise` was dropped — `-ffp-contract=off` is stricter and is
   the one §6 needs. Both branches now pass it last so nothing can reset contraction.

### Local prerequisites (Ubuntu 26.04)

Four packages beyond a stock install; the rest of Tauri's documented list ships preinstalled:

```bash
sudo apt install libwebkit2gtk-4.1-dev libxdo-dev libayatana-appindicator3-dev librsvg2-dev
```

## Top three risks carried into M1

1. **Cross-OS determinism is still unproven, despite three green platforms.** All three build
   and pass the same 17 tests — but those tests assert *properties*, not byte-identical
   output. Nothing yet compares a Linux result against a Windows one. §6's actual promise
   only gets tested at M2, when golden replays exist and the `determinism` job stops being a
   grep guard and starts being a cross-OS hash comparison. Treat the current green as
   "it compiles and behaves sanely everywhere", not as "it is deterministic".
2. **The fixed-point core has no property tests.** `Fixed` has 8 example-based tests. The
   invariants that matter — associativity bounds, round-trip error, monotonicity of the floor
   under composition — are asserted at specific values, not over ranges. A property test
   harness should land before `libpitchsim` grows arithmetic that depends on them.
3. **Three compilers now, and they will drift.** GCC 15.2, Apple Clang and Clang 20.1.8 each
   get their own codegen. ADR-0005 narrowed this from four to three by dropping MSVC, but
   every additional compiler is a permanent tax on §6. The `-ffp-contract` conflict found on
   day one is the mild version of this class of problem; the severe version is a flag that
   silently differs rather than erroring.

## Milestone ledger

| M | Deliverable | State |
|---|---|---|
| M0 | Repo skeleton, build, CI, ADR-0001 | **complete** — CI green on Linux, macOS, Windows |
| M1 | Schema + pack compiler + fictional base pack | not started |
| M2 | `libpitchsim` v1 + golden replays + `replaydiff` | not started |
| M3 | Headless server: calendar, fixtures, results, save/load | not started |
| M4 | Fast-sim calibration + game week ≤ 3 s | not started |
| M5 | Asset pipeline: crests, faces, kits | not started |
| M6 | UI shell + 11 screens + save/continue loop | not started |
| M7 | UE5 renderer (deferred here by ADR-0002) | not started |
| M8 | AI managers, scouting, media, youth, board | not started |
