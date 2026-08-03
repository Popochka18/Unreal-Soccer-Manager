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
            message(FATAL_ERROR
                "MSVC is not a supported compiler for /core (ADR-0005). "
                "Configure with the ClangCL toolset.")
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
        # clang-cl: /fp:precise blocks contraction and reassociation; the
        # remaining pins go through the Clang driver directly.
        target_compile_options(${target} PRIVATE
            /fp:precise
            /clang:-ffp-contract=off
            /clang:-fno-fast-math
            /clang:-fno-vectorize
            /clang:-fno-slp-vectorize
        )
    else()
        target_compile_options(${target} PRIVATE
            -ffp-contract=off            # no FMA fusion (§6)
            -fno-fast-math
            -fno-unsafe-math-optimizations
            -fno-associative-math
            -frounding-math
            -fno-strict-overflow         # signed overflow must not be assumed away
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
