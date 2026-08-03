# Calibration bands

Empty until M2. Will hold `bands.json5`: the statistical envelopes that the 10 000-match
harness and the multi-season soak assert against.

Per league tier: goals/game, shots, xG-equivalent, pass completion, fouls, cards, age
pyramid, wage/revenue ratio, transfer volume, injury rate, youth intake quality.

These are what separate "the simulation runs" from "the simulation is right". A soak that
crashes is easy to spot; a league that quietly gets stronger every season for twenty years is
not. **Monotonic drift over a 20-season soak is a bug, not a feature** — the bands are how it
gets caught.
