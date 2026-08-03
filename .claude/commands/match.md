---
description: Work on libpitchsim — the deterministic match engine.
argument-hint: <feature, e.g. offside line, pressing triggers, set pieces>
allowed-tools: Read, Write, Edit, Bash
---
Feature: $ARGUMENTS

Hard constraints: no UE includes, no float, no allocation in the tick loop, no new RNG call
site that shifts existing streams (derive a new stream instead).

Deliver: implementation + new event types documented in /docs/contracts/events.md + unit
tests + the effect on golden replays (expected: unchanged unless the feature is meant to
change outcomes — if it changes them, regenerate goldens in a separate commit).

Then run: 10 000-match statistical harness and report goals/game, shots, xG-equivalent,
pass completion, fouls, cards per tier vs the bands in /tests/calibration/bands.json5.
