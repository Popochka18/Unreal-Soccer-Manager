---
description: Implement or refactor a world-simulation subsystem (non-match).
argument-hint: <subsystem: transfers|finance|training|ai-manager|youth|media|calendar>
allowed-tools: Read, Write, Edit, Bash
---
Subsystem: $ARGUMENTS

Rules: fixed-point only, no globals, RNG injected with a dedicated stream, all tunables
loaded from DB, all outputs explainable via a `--explain` path.

Order of work: (1) read existing code + tests, (2) write the property tests and the
calibration expectations, (3) implement, (4) run the 10-season soak, (5) paste the
calibration bands before/after.

If the subsystem needs a new tunable, add it to the DB and to the modding docs — not to a
constant in the source.
