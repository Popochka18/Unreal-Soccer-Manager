# Unit tests. Golden-replay and property tests join these at M2 (§11).

FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)

add_executable(pitchsim_tests
    test_fixed.cpp
    test_rng.cpp
    test_wide.cpp
)

target_link_libraries(pitchsim_tests PRIVATE
    PitchForge::pitchsim
    Catch2::Catch2WithMain
)

# Tests get the strict warning bar but not the determinism pinning: they are
# not simulation translation units and never ship.
pitchforge_strict_target(pitchsim_tests)

catch_discover_tests(pitchsim_tests)
