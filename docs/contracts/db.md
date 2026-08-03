# Database schema contract

**Not yet implemented — lands at M1.** Use the `/schema` command.

Defined in CLAUDE.md §5. This file will document every table; what follows is the invariant
set that applies to all of them.

## Two databases

| DB | Contents | Runtime |
|---|---|---|
| `world.db` | compiled data pack: nations, competitions, clubs, persons, rules, assets | **read-only** |
| `save.db` | one save game: deltas, history, calendar, RNG state, career | read/write |

`world.db` is a build artefact of `/tools/packc`. Humans never edit it; modders edit the
json5/csv sources under `/data/packs/`. Both are gitignored.

## Invariants for every table

- `STRICT`, foreign keys on.
- `id INTEGER PRIMARY KEY` — internal, never referenced across pack layers.
- `uid TEXT UNIQUE NOT NULL` — stable, modder-facing (`club.eng.northbridge_utd`). Pack
  layering resolves by this, never by row id.
- `pack TEXT NOT NULL` — provenance, so the compiler can report a conflict by pack name.
- No nullable column without a written reason in this file.
- No index without an actual query that needs it.

## Rules that are easy to violate

**SQLite is not the runtime query engine.** On load, the server materialises the pack into
cache-friendly SoA arrays. Nothing in the sim loop touches SQL — a query inside a tick is a
§3 violation, not a performance note.

**No user-facing string in code** (§5.6). Strings live in the `strings` table keyed by
`str.*` uid with per-language columns. Commentary templates are data with a slot grammar.

**Migrations are forward-only and numbered**, and each ships a test that loads the previous
version's save and asserts a hashed world summary is unchanged.

## Entity families to be specified (§5)

`person` · `player_attributes` · `staff_attributes` · `contract` · `club` · `club_finance` ·
`stadium` · `competition` · `competition_rules` · `season` · `fixture` · `match_result` ·
`match_event` · `nation` · `region` · `city` · `transfer` · `injury` · `training_schedule` ·
`tactic` · `media_outlet` · `news_item` · `asset` · `strings`
