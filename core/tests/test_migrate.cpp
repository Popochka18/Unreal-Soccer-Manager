// Schema and migration-runner tests — CLAUDE.md §5.5.
//
// These run against the real migrations in core/db/schema, not a copy, so a
// migration cannot pass its tests while being wrong on disk.

#include <catch2/catch_test_macros.hpp>

#include "pitchforge/db/migrate.hpp"

#include <sqlite3.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

using pitchforge::db::apply_migrations;
using pitchforge::db::content_hash;
using pitchforge::db::load_migrations;
using pitchforge::db::Migration;
using pitchforge::db::schema_version;
using pitchforge::db::to_hex;

namespace {

// Every test gets a fresh in-memory database: a runner that only works on a
// database somebody else prepared is not actually tested.
class TestDb {
public:
    TestDb()
    {
        if (sqlite3_open(":memory:", &handle_) != SQLITE_OK) {
            throw std::runtime_error("cannot open in-memory database");
        }
    }
    ~TestDb() { sqlite3_close(handle_); }

    TestDb(const TestDb&)            = delete;
    TestDb& operator=(const TestDb&) = delete;
    TestDb(TestDb&&)                 = delete;
    TestDb& operator=(TestDb&&)      = delete;

    [[nodiscard]] sqlite3* get() const noexcept { return handle_; }

private:
    sqlite3* handle_{nullptr};
};

[[nodiscard]] std::filesystem::path schema_dir()
{
    return std::filesystem::path{PITCHFORGE_SCHEMA_DIR};
}

[[nodiscard]] int try_exec(sqlite3* db, const char* sql)
{
    char* err = nullptr;
    const int rc = sqlite3_exec(db, sql, nullptr, nullptr, &err);
    sqlite3_free(err);
    return rc;
}

void must_exec(sqlite3* db, const char* sql)
{
    INFO(sql);
    REQUIRE(try_exec(db, sql) == SQLITE_OK);
}

[[nodiscard]] int row_count(sqlite3* db, const char* sql)
{
    sqlite3_stmt* stmt = nullptr;
    REQUIRE(sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK);
    int rows = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ++rows;
    }
    sqlite3_finalize(stmt);
    return rows;
}

[[nodiscard]] std::string first_text(sqlite3* db, const char* sql)
{
    sqlite3_stmt* stmt = nullptr;
    REQUIRE(sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK);
    std::string value;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        if (const unsigned char* text = sqlite3_column_text(stmt, 0); text != nullptr) {
            value = reinterpret_cast<const char*>(text);
        }
    }
    sqlite3_finalize(stmt);
    return value;
}

// The schema as SQLite itself reports it. Two databases with equal dumps have
// equal schemas.
[[nodiscard]] std::string schema_dump(sqlite3* db)
{
    std::string dump;
    sqlite3_stmt* stmt = nullptr;
    REQUIRE(sqlite3_prepare_v2(db,
                               "SELECT type, name, COALESCE(sql, '') FROM sqlite_schema "
                               "WHERE name NOT LIKE 'sqlite_%' ORDER BY type, name;",
                               -1, &stmt, nullptr) == SQLITE_OK);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        for (int col = 0; col < 3; ++col) {
            if (const unsigned char* text = sqlite3_column_text(stmt, col); text != nullptr) {
                dump += reinterpret_cast<const char*>(text);
            }
            dump += '\x1f';
        }
        dump += '\n';
    }
    sqlite3_finalize(stmt);
    return dump;
}

// A minimal but complete geography, so `person` has something valid to point
// at. Fictional throughout (§14).
void seed_geography(sqlite3* db)
{
    must_exec(db,
              "INSERT INTO strings(uid, pack, en) VALUES"
              " ('str.nation.avalon',     'base', 'Avalon'),"
              " ('str.region.northmarch', 'base', 'Northmarch'),"
              " ('str.city.northbridge',  'base', 'Northbridge');");
    must_exec(db,
              "INSERT INTO nation(id, uid, pack, name_str, code3, reputation, league_strength)"
              " VALUES (1, 'nation.avalon', 'base', 'str.nation.avalon', 'AVL', 7000, 6500);");
    must_exec(db,
              "INSERT INTO region(id, uid, pack, nation_id, name_str)"
              " VALUES (1, 'region.avalon.northmarch', 'base', 1, 'str.region.northmarch');");
    must_exec(db,
              "INSERT INTO city(id, uid, pack, region_id, name_str, population)"
              " VALUES (1, 'city.avalon.northbridge', 'base', 1, 'str.city.northbridge', 412000);");
}

// dob 8035 = 1992-01-30, days since the 1970-01-01 epoch fixed by 002.
constexpr const char* kValidPerson =
    "INSERT INTO person(id, uid, pack, first_name, last_name, dob, nation_id,"
    " birth_city_id, height_cm, preferred_foot)"
    " VALUES (1, 'person.avalon.000001', 'base', 'Alaric', 'Vance', 8035, 1, 1, 183, 0);";

void migrate_fully(sqlite3* db)
{
    apply_migrations(db, load_migrations(schema_dir()));
}

} // namespace

TEST_CASE("migrations load in version order")
{
    const auto migrations = load_migrations(schema_dir());

    REQUIRE(migrations.size() >= 2);
    for (std::size_t i = 1; i < migrations.size(); ++i) {
        REQUIRE(migrations[i - 1].version < migrations[i].version);
    }
    REQUIRE(migrations[0].version == 1U);
    REQUIRE(migrations[0].name == "strings_geography");
    REQUIRE(migrations[1].version == 2U);
    REQUIRE(migrations[1].name == "person");
}

TEST_CASE("content hashing is stable and rendered as 16 hex digits")
{
    REQUIRE(content_hash("") == 14695981039346656037ULL);
    REQUIRE(content_hash("person") == content_hash("person"));
    REQUIRE(content_hash("person") != content_hash("persor"));
    REQUIRE(to_hex(0U).size() == 16U);
    REQUIRE(to_hex(0U) == "0000000000000000");
    REQUIRE(to_hex(0xFFFFFFFFFFFFFFFFULL) == "ffffffffffffffff");
}

TEST_CASE("a fresh database migrates to the newest version")
{
    TestDb db;
    REQUIRE(schema_version(db.get()) == 0U);

    const auto migrations = load_migrations(schema_dir());
    const auto result     = apply_migrations(db.get(), migrations);

    REQUIRE(result.from_version == 0U);
    REQUIRE(result.to_version == migrations.back().version);
    REQUIRE(result.applied.size() == migrations.size());
    REQUIRE(schema_version(db.get()) == migrations.back().version);
}

TEST_CASE("migrating an up-to-date database changes nothing")
{
    TestDb db;
    const auto migrations = load_migrations(schema_dir());

    apply_migrations(db.get(), migrations);
    const std::string before = schema_dump(db.get());

    const auto again = apply_migrations(db.get(), migrations);

    REQUIRE(again.applied.empty());
    REQUIRE(again.from_version == again.to_version);
    REQUIRE(schema_dump(db.get()) == before);
}

// The §5.5 contract asks each migration to ship a test that loads the previous
// version and asserts an unchanged summary. No save format exists until M3, so
// the closest honest equivalent is this: a database upgraded one migration at
// a time must be indistinguishable from one built in a single pass. It is what
// actually breaks when a migration is written as an ALTER that a fresh CREATE
// does not reproduce.
TEST_CASE("an incremental upgrade lands on the same schema as a fresh build")
{
    const auto all = load_migrations(schema_dir());
    REQUIRE(all.size() >= 2);

    TestDb stepwise;
    for (std::size_t count = 1; count <= all.size(); ++count) {
        const std::vector<Migration> prefix(all.begin(),
                                            all.begin() + static_cast<std::ptrdiff_t>(count));
        apply_migrations(stepwise.get(), prefix);
        REQUIRE(schema_version(stepwise.get()) == all[count - 1].version);
    }

    TestDb fresh;
    apply_migrations(fresh.get(), all);

    REQUIRE(schema_dump(stepwise.get()) == schema_dump(fresh.get()));
    REQUIRE(schema_version(stepwise.get()) == schema_version(fresh.get()));
}

TEST_CASE("an applied migration that changed on disk is refused")
{
    TestDb db;
    auto migrations = load_migrations(schema_dir());
    apply_migrations(db.get(), migrations);

    migrations[0].sql += "\n-- edited after the fact\n";
    migrations[0].hash = content_hash(migrations[0].sql);

    REQUIRE_THROWS_AS(apply_migrations(db.get(), migrations), std::runtime_error);
}

TEST_CASE("a migration recorded as applied but missing from disk is refused")
{
    TestDb db;
    const auto all = load_migrations(schema_dir());
    apply_migrations(db.get(), all);

    const std::vector<Migration> without_first(all.begin() + 1, all.end());
    REQUIRE_THROWS_AS(apply_migrations(db.get(), without_first), std::runtime_error);
}

TEST_CASE("a back-filled migration is refused")
{
    TestDb db;
    const auto all = load_migrations(schema_dir());

    // Apply only the newest, so the recorded version outruns migration 1.
    const std::vector<Migration> newest_only(all.end() - 1, all.end());
    apply_migrations(db.get(), newest_only);
    REQUIRE(schema_version(db.get()) == all.back().version);

    // Migration 1 now sits below the current version having never been
    // applied. Running it would silently reorder history.
    REQUIRE_THROWS_AS(apply_migrations(db.get(), all), std::runtime_error);
}

TEST_CASE("the migrated schema accepts a valid world")
{
    TestDb db;
    migrate_fully(db.get());
    seed_geography(db.get());
    must_exec(db.get(), kValidPerson);

    REQUIRE(row_count(db.get(), "PRAGMA foreign_key_check;") == 0);
    REQUIRE(first_text(db.get(), "PRAGMA integrity_check;") == "ok");
    REQUIRE(row_count(db.get(), "SELECT 1 FROM person WHERE common_name = '';") == 1);
}

TEST_CASE("the migrated schema rejects an invalid world")
{
    TestDb db;
    migrate_fully(db.get());
    seed_geography(db.get());

    SECTION("STRICT rejects a value of the wrong type")
    {
        REQUIRE(try_exec(db.get(),
                         "INSERT INTO person(id, uid, pack, first_name, last_name, dob,"
                         " nation_id, birth_city_id, height_cm, preferred_foot)"
                         " VALUES (1, 'person.a.1', 'base', 'A', 'B', 8035, 1, 1,"
                         " 'tall', 0);") != SQLITE_OK);
    }

    SECTION("a foreign key to a nation that does not exist is rejected")
    {
        REQUIRE(try_exec(db.get(),
                         "INSERT INTO person(id, uid, pack, first_name, last_name, dob,"
                         " nation_id, birth_city_id, height_cm, preferred_foot)"
                         " VALUES (1, 'person.a.1', 'base', 'A', 'B', 8035, 999, 1,"
                         " 183, 0);") != SQLITE_OK);
    }

    SECTION("a uid without its entity prefix is rejected")
    {
        REQUIRE(try_exec(db.get(),
                         "INSERT INTO person(id, uid, pack, first_name, last_name, dob,"
                         " nation_id, birth_city_id, height_cm, preferred_foot)"
                         " VALUES (1, 'club.a.1', 'base', 'A', 'B', 8035, 1, 1,"
                         " 183, 0);") != SQLITE_OK);
    }

    SECTION("an out-of-range height is rejected")
    {
        REQUIRE(try_exec(db.get(),
                         "INSERT INTO person(id, uid, pack, first_name, last_name, dob,"
                         " nation_id, birth_city_id, height_cm, preferred_foot)"
                         " VALUES (1, 'person.a.1', 'base', 'A', 'B', 8035, 1, 1,"
                         " 300, 0);") != SQLITE_OK);
    }

    SECTION("an unknown preferred foot is rejected")
    {
        REQUIRE(try_exec(db.get(),
                         "INSERT INTO person(id, uid, pack, first_name, last_name, dob,"
                         " nation_id, birth_city_id, height_cm, preferred_foot)"
                         " VALUES (1, 'person.a.1', 'base', 'A', 'B', 8035, 1, 1,"
                         " 183, 5);") != SQLITE_OK);
    }

    SECTION("an empty name is rejected")
    {
        REQUIRE(try_exec(db.get(),
                         "INSERT INTO person(id, uid, pack, first_name, last_name, dob,"
                         " nation_id, birth_city_id, height_cm, preferred_foot)"
                         " VALUES (1, 'person.a.1', 'base', '', 'B', 8035, 1, 1,"
                         " 183, 0);") != SQLITE_OK);
    }

    SECTION("a second nationality equal to the first is rejected")
    {
        REQUIRE(try_exec(db.get(),
                         "INSERT INTO person(id, uid, pack, first_name, last_name, dob,"
                         " nation_id, second_nation_id, birth_city_id, height_cm,"
                         " preferred_foot)"
                         " VALUES (1, 'person.a.1', 'base', 'A', 'B', 8035, 1, 1, 1,"
                         " 183, 0);") != SQLITE_OK);
    }

    SECTION("a duplicate uid is rejected")
    {
        must_exec(db.get(), kValidPerson);
        REQUIRE(try_exec(db.get(),
                         "INSERT INTO person(id, uid, pack, first_name, last_name, dob,"
                         " nation_id, birth_city_id, height_cm, preferred_foot)"
                         " VALUES (2, 'person.avalon.000001', 'base', 'C', 'D', 8035, 1,"
                         " 1, 183, 0);") != SQLITE_OK);
    }

    SECTION("a display name pointing at no string row is rejected")
    {
        REQUIRE(try_exec(db.get(),
                         "INSERT INTO nation(id, uid, pack, name_str, code3, reputation,"
                         " league_strength)"
                         " VALUES (2, 'nation.borea', 'base', 'str.nation.missing', 'BOR',"
                         " 5000, 4000);") != SQLITE_OK);
    }

    SECTION("a string uid outside the str.* namespace is rejected")
    {
        REQUIRE(try_exec(db.get(),
                         "INSERT INTO strings(uid, pack, en)"
                         " VALUES ('nation.borea', 'base', 'Borea');") != SQLITE_OK);
    }
}
