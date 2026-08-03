---
description: Work on the crest / face / kit asset pipeline.
argument-hint: <crests|faces|kits|store|fallbacks>
allowed-tools: Read, Write, Edit, Bash
---
Target: $ARGUMENTS

Constraints from §10: content-addressed store, deterministic generation seeded by entity
uid, facepack overrides always win, procedural fallback for everything, off-main-thread,
LRU cache, licence field mandatory.

Deliver: the generator/pipeline code, a contact sheet of 200 generated samples grouped by
nationality and age bucket for me to eyeball, cache hit-rate and timing numbers, and the
UE5 material-parameter mapping if faces or kits are touched.
