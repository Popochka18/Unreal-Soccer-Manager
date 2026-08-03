# core/server — headless game server

The native binary that owns the world. HTTP + WebSocket on loopback, MessagePack payloads,
save/load, DB access, job scheduler. No UI. No engine dependencies.

## Current state (M0)

Empty placeholder. Lands at M3 (§12).

## Boundaries

- The server **computes**; the UI formats. If the frontend needs a number, it asks for it
  (§3). The UI may sort, filter and display. It may not decide anything.
- The IPC surface is generated from `/docs/contracts/ipc.schema.json` — both the TS client
  in `/app/ipc` and the server-side handlers. Hand-editing either is a defect. Use `/api`.
- The UE5 renderer is a *child process*, launched on demand. It must never be required for
  a headless season, never be launched at startup, and its crash must never corrupt a save.
- Parallelism happens across matches, never inside one (§6).

## Payload rules that exist for a reason

No endpoint returns more than 2 000 rows in one shot; no unbounded array without a cursor;
no object nested deeper than 3. The UI renders 5 000-row tables at 60 fps by *virtualising*,
which only works if the server pages.
