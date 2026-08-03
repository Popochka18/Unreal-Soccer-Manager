# Dependencies.cmake — ADR-0003.
#
# Every third-party dependency is fetched by URL and pinned by SHA256. No
# vendored copies (keeps §3's binary-size rule honest), no floating git tags,
# no package manager. A build that resolves a different byte sequence than the
# hash below fails at configure time rather than producing a different binary.
#
# Hashes below were computed locally against the upstream archives on
# 2026-08-03. To bump: download, `sha256sum`, update both fields in one commit.

include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

# ---------------------------------------------------------------------------
# Catch2 — test framework. Not linked into shipping binaries.
# ---------------------------------------------------------------------------
FetchContent_Declare(Catch2
    URL      https://github.com/catchorg/Catch2/archive/refs/tags/v3.8.1.tar.gz
    URL_HASH SHA256=18b3f70ac80fccc340d8c6ff0f339b2ae64944782f8d2fca2bd705cf47cadb79
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# ---------------------------------------------------------------------------
# SQLite amalgamation — §5. Compiled in, WAL. Loading/persistence only; never
# touched from the sim loop.
# ---------------------------------------------------------------------------
FetchContent_Declare(sqlite3
    URL      https://sqlite.org/2025/sqlite-amalgamation-3500200.zip
    URL_HASH SHA256=387991de2834b5da2894119ff4173a9ea0779ea55ebcf53d9a40b24d1dc2484e
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
