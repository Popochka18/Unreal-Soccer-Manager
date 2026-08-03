---
description: Build or rework a frontend screen.
argument-hint: <screen, e.g. player-profile | tactics | transfer-search>
allowed-tools: Read, Write, Edit, Bash
---
Screen: $ARGUMENTS

Requirements: zero game logic; all data via the generated IPC client; virtualized tables;
full keyboard navigation; loading/empty/error/stale states all designed, not improvised;
theme tokens only, no hardcoded colours or spacing; every attribute cell uses the shared
primitive.

Deliver: component tree, the queries it issues (with expected row counts), the Playwright
journey, and a measured 5 000-row scroll FPS number.

If the screen needs data the server does not expose, stop and run /api first.
