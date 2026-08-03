# ADR-0003: Third-party dependencies fetched by URL and pinned by SHA256

- **Status:** Accepted
- **Date:** 2026-08-03
- **Amends:** clarifies CLAUDE.md §3 (committed binary size) for third-party source

## Context

The native core needs two third-party dependencies at M0: the SQLite amalgamation (§5) and
Catch2 (§11). More will follow — MessagePack, BLAKE3, an HTTP/WebSocket layer.

Two CLAUDE.md rules bear on how they enter the tree. §3 bans committed binaries over 1 MB.
§6 requires that the same input produce byte-identical output on every machine, forever —
which implies the *build* must also be reproducible, since a different SQLite would be a
different program.

`sqlite3.c` is about 9 MB of C source. It is not a binary, so §3's letter permits committing
it; its spirit clearly does not.

## Options considered

### 1. Vendor the sources into the repository

Copy `sqlite3.c` and Catch2 into `/core/db/third_party/` and commit them.

**Cost:** ~10 MB of third-party source in every clone, growing with each dependency, and
polluting every `git log`, `grep` and diff of the tree. Upgrades become large unreviewable
commits. The one real benefit — offline builds — is genuine and is what makes this option
tempting.

### 2. A package manager (vcpkg, Conan)

**Cost:** a second build system to learn, keep in sync with CMake, and reproduce in CI on
three platforms. For two dependencies this is heavy machinery, and it moves the pinning
decision into a manifest format whose resolution semantics we would then have to trust for a
determinism-critical build. §3's "no reflection-based magic, no dependency-injection
framework" is the same instinct applied elsewhere.

### 3. `FetchContent` with floating git tags

`GIT_REPOSITORY` + `GIT_TAG v3.8.1`.

**Cost:** tags are mutable. A retagged upstream release silently changes what we build, and
we would not find out from a hash mismatch — we would find out from a golden replay failing
for no apparent reason, which is the most expensive way to learn it.

### 4. `FetchContent` with URL + SHA256 *(chosen)*

Download a release archive; CMake verifies the hash before extracting.

**Cost:** a clean build needs network access, and CI cannot build offline without a cache.
Upstream can remove an archive and break old commits. Both are real; neither is silent, and
that is the deciding property.

## Decision

Every third-party dependency is declared in `cmake/Dependencies.cmake` with `URL` and
`URL_HASH SHA256=…`. No vendored copies, no floating tags, no package manager.

The hashes committed at M0 were computed locally against the upstream archives on
2026-08-03:

| Dependency | Version | SHA256 |
|---|---|---|
| Catch2 | v3.8.1 | `18b3f70ac80fccc340d8c6ff0f339b2ae64944782f8d2fca2bd705cf47cadb79` |
| SQLite amalgamation | 3.50.2 | `387991de2834b5da2894119ff4173a9ea0779ea55ebcf53d9a40b24d1dc2484e` |

To bump a dependency: download, `sha256sum`, update both the URL and the hash in one commit.
A commit that changes the URL without the hash is a defect.

Third-party targets are built with warnings suppressed (`-w`). §11 requires a clean warning
log, and the SQLite amalgamation emits `-Wstringop-overread` diagnostics under `-O3` that we
will not patch. Suppressing them at the third-party target keeps our own warning bar at
`-Werror` and prevents third-party noise from hiding a real diagnostic of ours.

## Consequences

**Easier:** upgrades are two-line diffs. The tree stays ours. A tampered or substituted
archive fails at configure time with a hash mismatch rather than producing a subtly
different binary.

**Harder:** no offline clean build. CI needs either network access or a warmed
`FETCHCONTENT_BASE_DIR` cache — the GitHub Actions workflow caches `build/*/\_deps` for this
reason. If an upstream archive disappears, an old commit stops building until someone
repoints the URL.

**Deliberately unresolved:** we do not mirror upstream archives. If dependency availability
becomes a real problem rather than a theoretical one, mirroring to our own storage is the
answer, and it does not change this ADR — only the URLs.

## Rollback path

Straightforward. Vendoring is always available: download the pinned archive, commit its
contents, replace the `FetchContent_Declare` with `add_subdirectory`. The hashes recorded
above identify exactly which bytes to vendor.

## Verification

`cmake --workflow --preset ci` on a clean tree downloads both archives, verifies both
hashes, and builds. A deliberately corrupted hash fails at configure time before any
compilation.
