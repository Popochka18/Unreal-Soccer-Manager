# Determinism.cmake — CLAUDE.md §6.
#
# Applies to simulation translation units only. The rule is that a sim TU must
# produce byte-identical output on every OS, compiler and CPU, forever. That
# forbids anything the optimiser is allowed to reassociate or contract.
#
# §3 bans float/double from sim state outright, so the floating-point flags
# below are belt-and-braces: they exist so that an accidentally-introduced float
# cannot silently vary between an FMA-capable and a non-FMA-capable build.
#
# ADR-0005: the supported compilers are GCC and Clang. On Windows that means
# clang-cl, which takes MSVC-style flag spellings but has Clang semantics, and
# needs GCC-style options tunnelled through with the /clang: prefix. MSVC proper
# is unsupported — fixed.hpp #errors on it.

function(_pitchforge_apply_warnings target)
    if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        target_compile_options(${target} PRIVATE /W4 /WX)
        if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            # Real MSVC (ADR-0006). /permissive- and /Zc:__cplusplus are not
            # passed to clang-cl, which already conforms and warns about them.
            target_compile_options(${target} PRIVATE /permissive- /Zc:__cplusplus)
        endif()
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic -Werror
            -Wconversion -Wsign-conversion
            -Wshadow -Wnon-virtual-dtor -Wold-style-cast
            -Wcast-align -Wunused -Woverloaded-virtual
            -Wdouble-promotion -Wfloat-equal
        )
    endif()
endfunction()

function(pitchforge_sim_target target)
    target_compile_features(${target} PUBLIC cxx_std_20)
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
    )

    _pitchforge_apply_warnings(${target})

    if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            # clang-cl. Note the absence of /fp:precise: it maps to
            # -ffp-model=precise, and following it with -ffp-contract=off makes
            # clang emit -Woverriding-option, which /WX turns into a build
            # error. -ffp-contract=off is the stricter of the two and the one
            # §6 needs — contraction is what varies between an FMA-capable and
            # a non-FMA-capable target — so it is the one we keep.
            #
            # -ffp-contract=off is passed last so that nothing after it can
            # reset contraction back to a default.
            target_compile_options(${target} PRIVATE
                /clang:-fno-fast-math
                /clang:-fno-vectorize
                /clang:-fno-slp-vectorize
                /clang:-ffp-contract=off
            )
        else()
            # Real MSVC (ADR-0006). /fp:precise is the default but is stated
            # explicitly so a future /fp:fast in a toolchain file cannot win by
            # accident. /fp:contract is deliberately *not* passed. /Qvec- turns
            # off the auto-vectorizer, which §6 bans in sim TUs.
            target_compile_options(${target} PRIVATE
                /fp:precise
                /Qvec-
            )
        endif()
    else()
        target_compile_options(${target} PRIVATE
            -fno-fast-math
            -fno-unsafe-math-optimizations
            -fno-associative-math
            -frounding-math
            -fno-strict-overflow         # signed overflow must not be assumed away
            # Last, so nothing above can reset contraction to a default. GCC's
            # default is `fast`, so this flag is doing real work, not decoration.
            -ffp-contract=off            # no FMA fusion (§6)
        )
        # Auto-vectorisation is banned in sim TUs (§6): the vector and scalar
        # tails of a reduction can round differently, and the decision depends
        # on the host's -march. Pin it off rather than trusting the default.
        target_compile_options(${target} PRIVATE
            $<$<CXX_COMPILER_ID:GNU>:-fno-tree-vectorize>
            $<$<CXX_COMPILER_ID:Clang,AppleClang>:-fno-vectorize;-fno-slp-vectorize>
        )
    endif()
endfunction()

# Non-sim targets (server I/O, tools). Same warning bar, no determinism pinning.
function(pitchforge_strict_target target)
    target_compile_features(${target} PUBLIC cxx_std_20)
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
    _pitchforge_apply_warnings(${target})
endfunction()
