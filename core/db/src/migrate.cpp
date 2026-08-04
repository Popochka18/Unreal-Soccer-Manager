// Migration runner — CLAUDE.md §5.5.
//
// Not a simulation translation unit: this runs at load and at pack-compile
// time, never in a tick. It may allocate and may touch the filesystem.

#include "pitchforge/db/migrate.hpp"

#include <sqlite3.h>

#include <algorithm>
#include <charconv>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace pitchforge::db {
namespace {

[[noreturn]] void fail(sqlite3* db, std::string message)
{
    if (db != nullptr) {
        message += ": ";
        message += sqlite3_errmsg(db);
    }
    throw std::runtime_error(std::move(message));
}

void exec(sqlite3* db, const char* sql)
{
    char* err = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
        std::string message{"sql failed: "};
        message += (err != nullptr) ? err : "(no message)";
        sqlite3_free(err);
        throw std::runtime_error(message);
    }
}

// Used only on the unwind path, where throwing again would terminate.
void exec_quiet(sqlite3* db, const char* sql) noexcept
{
    sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
}

// Infrastructure rather than a pack entity, so deliberately no uid/pack
// columns. This is the single documented exception to the §5.3 invariants.
void ensure_meta_table(sqlite3* db)
{
    exec(db,
         "CREATE TABLE IF NOT EXISTS schema_migration ("
         "  version INTEGER PRIMARY KEY,"
         "  name    TEXT NOT NULL,"
         "  hash    TEXT NOT NULL"
         ") STRICT;");
}

[[nodiscard]] bool has_meta_table(sqlite3* db)
{
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(
            db,
            "SELECT 1 FROM sqlite_schema WHERE type = 'table' AND name = 'schema_migration';",
            -1, &stmt, nullptr) != SQLITE_OK) {
        fail(db, "prepare schema_migration probe");
    }
    const bool present = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return present;
}

struct AppliedRow {
    std::uint32_t version{0};
    std::string   hash;
};

// A std::vector rather than a hash map: the set is tiny, and an unordered
// container would put a host-dependent iteration order inside a code path that
// decides what gets applied (§6, and §3's ban on unordered iteration).
[[nodiscard]] std::vector<AppliedRow> read_applied(sqlite3* db)
{
    std::vector<AppliedRow> rows;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db,
                           "SELECT version, hash FROM schema_migration ORDER BY version;",
                           -1, &stmt, nullptr) != SQLITE_OK) {
        fail(db, "prepare schema_migration read");
    }

    for (;;) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_DONE) {
            break;
        }
        if (rc != SQLITE_ROW) {
            sqlite3_finalize(stmt);
            fail(db, "read schema_migration");
        }

        AppliedRow row;
        row.version = static_cast<std::uint32_t>(sqlite3_column_int64(stmt, 0));
        if (const unsigned char* text = sqlite3_column_text(stmt, 1); text != nullptr) {
            row.hash = reinterpret_cast<const char*>(text);
        }
        rows.push_back(std::move(row));
    }

    sqlite3_finalize(stmt);
    return rows;
}

void record_applied(sqlite3* db, const Migration& migration)
{
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(
            db, "INSERT INTO schema_migration(version, name, hash) VALUES (?, ?, ?);",
            -1, &stmt, nullptr) != SQLITE_OK) {
        fail(db, "prepare schema_migration insert");
    }

    const std::string hash = to_hex(migration.hash);
    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(migration.version));
    sqlite3_bind_text(stmt, 2, migration.name.c_str(),
                      static_cast<int>(migration.name.size()), SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, hash.c_str(), static_cast<int>(hash.size()), SQLITE_TRANSIENT);

    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        fail(db, "record migration " + std::to_string(migration.version));
    }
}

[[nodiscard]] std::string read_file(const std::filesystem::path& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("cannot read migration: " + path.string());
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

} // namespace

std::uint64_t content_hash(std::string_view bytes) noexcept
{
    constexpr std::uint64_t kOffsetBasis = 14695981039346656037ULL;
    constexpr std::uint64_t kPrime       = 1099511628211ULL;

    std::uint64_t hash = kOffsetBasis;
    for (const char byte : bytes) {
        hash ^= static_cast<std::uint64_t>(static_cast<unsigned char>(byte));
        hash *= kPrime;
    }
    return hash;
}

std::string to_hex(std::uint64_t value)
{
    static constexpr char kDigits[] = "0123456789abcdef";

    std::string out(16, '0');
    for (std::size_t i = 16; i-- > 0;) {
        out[i] = kDigits[static_cast<std::size_t>(value & 0xFU)];
        value >>= 4;
    }
    return out;
}

std::vector<Migration> load_migrations(const std::filesystem::path& dir)
{
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec)) {
        throw std::runtime_error("schema directory not found: " + dir.string());
    }

    std::vector<Migration> migrations;

    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".sql") {
            continue;
        }

        const std::string stem = entry.path().stem().string();
        const std::size_t underscore = stem.find('_');
        if (underscore == std::string::npos || underscore == 0) {
            throw std::runtime_error("migration must be named NNN_name.sql, got: " + stem);
        }

        std::uint32_t version = 0;
        const char* const first = stem.data();
        const char* const last  = stem.data() + underscore;

        const auto parsed = std::from_chars(first, last, version);
        if (parsed.ec != std::errc{} || parsed.ptr != last) {
            throw std::runtime_error("migration version is not a number: " + stem);
        }
        if (version == 0) {
            throw std::runtime_error(
                "migration version 0 is reserved for 'nothing applied': " + stem);
        }

        Migration migration;
        migration.version = version;
        migration.name    = stem.substr(underscore + 1);
        migration.sql     = read_file(entry.path());
        migration.hash    = content_hash(migration.sql);
        migrations.push_back(std::move(migration));
    }

    std::sort(migrations.begin(), migrations.end(),
              [](const Migration& a, const Migration& b) { return a.version < b.version; });

    for (std::size_t i = 1; i < migrations.size(); ++i) {
        if (migrations[i].version == migrations[i - 1].version) {
            throw std::runtime_error("duplicate migration version: " +
                                     std::to_string(migrations[i].version));
        }
    }

    return migrations;
}

std::uint32_t schema_version(sqlite3* db)
{
    if (!has_meta_table(db)) {
        return 0;
    }
    const auto applied = read_applied(db);
    return applied.empty() ? 0U : applied.back().version;
}

MigrateResult apply_migrations(sqlite3* db, const std::vector<Migration>& migrations)
{
    // Cannot run inside a transaction, so it happens before any BEGIN. The
    // build already compiles in SQLITE_DEFAULT_FOREIGN_KEYS=1; this makes the
    // guarantee independent of how the connection was opened.
    exec(db, "PRAGMA foreign_keys = ON;");
    ensure_meta_table(db);

    const auto applied = read_applied(db);

    MigrateResult result;
    result.from_version = applied.empty() ? 0U : applied.back().version;
    result.to_version   = result.from_version;

    // Validate the whole of recorded history before applying anything, so a
    // tampered migration is refused rather than half-honoured.
    for (const AppliedRow& row : applied) {
        const auto it = std::find_if(
            migrations.begin(), migrations.end(),
            [&row](const Migration& m) { return m.version == row.version; });

        if (it == migrations.end()) {
            throw std::runtime_error(
                "migration " + std::to_string(row.version) +
                " is recorded as applied but its file is missing; migrations are "
                "forward-only and immutable (§5.5)");
        }
        if (to_hex(it->hash) != row.hash) {
            throw std::runtime_error(
                "migration " + std::to_string(row.version) + " (" + it->name +
                ") changed after being applied; migrations are immutable (§5.5). "
                "Add a new migration instead.");
        }
    }

    // A file at or below the current version that was never applied would
    // reorder history silently on the next fresh build.
    for (const Migration& migration : migrations) {
        if (migration.version > result.from_version) {
            continue;
        }
        const bool was_applied =
            std::any_of(applied.begin(), applied.end(), [&migration](const AppliedRow& row) {
                return row.version == migration.version;
            });
        if (!was_applied) {
            throw std::runtime_error(
                "migration " + std::to_string(migration.version) + " (" + migration.name +
                ") is older than the current schema version " +
                std::to_string(result.from_version) +
                " but was never applied; migrations are forward-only (§5.5)");
        }
    }

    for (const Migration& migration : migrations) {
        if (migration.version <= result.from_version) {
            continue;
        }

        exec(db, "BEGIN;");
        try {
            exec(db, migration.sql.c_str());
            record_applied(db, migration);
        } catch (...) {
            exec_quiet(db, "ROLLBACK;");
            throw;
        }
        exec(db, "COMMIT;");

        result.applied.push_back(migration.version);
        result.to_version = migration.version;
    }

    return result;
}

} // namespace pitchforge::db
