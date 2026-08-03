---
description: Hostile self-review of the current diff.
allowed-tools: Read, Bash, Grep
---
Review scope: $ARGUMENTS (empty = git diff against main)

Act as a senior reviewer who wants to reject this change. Check, in order:
CLAUDE.md §3 hard-NOs · determinism · contract/migration completeness · error handling and
failure modes · allocation and copies in hot paths · test quality (do the tests actually
fail if the logic is broken? prove it by mutating one line) · naming and dead code ·
save-compat · modding surface impact.

Output: BLOCKERS / SHOULD-FIX / NITS, with file:line. Then a one-line verdict: SHIP or REJECT.

Be harsh. A review with no findings is a failed review — say what you looked at and why it
was clean.
