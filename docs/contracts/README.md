# Contracts

Four versioned contracts (§0.3). Changing any one without a migration, a version bump and a
compatibility test is a build-breaking offence.

| Contract | Source of truth | Current version | State |
|---|---|---|---|
| IPC | [`ipc.schema.json`](ipc.schema.json) | 0 | declared, no endpoints (server lands M3) |
| DB schema | [`db.md`](db.md) + `/core/db/schema/*.sql` | — | not started (M1) |
| Match event stream | [`events.md`](events.md) | 0 | not started (M2) |
| Save format | [`save.md`](save.md) | — | not started (M3) |

## The rule

These files are the source of truth, and the code is generated from or checked against them
— not the other way round. The IPC contract already works this way: `tools/ipcgen` writes
`/app/ipc/src/generated.ts`, and `pnpm ipc:check` fails CI if the two disagree.

## Which command to use

| Changing | Command |
|---|---|
| a database entity | `/schema` |
| the server↔UI boundary | `/api` |
| match event types | `/match` |
| anything structural | `/adr` first |
