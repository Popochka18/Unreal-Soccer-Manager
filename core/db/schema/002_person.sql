-- 002_person.sql — CLAUDE.md §5.
--
-- `person` is the shared base for every human in the world: players, staff,
-- referees, and later agents and chairmen. It holds only what is true of a
-- human regardless of role. Everything role-specific lives in a sibling table
-- keyed by person_id — `player_attributes`, `staff_attributes` — which land in
-- their own migrations.
--
-- Forward-only (§5.5). Immutable once applied.

-- ---------------------------------------------------------------------------
-- There is deliberately no `person_type` discriminant.
--
-- A person's roles are established by the *presence* of a row in the matching
-- attribute table. A single discriminant column cannot express a
-- player-manager, nor a player who retires into coaching without changing
-- identity, and a bitmask would be denormalised state that drifts out of step
-- with the attribute tables it summarises. "Every player" is a join, executed
-- once at load, never in a tick (§5.4).
--
-- Dates are INTEGER days since 1970-01-01, signed. This is the project-wide
-- encoding for every date column: it is exactly std::chrono::sys_days, so
-- conversion is a cast rather than arithmetic; `date(dob * 86400, 'unixepoch')`
-- renders it in a SQL shell; and negative values carry pre-1970 dates without
-- a special case. Never a string, never a float, never time_t.
-- ---------------------------------------------------------------------------
CREATE TABLE person (
    id               INTEGER PRIMARY KEY,
    uid              TEXT    NOT NULL UNIQUE CHECK (uid LIKE 'person.%'),
    pack             TEXT    NOT NULL,

    -- Generated proper nouns, not interface text, so they are TEXT here rather
    -- than `strings` uids (see the note in 001). `common_name` is '' when the
    -- person has none, which keeps the column NOT NULL: an empty string is a
    -- complete answer to "what is this person called informally", so NULL
    -- would add a state the loader must branch on for no gain.
    first_name       TEXT    NOT NULL CHECK (first_name <> ''),
    last_name        TEXT    NOT NULL CHECK (last_name <> ''),
    common_name      TEXT    NOT NULL DEFAULT '',

    dob              INTEGER NOT NULL,

    nation_id        INTEGER NOT NULL REFERENCES nation(id),

    -- The only nullable column in this migration, and the reason is that the
    -- absence of a second nationality is genuine absence rather than a value.
    -- The alternative — a sentinel row in `nation` meaning "none" — would have
    -- to be excluded by hand from every nation query, every competition
    -- eligibility rule and every SoA materialisation. NULL is cheaper and
    -- says what it means.
    second_nation_id INTEGER          REFERENCES nation(id),

    birth_city_id    INTEGER NOT NULL REFERENCES city(id),

    height_cm        INTEGER NOT NULL CHECK (height_cm BETWEEN 140 AND 220),

    -- 0 = right, 1 = left, 2 = both. A structural discriminant rather than a
    -- balance constant, so it is an enum in the schema and mirrored in C++;
    -- §3's "no gameplay constant in source" governs weights and rules, not the
    -- identity of an enumerator.
    preferred_foot   INTEGER NOT NULL CHECK (preferred_foot IN (0, 1, 2)),

    CHECK (second_nation_id IS NULL OR second_nation_id <> nation_id)
) STRICT;

-- No secondary indices, for the reason given at the end of 001: the loader
-- reads `person` once, in full, into SoA arrays. Filtering by nationality, age
-- or club happens over those arrays at runtime, not in SQL, so an index on
-- `nation_id` or `dob` would never be read. `uid`'s UNIQUE index is the only
-- lookup path, and it exists for pack layering.
