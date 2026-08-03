# core/world — season and world simulation

Calendar, fixtures, transfers, finance, contracts, training, player development, AI
managers, youth intake, media. Everything that happens between matches.

## Current state (M0)

Empty placeholder. Lands at M3 (§12). The directory exists so the canonical layout in §4 is
real on disk.

## Read before you write here

Use the `/sim` command. The constraints it enforces are the ones that matter:

- Fixed-point only, no globals, RNG injected with a dedicated stream from `RngStream`.
- **Every tunable comes from the DB.** A wage-inflation coefficient or an age-curve knee
  typed into a `.cpp` is a §3 revert, not a review comment.
- Every subsystem needs an `--explain` path. `/tools/valuation --explain <player>` printing
  a term-by-term breakdown is the standard the rest must meet — a model nobody can audit is
  a model nobody can calibrate.
- §13: the human manager gets no exclusive systems. If the human can do it, AI clubs do it
  through the same functions.

## The thing that will actually bite

Twenty-season soaks fail on *monotonic drift*, not on crashes. Wages that only ever rise,
squads that only ever age, a league that only ever gets stronger. Calibration bands live in
`/tests/calibration/bands.json5` and the soak is what gates this subsystem.
