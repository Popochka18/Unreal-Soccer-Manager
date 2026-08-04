#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

struct sqlite3;

namespace pitchforge::db {

// One forward-only migration, loaded from core/db/schema/NNN_name.sql (§5.5).
struct Migration {
    std::uint32_t version{0};
    std::string   name;
    std::string   sql;
    std::uint64_t hash{0};
};

struct MigrateResult {
    std::uint32_t              from_version{0};
    std::uint32_t              to_version{0};
    std::vector<std::uint32_t> applied;
};

// FNV-1a 64 over a byte sequence. Deterministic and endianness-independent.
//
// This is tamper *detection*, not authentication: it answers "has this file
// changed since it was applied", which is what forward-only migrations need.
// It is not a cryptographic guarantee and must not be relied on as one.
[[nodiscard]] std::uint64_t content_hash(std::string_view bytes) noexcept;

// Renders a hash as 16 lowercase hex digits. Stored as TEXT rather than
// INTEGER so that a 64-bit value with the top bit set does not have to be
// laundered through SQLite's signed integers.
[[nodiscard]] std::string to_hex(std::uint64_t value);

// Loads every NNN_*.sql in `dir`, sorted ascending by version.
//
// The sort is load-bearing: std::filesystem directory iteration order is
// unspecified, and applying migrations in filesystem order would make the
// resulting schema depend on the host (§6).
//
// Throws std::runtime_error on a malformed filename, a version of 0 (reserved
// for "nothing applied"), or two files claiming the same version.
[[nodiscard]] std::vector<Migration> load_migrations(const std::filesystem::path& dir);

// Highest applied migration version, or 0 if none. Does not create anything.
[[nodiscard]] std::uint32_t schema_version(sqlite3* db);

// Applies every migration newer than the recorded version, each inside its own
// transaction, and records it in `schema_migration`.
//
// Throws, without applying anything, if:
//   - an already-applied migration's content hash no longer matches its file
//     (an applied migration was edited — forbidden by §5.5),
//   - a recorded migration's file has disappeared,
//   - a migration file exists at or below the current version but was never
//     applied (a back-filled migration, which would silently reorder history).
MigrateResult apply_migrations(sqlite3* db, const std::vector<Migration>& migrations);

} // namespace pitchforge::db
