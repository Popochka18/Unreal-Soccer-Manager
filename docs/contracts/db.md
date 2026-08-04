# Database schema contract

**Schema version 2.** Migrations live in `/core/db/schema/NNN_name.sql` and are applied by
`pitchforge::db::apply_migrations`. Use the `/schema` command to change anything here.

Defined in CLAUDE.md §5. This file documents every table; what follows is the invariant set
that applies to all of them.

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

## Project-wide encodings

**Dates are `INTEGER` days since 1970-01-01, signed.** Every date column, everywhere. This is
exactly `std::chrono::sys_days`, so conversion is a cast rather than arithmetic;
`date(x * 86400, 'unixepoch')` renders one in a SQL shell; and negative values carry pre-1970
dates without a special case. Never a string, never a float, never `time_t`.

**Money is `INTEGER` minor units.** No column that participates in an outcome may be `REAL`
(§3). This also rules out latitude/longitude: when travel distance is modelled it lands as
fixed-point integer micro-degrees, not as a float.

## Where a name lives

Two kinds of text, and confusing them is the mistake to avoid:

| Kind | Home | Why |
|---|---|---|
| Interface text, competition and nation names, position and role labels, commentary templates | `strings` table, `str.*` uid | Curated, bounded, genuinely translated |
| Generated proper nouns — person and club names | `TEXT` on the entity | Per-row content, unique to its entity, not translated |

Routing 500k persons' names through a per-language table would be absurd; routing nation
names around it would break localisation. The line is *translated* versus *generated*.

`strings` has one column per language, which is §5.6's stated shape and the correct one: a
row per `(uid, language)` would make `uid` non-unique and break the layering invariant below.
Adding a language is therefore a migration — correct, because languages ship with the
project, not with content packs.

## Implemented tables

### Migration 001 — `strings`, `nation`, `region`, `city`

Geography and display text: the identity floor that everything else is anchored to.

`nation` deliberately has no `population` — it is the sum of its cities' and would be
denormalised state that can drift. `city.population` is kept because it is intrinsic and the
pack authors it directly.

### Migration 002 — `person`

The shared base for every human: players, staff, referees, later agents and chairmen. Holds
only what is true of a human regardless of role; everything role-specific lives in a sibling
table keyed by `person_id` (`player_attributes`, `staff_attributes`), each in its own
migration.

**There is no `person_type` discriminant.** Roles are established by the *presence* of a row
in the matching attribute table. A single discriminant cannot express a player-manager or a
player who retires into coaching, and a bitmask would drift out of step with the tables it
summarises. "Every player" is a join, executed once at load, never in a tick.

`preferred_foot`: `0` = right, `1` = left, `2` = both.

## Nullable columns, and why

The invariant above forbids a nullable column without a reason recorded here. The complete
list, as of version 2:

| Column | Reason |
|---|---|
| `person.second_nation_id` | The absence of a second nationality is genuine absence, not a value. A sentinel row in `nation` meaning "none" would have to be excluded by hand from every nation query, every eligibility rule and every SoA materialisation. |

## Indices, and why there are none

No table above carries a secondary index. Every `UNIQUE` constraint already provides one, and
that covers the only lookup that exists: the pack compiler resolving a later layer against an
earlier one by uid. The loader reads each table once, in full, into SoA arrays — it never
filters in SQL — so an index on `person.nation_id` or `city.region_id` would be paid for and
never read. They arrive when a query needs them.

## The one exception to the column invariants

`schema_migration` (`version`, `name`, `hash`) carries no `uid` or `pack`. It is
infrastructure created by the migration runner, not a pack entity, and nothing layers over
it. `hash` is FNV-1a 64 of the migration text, stored as 16 hex digits — tamper *detection*,
not authentication: it answers "was an applied migration edited", which is what forward-only
requires. The runner refuses to proceed if an applied migration's text changed, if a recorded
migration's file vanished, or if a migration appears below the current version having never
been applied.

## Entity families still to be specified (§5)

`player_attributes` · `staff_attributes` · `contract` · `club` · `club_finance` · `stadium` ·
`competition` · `competition_rules` · `season` · `fixture` · `match_result` · `match_event` ·
`transfer` · `injury` · `training_schedule` · `tactic` · `media_outlet` · `news_item` ·
`asset`
