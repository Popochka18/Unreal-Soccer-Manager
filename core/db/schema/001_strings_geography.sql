-- 001_strings_geography.sql — CLAUDE.md §5.
--
-- The identity floor of the world: localised display text, and the geography
-- that every person and club is anchored to. Nothing above this layer can be
-- expressed without it, which is why it is migration 001 rather than `person`.
--
-- Forward-only (§5.5). Once applied anywhere, this file is immutable: the
-- migration runner records its content hash and refuses to run if it changes.
--
-- Every table here follows the §5.3 invariants — STRICT, foreign keys on,
-- `id` / `uid` / `pack`. The one exception in the database is
-- `schema_migration`, which the runner creates: it is infrastructure rather
-- than a pack entity, so it carries no uid or pack provenance.

-- ---------------------------------------------------------------------------
-- strings — §5.6. No user-facing string in code.
--
-- One column per language, keyed by a `str.*` uid. This is §5.6's stated shape
-- and it is the right one: the obvious alternative, a row per (uid, language),
-- makes `uid` non-unique and so breaks the universal `uid TEXT UNIQUE`
-- invariant that pack layering resolves against (§5.2). Adding a language is
-- therefore a migration, which is correct — languages ship with the project,
-- not with content packs.
--
-- What belongs here: interface text, competition and nation names, position
-- and role labels, commentary templates. What does not: generated proper nouns
-- (person and club names). Those are per-row content, unique to their entity
-- and not translated, and routing 500k persons' names through a per-language
-- table would be absurd. They live as TEXT on the entity — see 002_person.sql.
-- ---------------------------------------------------------------------------
CREATE TABLE strings (
    id   INTEGER PRIMARY KEY,
    uid  TEXT    NOT NULL UNIQUE CHECK (uid LIKE 'str.%'),
    pack TEXT    NOT NULL,
    en   TEXT    NOT NULL
) STRICT;

-- ---------------------------------------------------------------------------
-- nation
--
-- `population` is deliberately absent: nothing consumes it at M1, and a
-- nation's population is the sum of its cities', so storing it here would be
-- denormalised state that can drift. It arrives with youth intake (M8) if that
-- system wants it independently of the city table.
--
-- No latitude/longitude: they would have to be REAL, and §3 bans floats from
-- anything that affects an outcome. When travel distance is modelled, it lands
-- as fixed-point integer micro-degrees.
-- ---------------------------------------------------------------------------
CREATE TABLE nation (
    id              INTEGER PRIMARY KEY,
    uid             TEXT    NOT NULL UNIQUE CHECK (uid LIKE 'nation.%'),
    pack            TEXT    NOT NULL,
    name_str        TEXT    NOT NULL REFERENCES strings(uid),
    code3           TEXT    NOT NULL UNIQUE CHECK (length(code3) = 3),
    reputation      INTEGER NOT NULL CHECK (reputation BETWEEN 0 AND 10000),
    league_strength INTEGER NOT NULL CHECK (league_strength BETWEEN 0 AND 10000)
) STRICT;

CREATE TABLE region (
    id        INTEGER PRIMARY KEY,
    uid       TEXT    NOT NULL UNIQUE CHECK (uid LIKE 'region.%'),
    pack      TEXT    NOT NULL,
    nation_id INTEGER NOT NULL REFERENCES nation(id),
    name_str  TEXT    NOT NULL REFERENCES strings(uid)
) STRICT;

-- `population` is kept on city because it is intrinsic to the entity and the
-- base pack authors it directly; stadium sizing and local interest read it.
CREATE TABLE city (
    id         INTEGER PRIMARY KEY,
    uid        TEXT    NOT NULL UNIQUE CHECK (uid LIKE 'city.%'),
    pack       TEXT    NOT NULL,
    region_id  INTEGER NOT NULL REFERENCES region(id),
    name_str   TEXT    NOT NULL REFERENCES strings(uid),
    population INTEGER NOT NULL CHECK (population >= 0)
) STRICT;

-- No secondary indices in this migration.
--
-- §5.3 forbids an index without an actual query that needs it. Every UNIQUE
-- constraint above already carries one, and that covers the only lookup that
-- exists today: the pack compiler resolving a later layer against an earlier
-- one by uid (§5.2). The loader reads each table once, in full, into SoA
-- arrays (§5.4) — it never filters in SQL — so an index on `region.nation_id`
-- or `city.region_id` would be paid for and never used. They arrive when a
-- query needs them, not before.
