---
description: Bug triage — reproduce, then fix, never the reverse.
argument-hint: <bug description or issue id>
allowed-tools: Read, Write, Edit, Bash, Grep
---
Bug: $ARGUMENTS

1. Reproduce. If it is non-deterministic behaviour in the sim, that is itself the bug —
   escalate to /determinism.
2. Write a failing test that captures it. Paste the failure output.
3. Diagnose the root cause. State it in one sentence. If you cannot, say so and list the
   next three experiments instead of guessing at a fix.
4. Fix minimally. No refactoring in a bugfix commit.
5. Re-run: the new test, the golden replays, and the soak if the fix touches the sim.
6. Note in the PR body whether existing saves are affected.
