# Match event stream contract

**Version 0. Not yet implemented — lands at M2.**

Defined in CLAUDE.md §7. This file will hold the full event type table; what follows is the
shape that everything else is being built against, recorded now because three consumers
depend on it.

## Why this is the most important contract in the project

The event stream is the **single source of truth** for commentary, the 2D view, the UE5 3D
view, statistics, highlights, player ratings and post-match analysis. Nothing recomputes
anything from anything else.

That is what makes three synchronised views possible. It is also the constraint that is
easiest to break by accident: the first time a consumer computes something itself — a shot
count, a possession percentage, an expected-goals figure — the views start to disagree and
there is no single place to fix it.

## Shape

An append-only, versioned sequence of:

```
tick, type, actors[], position, payload
```

Two granularities in one stream:

- **micro** — position snapshots at 5 Hz, quantized. Consumers interpolate between them and
  never extrapolate gameplay.
- **macro** — pass, tackle, foul, shot, goal, substitution, injury, card.

## Rules that will apply from M2

- Append-only. An event that has been emitted is never revised.
- `kEventStreamVersion` in `core/pitchsim/include/pitchsim/version.hpp` is the negotiated
  version. A bump needs a compatibility note here.
- The stream is a pure function of the match input tuple (§6): seed, both tactic snapshots,
  both roster snapshots, pitch/weather state, ruleset version.
- `simulate_full()` and `simulate_fast()` emit the same event *types*. Fast mode emits fewer
  micro events; it must never emit a macro event that full mode could not.

## Consumer obligations

Documented per-consumer at M7, but the rule is fixed now: a consumer that receives events
late, out of order, or truncated mid-match must degrade visibly rather than invent state.
The renderer crashing must not interrupt or corrupt a save.
