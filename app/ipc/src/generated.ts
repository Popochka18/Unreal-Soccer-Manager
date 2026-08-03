// GENERATED FILE — DO NOT EDIT.
//
// Source:    docs/contracts/ipc.schema.json
// Generator: tools/ipcgen/generate.mjs
//
// Hand-edits are a defect (§9). Change the schema and regenerate:
//     pnpm ipc:generate

/** Wire contract version. A client and server that disagree must not talk. */
export const CONTRACT_VERSION = 0;

/** Transport limits the server guarantees and the client may rely on. */
export const TRANSPORT_LIMITS = {
  maxRowsPerResponse: 2000,
  maxObjectDepth: 3,
} as const;

/** Every command/query endpoint declared by the contract. */
export type EndpointName = never;

/** Every server-push stream declared by the contract. */
export type StreamName = never;

/** Request and response shapes, keyed by endpoint. */
export interface EndpointContracts extends Record<EndpointName, { request: unknown; response: unknown }> {}

/** Payload shape for each stream. */
export interface StreamContracts extends Record<StreamName, { payload: unknown }> {}
