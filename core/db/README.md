# core/db — data layer

CLAUDE.md §5 is the contract.

## Current state (M1, schema version 2)

The SQLite amalgamation is wired up as a target (pinned by SHA256 in
`cmake/Dependencies.cmake`, see ADR-0003) with `SQLITE_DQS=0`,
`SQLITE_DEFAULT_FOREIGN_KEYS=1` and extension loading compiled out.

`PitchForge::db` adds the forward-only migration runner
(`pitchforge/db/migrate.hpp`) and the migrations in `./schema`:

| Version | File | Tables |
|---|---|---|
| 1 | `001_strings_geography.sql` | `strings`, `nation`, `region`, `city` |
| 2 | `002_person.sql` | `person` |

Still missing: the hand-written query layer (no ORM, §3) and the pack compiler in
`/tools/packc`, which is what turns `/data/packs/` into `world.db`. Until it exists there is
no way to *populate* this schema outside of tests.

Use the `/schema` command to change any of it — migrations are immutable once applied and the
runner enforces that by content hash.

**WAL is the opener's responsibility.** The build sets `SQLITE_DEFAULT_WAL_SYNCHRONOUS=1`,
which selects the synchronous level *within* WAL mode — it does not select the journal mode.
§5 requires WAL, so whoever opens a connection must issue `PRAGMA journal_mode=WAL`. The
migration runner deliberately does not: it must work against `:memory:` in tests, where WAL
is unavailable.

## The shape of it

Two databases, and confusing them is the mistake to avoid:

| DB | Contents | Runtime access |
|---|---|---|
| `world.db` | compiled data pack — nations, competitions, clubs, persons, rules, assets | **read-only** |
| `save.db` | one save game — deltas, history, calendar, RNG state, career | read/write |

`world.db` is a *build artefact* of `/tools/packc`. Humans never edit it; modders edit the
json5/csv sources under `/data/packs/`. Both are gitignored.

## Non-obvious constraints

- **SQLite is not the runtime query engine.** On load the server materialises the pack into
  cache-friendly SoA arrays. Nothing in the sim loop touches SQL — a query inside a tick is
  a §3 violation, not a performance note.
- **No ORM.** Hand-written statements, prepared once.
- Every table: `id INTEGER PRIMARY KEY`, `uid TEXT UNIQUE NOT NULL`, `pack TEXT NOT NULL`.
  `STRICT`, foreign keys on.
- Migrations are forward-only and numbered, and each one ships a test that loads the
  previous version's save and asserts a hashed world summary is unchanged.
- Layered packs resolve by stable string uid (`club.eng.northbridge_utd`), never by row id.
  Conflicts are reported with pack names, never silently resolved.
