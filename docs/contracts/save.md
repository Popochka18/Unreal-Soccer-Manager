# Save format contract

**Not yet implemented — lands at M3.**

Defined by CLAUDE.md §5 and §6.

## The promise

Load → continue → **the future is identical**. Not statistically similar: identical, event
for event.

That is only achievable if the save captures every input to the simulation, which in
practice means one thing people forget:

**Every RNG stream's state is persisted.** Not just a master seed — the state of each
derived stream at the moment of the save. A save that stores only the seed will replay
correctly from the start of a season and diverge from any mid-season save point.

## What a save contains

- calendar position and season state
- world deltas against `world.db` (a save never duplicates the read-only pack)
- history: results, transfers, injuries, career records
- the player's career state
- RNG state per stream

## Compatibility

Forward-only, numbered migrations. Each ships a test that loads a previous version's save
and asserts a hashed world summary is unchanged (§5.5). A save that cannot be migrated must
fail loudly at load — never partially load, never silently reset a subsystem.

## The renderer must not be able to break this

The UE5 process is a child process that consumes an event stream (§8). It never owns the
save, never writes to it, and its crash must leave the save untouched.
