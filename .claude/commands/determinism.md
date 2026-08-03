---
description: Audit and enforce determinism across the codebase.
allowed-tools: Read, Bash, Grep, Edit
---
Scope: $ARGUMENTS (empty = full audit)

1. Grep the sim TUs for: float/double, rand, time, chrono, unordered_ iteration,
   pointer-value comparisons, uninitialised reads, thread usage, static mutable state.
   Print every hit with a verdict: violation | justified (and why).
2. Rebuild goldens on this machine and compare against the committed hashes.
3. Run the cross-OS comparison job if available; otherwise state clearly that it was not run.
4. For the first divergence found, use /tools/replaydiff and report the exact tick, stream
   and call site. Do not guess a cause without the diff.
