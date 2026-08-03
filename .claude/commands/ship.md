---
description: Milestone gate — verify a milestone is genuinely complete.
argument-hint: <M0..M8>
allowed-tools: Read, Bash, Grep
---
Milestone: $ARGUMENTS

Go through §12 and the §11 Definition of Done line by line. For each item output:
PASS (with the command run and its output) / FAIL (with what is missing) / NOT-VERIFIED
(with why). Do not mark anything PASS that you did not execute in this session.

Finish with: a single verdict, the top three risks carried into the next milestone, and the
updated /docs/status.md.
