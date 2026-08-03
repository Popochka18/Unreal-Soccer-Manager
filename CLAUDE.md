# CLAUDE.md — GRIDIRON/PITCH PROJECT MASTER DIRECTIVE

> Codename: **PITCHFORGE**. A Football-Manager-class management simulation.
> This file is the highest authority in the repository. If any instruction, ticket,
> comment, or previous decision contradicts this file, **this file wins** — or you
> stop and open an ADR. You never silently deviate.

---

## 0. YOUR ROLE AND OPERATING CONTRACT

You are the principal engineer on a multi-year simulation project, not a code-snippet
vending machine. You behave accordingly:

1. **Read before you write.** Before touching any subsystem, read its `README.md`,
   its ADRs in `/docs/adr/`, and its tests. Never re-implement something that exists.
2. **Plan, then execute.** For any task larger than ~50 LOC, first output a numbered
   plan: files touched, contracts changed, tests added, rollback path. Wait for `GO`
   only if the plan changes a public contract (schema, IPC, save format). Otherwise proceed.
3. **Contracts are law.** DB schema, IPC message shapes, save-file format and the
   match event stream are versioned contracts. Changing one without a migration +
   version bump + compatibility test is a build-breaking offence.
4. **Tests first for anything with a number in it.** Simulation, finance, ratings,
   scheduling — write the failing test first, then the code.
5. **Report honestly.** If a task is infeasible, if the requested design is wrong, or
   if you took a shortcut — say it in plain language at the top of your reply under
   `⚠️ DEVIATIONS`. Never claim something works that you did not run.
6. **No scope creep.** One task, one branch, one concern. Extra ideas go to
   `/docs/backlog.md`, not into the diff.
7. **Small diffs.** > 400 changed lines without an explicit instruction = reject your
   own work and split it.

---

## 1. PRODUCT DEFINITION (what we are actually building)

A single-player, offline-first football management simulation:

- **Scale target:** 60+ playable leagues, ~500k persons in the database, ~250k of them
  simulated in detail, 20+ seasons of continuous play without database rot.
- **Loop:** squad/tactics/training/transfers/finance/staff/scouting/media → continue →
  deterministic match simulation → results, news, evolution of every entity.
- **Match presentation:** three synchronized views over the *same* event stream —
  text commentary, 2D top-down, 3D (UE5). All three are *pure consumers*. None of them
  may compute anything about the match.
- **Moddable to the bone.** Every rule, competition format, nation, attribute weight and
  string is data. A modder with a text editor and a folder of images can build a new
  football world without recompiling anything.

---

## 2. NON-NEGOTIABLE ARCHITECTURE

```
┌──────────────────────────────────────────────────────────────────────┐
│  UI SHELL (pnpm / TypeScript)                                        │
│  Tauri v2 window · React 18 · Vite · TanStack Query+Table+Virtual     │
│  Zustand for view state · zero game logic, zero rules, zero maths     │
└───────────────▲──────────────────────────────────────────────────────┘
                │  loopback HTTP (commands/queries) + WebSocket (streams)
                │  MessagePack payloads, schema-versioned, generated types
┌───────────────▼──────────────────────────────────────────────────────┐
│  GAME SERVER  (native, C++20, headless, no UI, no engine deps)       │
│  world state · calendar · transfers · finance · AI managers · saves   │
│  links →  libpitchsim (match engine, deterministic, fixed-point)      │
│  owns →  SQLite data layer + asset manifest                          │
└───────────────▲──────────────────────────────────────────────────────┘
                │  same libpitchsim, linked statically
┌───────────────┴──────────────────────────────────────────────────────┐
│  UE5 MATCH RENDERER (separate process, launched on demand)           │
│  thin C++ plugin wrapping libpitchsim + Niagara/anim/crowd/stadium    │
│  consumes the event stream · renders · sends nothing back but input   │
└──────────────────────────────────────────────────────────────────────┘
```

**Read this twice:** the match engine is a *standalone C++ library* (`libpitchsim`)
with **zero Unreal dependencies**. UE5 links it; the headless server links it. This is
what "UE5 is the match backend" means in practice — the football is in the library,
the pixels are in Unreal. Gameplay code that includes `CoreMinimal.h` is rejected.
Rationale: headless season simulation must run thousands of matches per second with no
editor, no RHI, no game thread. If the simulation lives inside UE5 classes, that is
impossible and the project dies at milestone M4.

---

## 3. HARD NOs (violating any of these is a revert, not a discussion)

- ❌ No gameplay constant, name, club, rule, formula weight or string **in source code**.
  If you type a club name, a nation, an attribute weight or a competition rule into a
  `.cpp`, `.ts`, or `.h` file — you are wrong. It goes in the database.
- ❌ No `float`/`double` anywhere in simulation state or in any calculation that affects
  match/season outcome. Fixed-point `q16.16` (`int64`) only. Floats are permitted **only**
  in rendering and in UI display formatting.
- ❌ No `std::unordered_map` iteration order, `time()`, `rand()`, thread-id, pointer value,
  or any other non-determinism source inside the sim.
- ❌ No business logic in the frontend. If the UI needs a number, the server computes it.
  The UI may sort, filter and format. It may not decide anything.
- ❌ No Unreal types in `libpitchsim`. No `FString`, `UObject`, `TArray`, `AActor`.
- ❌ No blocking I/O on the sim thread. No allocations in the per-tick hot path.
- ❌ No ORM, no reflection-based magic, no dependency-injection framework, no `any`,
  no `// @ts-ignore`, no `catch {}` that swallows.
- ❌ No real-world player names, real club crests, real kit designs, or scraped datasets
  in the repository. Fictional or explicitly licensed content only. See §14.
- ❌ No committed binaries > 1 MB. Assets live in a content-addressed store, referenced
  by hash, fetched or generated by the pipeline.
- ❌ No "temporary" hardcoded test data that ships. Fixtures live under `/tests/fixtures/`.

---

## 4. REPOSITORY LAYOUT (canonical — do not invent new top-level folders)

```
/core/pitchsim/         C++20 match engine library. Pure. Deterministic. No deps but STL.
/core/world/            Season/world simulation: calendar, ai, finance, transfers, dev.
/core/server/           Headless binary: HTTP+WS, save/load, DB access, job scheduler.
/core/db/               Schema, migrations, query layer (sqlite3 amalgamation, no ORM).
/core/tests/            Unit + property tests. Catch2. Test *code* only — see /tests/.
/tests/golden/          Recorded matches: input blob + blake3 of the event stream. (ADR-0004)
/tests/calibration/     Bands for the soak and the 10k-match harness. (ADR-0004)
/tests/fixtures/        Shared test data. The only legitimate home for hardcoded content.
/ue5/PitchForgeMatch/   UE5 5.4 project. Thin. Renderer + audio + camera only. (M7, ADR-0002)
/ue5/Plugins/PitchSim/  C++ plugin: wraps libpitchsim, translates events → actors. (M7)
/app/                   pnpm workspace root. Tauri v2 shell.
  /app/ui/              React + Vite renderer.
  /app/ipc/             Generated TS types + client from the IPC schema. NEVER hand-edited.
  /app/design/          Tokens, primitives, dense-table system.
  /app/tauri/           Tauri v2 Rust crate: launcher and bridge only. (ADR-0002)
/data/packs/base/       The official data pack: .csv/.json5 sources + images.
/data/schema/           JSON-Schema for every data pack file. Validation is CI-gated.
/tools/                 Pack compiler, face generator, logo pipeline, replay differ, bench.
/docs/adr/              Architecture Decision Records, numbered, immutable once merged.
/docs/contracts/        DB schema doc, IPC spec, event stream spec, save format spec.
```

---

## 5. DATA LAYER CONTRACT

**Storage:** SQLite (amalgamation, compiled in, WAL). Two distinct databases:

| DB | Purpose | Mutability |
|---|---|---|
| `world.db` | Compiled data pack: nations, competitions, clubs, persons, rules, assets | Read-only at runtime |
| `save.db`  | A save game: deltas, history, calendar state, RNG state, player career | Read/write |

**Rules:**

1. The pack compiler (`/tools/packc`) turns human-editable `json5`/`csv` sources into
   `world.db`. Humans never edit `world.db` directly. Modders edit the sources.
2. Layered packs: `base` → `official-update` → `user-mod-*`. Later layers patch earlier
   ones by stable string ID (`club.eng.northbridge_utd`), never by numeric row ID.
   Conflicts are reported by the compiler with the pack names — never silently resolved.
3. Every table has: `id INTEGER PRIMARY KEY`, `uid TEXT UNIQUE NOT NULL` (stable, modder-
   facing), `pack TEXT NOT NULL` (provenance). Foreign keys ON. `STRICT` tables.
4. On load, the server materializes the pack into cache-friendly SoA arrays. SQLite is a
   loading and persistence format, **not** the runtime query engine. Nothing in the sim
   loop touches SQL.
5. Migrations are forward-only, numbered, and each ships a test that loads the previous
   version's save and asserts equality of a hashed world summary.
6. **Localization:** no user-facing string in code. `strings` table keyed by `str.*` uid,
   with per-language columns. Commentary templates are data with slot grammar.

**Core entity families** (each gets its own schema doc; this is the minimum):

`person` (player/staff/referee shared base) · `player_attributes` (technical/mental/physical/
hidden/positional familiarity) · `staff_attributes` · `contract` · `club` · `club_finance` ·
`stadium` · `competition` · `competition_rules` (format graph, qualification, prize money,
registration rules) · `season` · `fixture` · `match_result` · `match_event` ·
`nation` · `region` · `city` · `transfer` · `injury` · `training_schedule` · `tactic` ·
`media_outlet` · `news_item` · `asset` (logos/faces/kits) · `strings`.

---

## 6. DETERMINISM POLICY (the spine of the project)

- One `Rng` type: SplitMix64/PCG, explicit state, **passed in**, never global. Streams are
  derived per-purpose: `rng.derive(RngStream::MatchPhysics, matchId, tick)`. Adding a call
  site must not shift any other stream.
- Match input = `(seed, home tactic snapshot, away tactic snapshot, roster snapshot,
  pitch/weather state, ruleset version)`. Same input ⇒ byte-identical event stream, on
  every OS, compiler and CPU, forever.
- `/tests/golden/` holds ~200 recorded matches: input blob + `blake3` of the event stream.
  CI fails on any hash change. If a change is intentional, the diff must be regenerated in
  a dedicated commit titled `golden: <reason>` and reviewed on its own.
- `-ffp-contract=off`, no `-ffast-math`, no SIMD auto-vectorization in sim TUs, no
  parallelism inside a single match. Parallelism happens **across** matches only.
- Save files store the RNG state of every stream. Load → continue → the future is identical.
- `/tools/replaydiff` prints the first divergent tick between two streams. Use it before
  you guess.

---

## 7. MATCH ENGINE CONTRACT (`libpitchsim`)

- Fixed timestep: 10 Hz decision tick, 50 Hz movement tick. No variable dt. Ever.
- Model: 22 agents + ball, continuous 2D positions in fixed-point metres, plus ball height.
  Per-agent: role instruction, mentality, stamina, morale, current decision, marking assignment.
- Decision layer: utility scoring over candidate actions (pass targets, dribble vectors,
  shot, hold, clear, press, drop). Weights come from the DB, not from code.
- Physics: simple but honest — ball flight with drag, bounce, spin bucketed; no full 3D
  rigid body. Realism budget goes into *decisions*, not into ballistics.
- Output: an append-only, versioned **event stream**:
  `tick, type, actors[], position, payload` for both micro (position snapshots, 5 Hz,
  quantized) and macro (pass, tackle, foul, shot, goal, sub, injury) events.
  The stream is the single source of truth for: commentary, 2D, UE5 3D, stats, highlights,
  ratings and post-match analysis. Nothing recomputes anything from anything else.
- Two run modes from one code path:
  - `simulate_full()` — every tick, for matches the human watches or that need full stats.
  - `simulate_fast()` — same decision model, coarser movement, statistically calibrated
    against `simulate_full` (KS-test on 10k matches, tolerance in `/tests/calibration/`).
    Used for the other 40 000 matches of the game week.
- Perf budget: `simulate_fast` ≤ **1.5 ms** single-threaded per match; `simulate_full`
  ≤ 40 ms. A full world game-week (all 60 leagues) ≤ **3 s** on 8 cores.

---

## 8. UE5 BOUNDARY (read this before opening the editor)

UE 5.4+, C++ project, Blueprints only for cosmetic wiring and never for anything
that reads game state.

**UE5 is allowed to:** render the pitch, stadium, crowd, players and ball; run animation
graphs and Motion Matching; run cameras, replays, weather VFX, audio; draw a minimal
in-match overlay; forward user input (speed, camera, pause, sub request) to the server.

**UE5 is forbidden from:** deciding anything about the match; storing world state; owning
the save; talking to SQLite; being required for a headless season; being launched at
startup.

**Integration:**

- Launched on demand as a child process with a session token + shared-memory ring buffer
  (fallback: local WebSocket). Crash of the renderer must never corrupt or interrupt a save.
- The plugin translates events → actor commands. It interpolates between 5 Hz position
  snapshots; it never extrapolates gameplay.
- Kits, faces and crests are streamed in as runtime textures from the asset store, applied
  to material instances. No per-club assets are cooked into the UE5 project.
- Editor content is asset-only (meshes, anims, materials, Niagara). Any gameplay `.uasset`
  is a defect.

---

## 9. FRONTEND CONTRACT (`/app`)

- pnpm workspaces, TypeScript `strict` + `noUncheckedIndexedAccess`, ESLint flat config,
  Vitest, Playwright for the critical journeys.
- Tauri v2 shell (Rust side is a launcher and a bridge — no logic). The UI must also run
  in a plain browser against a running server for development.
- IPC: single generated client in `/app/ipc`, produced from `/docs/contracts/ipc.schema.json`.
  Every request/response is typed and versioned. `/app/ui` may not `fetch()` anything itself.
- **This UI is dense data, not a landing page.** Tables of 5 000 rows must scroll at 60 fps
  (TanStack Virtual, no per-row React context, memoized cells, no layout thrash).
  Keyboard-first: every screen reachable without a mouse; `/` opens a global entity search.
- Design system in `/app/design`: tokens for spacing/typography/colour, one dense table
  primitive, one attribute-cell primitive with the colour ramp, one comparison primitive.
  No ad-hoc CSS values, no component library that fights us.
- Skinnable: colours, fonts and layout density come from a theme file that a modder can ship.
- Minimum screen set for M6: Inbox, Squad, Player Profile, Tactics, Training, Schedule,
  Transfers/Search, Finances, Competition (table/fixtures/stats), Staff, Match Day.

---

## 10. ASSET PIPELINE — LOGOS, FACES, KITS

**Storage:** content-addressed. `/<store>/<sha256[0:2]>/<sha256>.<ext>`, described by the
`asset` table: `uid, kind, sha256, w, h, license, pack`. Entities reference assets by uid.
Missing asset ⇒ deterministic procedural fallback, never a broken image, never a crash.

**Crests/logos:** authored or generated as SVG; pipeline rasterizes to WebP at 32/64/128/512
with correct premultiplied alpha. A generator (`/tools/crestgen`) composes fictional crests
from shield shapes + charges + palettes seeded by `club.uid`, so a generated world always
looks complete. Club colours in the DB drive kits *and* crest palettes.

**Faces:** two interoperable paths, both mandatory.

1. **Procedural, seeded by `person.uid`:** layered PNG parts (head shape, skin tone ramp,
   eyes, nose, mouth, brow, hair, facial hair, ears, wrinkles/age overlay) selected with
   weights conditioned on nationality/ethnic-region field, age and a hidden appearance
   gene stored on the person. Composited by `/tools/facegen` into a WebP at 128/256, cached
   by hash. Deterministic: same person ⇒ same face in every save, on every machine.
2. **Facepack override:** a mod folder maps `person.uid` → image file; the pack compiler
   ingests it into the asset store. Overrides always win.
   The same layer set feeds UE5 as material parameters for the 3D head.

- Ageing: hair/skin/wrinkle layers shift with age using the same seed. A 34-year-old is a
  recognizably older version of the same 21-year-old.
- Budget: face composite ≤ 4 ms, crest raster ≤ 2 ms, both off the main thread, LRU-cached,
  pre-warmed for the visible viewport only.

---

## 11. TESTING & DEFINITION OF DONE

A task is **done** only when all of the following are true:

- [ ] Unit tests for new logic; property tests for anything with invariants
      (finance never NaNs, squad size bounds, contract dates, table points arithmetic).
- [ ] Golden replay hashes unchanged, or regenerated in a dedicated reviewed commit.
- [ ] A 10-season headless soak (`/tools/soak --seasons 10`) passes: no crash, no leak,
      no wage/transfer/inflation runaway, age pyramid and league quality within bands
      defined in `/tests/calibration/bands.json5`.
- [ ] Perf budgets in §7/§9 measured, not assumed. Numbers pasted in the PR body.
- [ ] Contracts updated in `/docs/contracts/` if touched; migration + compat test if schema.
- [ ] `ASAN`+`UBSAN` build green; TS typecheck and lint green; no new warnings.
- [ ] `⚠️ DEVIATIONS` section written, even if it says "none".

---

## 12. MILESTONES (do not work out of order without an ADR)

| M | Deliverable | Gate |
|---|---|---|
| M0 | Repo skeleton, build for core + /app, CI, ADR-0001 (UE5 deferred, ADR-0002) | one command builds everything |
| M1 | Schema + pack compiler + fictional base pack (4 nations, 8 divisions) | `packc` builds `world.db`, validation green |
| M2 | `libpitchsim` v1 + golden replays + `replaydiff` | 200 golden matches stable across 3 OSes |
| M3 | Headless server: calendar, fixtures, results, tables, save/load | 5 seasons headless, no drift |
| M4 | Fast-sim calibration + full world game week ≤ 3 s | KS-test within tolerance |
| M5 | Asset pipeline: crests, faces, kits, fallbacks | 500k persons have deterministic faces |
| M6 | UI shell + the 11 core screens + full save/continue loop | playable, keyboard-only run of a season |
| M7 | UE5 renderer consuming the event stream | 3D match visually synced to text/2D, ±0 divergence |
| M8 | Depth pass: AI managers, scouting, media, youth intake, board | 20-season soak stable |

---

## 13. AI MANAGER & WORLD-EVOLUTION PRINCIPLES

- The human manager gets **no** exclusive systems. Every AI club runs the same code paths
  for tactics, training, transfers, contracts and squad building. If the human can do it,
  the AI does it with the same functions.
- Transfer market clears globally per window with a valuation model that is *data-driven*
  and auditable: `/tools/valuation --explain <player>` prints the term-by-term breakdown.
- Player development uses a hidden potential band + coaching + minutes + age curve +
  a personality/professionalism modifier. No teleporting wonderkids, no dead leagues:
  the calibration bands in M4 gate this.
- Economy is closed-loop and inflation-aware; wage/revenue ratios must stay in band across
  a 20-season soak, per league tier.

---

## 14. LEGAL & CONTENT GUARDRAILS

The repository ships a **fictional football world**. No real player names, real club names,
real crests, real kits, real competition names or scraped third-party databases. The data
pack format is designed so users can add their own content locally; we do not distribute it,
do not link to it, and do not build tooling whose sole purpose is importing it. Every asset
row carries a `license` field and CI fails on `unknown`.

---

## 15. HOW TO ANSWER ME

- Terse. Engineering register. No praise, no recap of what I just said.
- Lead with the plan or the diff, not with preamble.
- End every non-trivial reply with `⚠️ DEVIATIONS` and `NEXT: <one concrete step>`.
- If I ask for something that contradicts this file, say so and refuse until I amend the file.

---

## 16. SLASH COMMANDS

The project commands live in `.claude/commands/` — one file per command, invoked as `/name`:

`/bootstrap` `/adr` `/schema` `/seed` `/sim` `/match` `/determinism` `/ue` `/api` `/ui`
`/assets` `/bench` `/review` `/fix` `/mod` `/ship` `/soak`
