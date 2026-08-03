# libpitchsim — the deterministic match engine (CLAUDE.md §7).
#
# Pure C++20. STL only. No Unreal, no SQLite, no I/O, no threads. Both the
# headless server and the UE5 plugin link this same static library, which is
# the whole point of §2: the football is in the library, the pixels are in
# Unreal.

add_library(pitchsim STATIC
    src/version.cpp
)

add_library(PitchForge::pitchsim ALIAS pitchsim)

target_include_directories(pitchsim PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Determinism flags apply here and nowhere else matters as much.
pitchforge_sim_target(pitchsim)

# Guard the §3 hard-NO at link time as well as in CI: if anyone adds a
# dependency to this target, it shows up here in review.
set_target_properties(pitchsim PROPERTIES
    EXPORT_NAME pitchsim
    LINKER_LANGUAGE CXX
)
