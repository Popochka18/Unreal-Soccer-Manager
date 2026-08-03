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

# The hand-written query layer (no ORM, §3) and the migration runner land at
# M1 alongside the schema in ./schema and the pack compiler in /tools/packc.
