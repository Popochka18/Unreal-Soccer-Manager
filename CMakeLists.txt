cmake_minimum_required(VERSION 3.27)

project(PitchForge
    VERSION 0.0.0
    DESCRIPTION "PITCHFORGE — football management simulation"
    LANGUAGES CXX C   # C is for the SQLite amalgamation only.
)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not allowed. Use a preset: cmake --preset debug")
endif()

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(Determinism)
include(Dependencies)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PITCHFORGE_BUILD_TESTS "Build the Catch2 test suite" ON)

# §7: the match engine. Everything else may depend on this; it depends on
# nothing.
add_subdirectory(core/pitchsim)

# §5: SQLite data layer. Loading and persistence only — never in the sim loop.
add_subdirectory(core/db)

# Land at M3 (§12). Present as documented placeholders so the layout in §4 is
# real on disk rather than aspirational.
add_subdirectory(core/world)
add_subdirectory(core/server)

if(PITCHFORGE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(core/tests)
endif()

message(STATUS "PitchForge ${PROJECT_VERSION} | ${CMAKE_BUILD_TYPE} | ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
