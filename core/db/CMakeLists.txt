# §5 data layer. SQLite is a loading and persistence format, not the runtime
# query engine — nothing in the sim loop touches SQL.

FetchContent_MakeAvailable(sqlite3)

# The amalgamation ships no CMakeLists, so build it as our own target. Third
# party C: the project warning bar deliberately does not apply.
add_library(sqlite3 STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)

target_include_directories(sqlite3 SYSTEM PUBLIC ${sqlite3_SOURCE_DIR})

# The amalgamation emits a handful of -Wstringop-overread diagnostics under
# -O3 that we cannot fix and will not patch. §11 requires a clean warning log,
# and third-party noise here would mask a real warning of ours.
if(MSVC)
    target_compile_options(sqlite3 PRIVATE /w)
else()
    target_compile_options(sqlite3 PRIVATE -w)
endif()

target_compile_definitions(sqlite3 PUBLIC
    SQLITE_THREADSAFE=1              # the server runs a job scheduler
    SQLITE_DQS=0                     # reject double-quoted string literals; they hide typos
    SQLITE_DEFAULT_FOREIGN_KEYS=1    # §5.3 requires FKs on
    SQLITE_DEFAULT_WAL_SYNCHRONOUS=1
    SQLITE_ENABLE_FTS5               # global entity search (§9)
    SQLITE_OMIT_DEPRECATED
    SQLITE_OMIT_LOAD_EXTENSION       # a save file must not be able to load code
    SQLITE_USE_ALLOCA
)

if(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(sqlite3 PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
endif()

add_library(PitchForge::sqlite3 ALIAS sqlite3)

# ---------------------------------------------------------------------------
# The data layer proper. Not a simulation target: this runs at load and at
# pack-compile time, never inside a tick (§5.4), so it gets the strict warning
# bar but not the determinism pinning.
#
# The hand-written query layer (no ORM, §3) joins this alongside the pack
# compiler in /tools/packc.
# ---------------------------------------------------------------------------
add_library(pitchforge_db STATIC
    src/migrate.cpp
)

add_library(PitchForge::db ALIAS pitchforge_db)

target_include_directories(pitchforge_db PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(pitchforge_db PUBLIC PitchForge::sqlite3)

pitchforge_strict_target(pitchforge_db)

# Where the migrations live. Exported so that tests and /tools/packc resolve
# the same directory rather than each guessing a relative path.
set(PITCHFORGE_SCHEMA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/schema" CACHE INTERNAL
    "Directory holding the forward-only NNN_*.sql migrations")
