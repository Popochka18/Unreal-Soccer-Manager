---
description: Change the IPC contract between server and UI.
argument-hint: <endpoint or stream name>
allowed-tools: Read, Write, Edit, Bash
---
Contract change: $ARGUMENTS

1. Edit /docs/contracts/ipc.schema.json first — it is the source of truth.
2. Regenerate /app/ipc (typed client) and the server-side handlers. Both are generated;
   hand-edits are a defect.
3. Bump the contract version; add a compatibility note; update the version-negotiation test.
4. Payloads: MessagePack, no nested objects deeper than 3, no unbounded arrays without a
   cursor, no endpoint that returns more than 2 000 rows in one shot.
5. Show me the schema diff and the generated TS types.
