#!/usr/bin/env node
// IPC client generator — CLAUDE.md §9.
//
// Reads /docs/contracts/ipc.schema.json and writes /app/ipc/src/generated.ts.
// The output is committed so that a fresh clone typechecks without a codegen
// step, which means it can drift. `--check` regenerates into memory and diffs;
// CI runs it so a hand-edit or a stale commit fails the build rather than
// shipping a client that disagrees with the contract.
//
// Usage:
//   node tools/ipcgen/generate.mjs           # write
//   node tools/ipcgen/generate.mjs --check   # verify, exit 1 on drift

import { mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, resolve } from 'node:path';

const here = dirname(fileURLToPath(import.meta.url));
const repoRoot = resolve(here, '..', '..');
const schemaPath = resolve(repoRoot, 'docs/contracts/ipc.schema.json');
const outputPath = resolve(repoRoot, 'app/ipc/src/generated.ts');

const schema = JSON.parse(readFileSync(schemaPath, 'utf8'));

if (typeof schema.contractVersion !== 'number') {
  throw new Error('ipc.schema.json: contractVersion must be a number.');
}

const endpointNames = Object.keys(schema.endpoints ?? {}).sort();
const streamNames = Object.keys(schema.streams ?? {}).sort();

const unionOrNever = (names) =>
  names.length === 0 ? 'never' : names.map((n) => JSON.stringify(n)).join(' | ');

const render = () => `// GENERATED FILE — DO NOT EDIT.
//
// Source:    docs/contracts/ipc.schema.json
// Generator: tools/ipcgen/generate.mjs
//
// Hand-edits are a defect (§9). Change the schema and regenerate:
//     pnpm ipc:generate

/** Wire contract version. A client and server that disagree must not talk. */
export const CONTRACT_VERSION = ${schema.contractVersion};

/** Transport limits the server guarantees and the client may rely on. */
export const TRANSPORT_LIMITS = {
  maxRowsPerResponse: ${schema.transport?.limits?.maxRowsPerResponse ?? 2000},
  maxObjectDepth: ${schema.transport?.limits?.maxObjectDepth ?? 3},
} as const;

/** Every command/query endpoint declared by the contract. */
export type EndpointName = ${unionOrNever(endpointNames)};

/** Every server-push stream declared by the contract. */
export type StreamName = ${unionOrNever(streamNames)};

/** Request and response shapes, keyed by endpoint. */
export interface EndpointContracts extends Record<EndpointName, { request: unknown; response: unknown }> {${
  endpointNames.length === 0 ? '' : '\n  // populated by /api'
}}

/** Payload shape for each stream. */
export interface StreamContracts extends Record<StreamName, { payload: unknown }> {${
  streamNames.length === 0 ? '' : '\n  // populated by /api'
}}
`;

const rendered = render();

if (process.argv.includes('--check')) {
  let current = '';
  try {
    current = readFileSync(outputPath, 'utf8');
  } catch {
    console.error(`ipcgen: ${outputPath} is missing. Run: pnpm ipc:generate`);
    process.exit(1);
  }
  if (current !== rendered) {
    console.error(
      'ipcgen: app/ipc/src/generated.ts is out of date or was hand-edited.\n' +
        '        Regenerate with: pnpm ipc:generate',
    );
    process.exit(1);
  }
  console.log('ipcgen: generated client is in sync with the contract.');
} else {
  mkdirSync(dirname(outputPath), { recursive: true });
  writeFileSync(outputPath, rendered);
  console.log(
    `ipcgen: wrote app/ipc/src/generated.ts ` +
      `(contract v${schema.contractVersion}, ${endpointNames.length} endpoints, ${streamNames.length} streams)`,
  );
}
