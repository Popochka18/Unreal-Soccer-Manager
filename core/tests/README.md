# core/tests

Catch2 unit tests for the native core. §11 is the Definition of Done.

## Layout

| Location | Contents |
|---|---|
| `core/tests/` | unit and property tests, compiled into `pitchsim_tests` |
| `/tests/golden/` | ~200 recorded matches: input blob + blake3 of the event stream |
| `/tests/calibration/` | statistical bands the soak and the 10k-match harness assert against |
| `/tests/fixtures/` | shared test data — the only legitimate home for hardcoded test content |

The split is deliberate and is recorded in ADR-0004: `core/tests/` holds *code*, top-level
`/tests/` holds *data* that the tools under `/tools/` also consume.

## Running

```bash
cmake --workflow --preset ci      # release build + full suite
cmake --workflow --preset asan
cmake --workflow --preset ubsan
```

## What a good test looks like here

The `/review` command asks whether a test would actually fail if the logic broke, and
demands you prove it by mutating a line. Two current tests are written to that standard and
are worth copying:

- `SplitMix64 matches the reference vectors` — pins the generator against published output.
  If it fails, every golden replay in the repo is invalid; the fix is never to update the
  constants.
- `derive is independent of root consumption` — encodes the §6 invariant that adding a call
  site to one RNG stream must not shift another. A plausible-looking refactor of `derive()`
  breaks this and nothing else.

Tests get the strict warning bar but not the determinism compile pinning: they are not
simulation translation units and never ship.
