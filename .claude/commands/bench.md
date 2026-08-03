---
description: Measure, then optimize. Never the other way round.
argument-hint: <target: fast-sim | game-week | load | ui-table | facegen>
allowed-tools: Read, Bash, Edit
---
Target: $ARGUMENTS

1. Measure the current state and paste raw numbers (median, p95, machine, build type).
2. Profile. Name the actual bottleneck with evidence (perf/VTune/Tracy/Chrome profile).
3. Only then propose optimizations, ranked by benefit/risk. Determinism must be preserved —
   state explicitly for each change whether it can alter the event stream.
4. Re-measure. Paste before/after. If the win is under 10%, revert it and say so.
