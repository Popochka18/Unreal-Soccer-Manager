---
description: Scaffold or repair the canonical repo layout, build system and CI.
allowed-tools: Bash, Read, Write, Edit, Glob
---
Target: $ARGUMENTS (empty = everything)

1. Compare the working tree against §4 of CLAUDE.md. Print a table: path | exists | correct.
2. Create only what is missing. Never overwrite an existing file without showing me a diff.
3. Build system: CMake ≥3.27 presets for core (debug/release/asan/ubsan), pnpm workspace
   for /app, UE5 project referencing /core/pitchsim as an external static lib.
4. CI: build all three targets on linux+win+mac, run unit tests, golden replays,
   determinism cross-OS hash comparison, TS typecheck/lint, pack validation.
5. Write /docs/adr/0001-architecture.md capturing §2 as an accepted decision.

Finish with the exact commands I must run to verify.
