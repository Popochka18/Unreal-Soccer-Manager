---
description: Generate or extend the fictional base data pack.
argument-hint: <scope, e.g. nation:northland, or competition pyramid>
allowed-tools: Read, Write, Edit, Bash
---
Scope: $ARGUMENTS

Generate plausible fictional content only (§14). Requirements:

- Names generated from per-nation phoneme/name-part tables in /data/packs/base/naming/,
  never from a hardcoded list in code.
- Realistic distributions: age pyramid, attribute distribution per tier, wage/revenue
  ratios, squad sizes, contract expiry spread, youth intake pipeline.
- Every club gets: colours, stadium with tier-appropriate capacity, finances, staff,
  a full squad with position coverage, and a crest seed.
- Run `packc` + validation + a 3-season headless soak and paste the resulting league
  tables and the calibration report.
