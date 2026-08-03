# @pitchforge/ipc

The single typed client for the server boundary. CLAUDE.md §9.

## Everything in `src/` is generated

`src/generated.ts` is written by `tools/ipcgen/generate.mjs` from
`docs/contracts/ipc.schema.json`. Hand-editing it is a defect. CI runs
`pnpm ipc:check`, which regenerates into memory and fails on any drift — so a hand-edit
does not survive review, it breaks the build.

To change the contract, use the `/api` command. In short: edit the schema first, bump
`contractVersion`, regenerate both this client and the server handlers, update the
version-negotiation test.

```bash
pnpm ipc:generate   # rewrite src/generated.ts
pnpm ipc:check      # verify it matches the schema (what CI runs)
```

`package.json` and `tsconfig.json` are build configuration, not contract surface, and are
maintained by hand.

## Why the generated file is committed

So a fresh clone typechecks without running codegen first. The cost is that it can go
stale, which is exactly what `--check` exists to catch.

## Current state (M0)

The contract declares a version and no endpoints. That is deliberate: the server does not
exist until M3, and inventing endpoint shapes before there is something to serve them
would be committing to a contract we cannot honour.

`/app/ui` may not `fetch()` anything itself — the ESLint config enforces it. All server
traffic goes through this package, because this is the only place that knows the contract
version.
