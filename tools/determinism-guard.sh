#!/usr/bin/env bash
# Determinism guard — CLAUDE.md §6, and the first step of the /determinism command.
#
# Greps the simulation translation units for constructs that make output depend
# on something other than the declared match input. Comments are stripped first,
# because the headers legitimately *discuss* floats and threads while banning
# them.
#
# This is a coarse net, not a proof. It does not catch: mutable static state,
# uninitialised reads, iteration over a pointer-keyed ordered container, or
# ODR-violating inline functions. Those need the sanitizer builds and a real
# cross-OS golden comparison. Do not treat a green run here as determinism.

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Directories whose contents must be deterministic. /core/tests is excluded:
# tests are not sim TUs and legitimately use timing and unordered containers.
sim_dirs=("core/pitchsim" "core/world")

# pattern<TAB>human-readable reason
patterns=$(
    cat <<'EOF'
\bfloat\b	float in a sim TU (§3: fixed-point q16.16 only)
\bdouble\b	double in a sim TU (§3: fixed-point q16.16 only)
\bsrand\b	srand (§6: no global RNG)
\brand\s*\(	rand() (§6: use Rng with a derived stream)
\bdrand48\b	drand48 (§6: no global RNG)
\btime\s*\(	time() (§6: wall clock is not an input)
std::chrono	std::chrono (§6: wall clock is not an input)
std::unordered_	unordered container (§6: iteration order is unspecified)
std::thread	std::thread (§6: no parallelism inside a match)
std::async	std::async (§6: no parallelism inside a match)
#include <thread>	<thread> (§6: no parallelism inside a match)
#include <random>	<random> (§6: one Rng type, see rng.hpp)
EOF
)

strip_comments() {
    perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g' "$1"
}

violations=0

for dir in "${sim_dirs[@]}"; do
    full="$repo_root/$dir"
    [ -d "$full" ] || continue

    while IFS= read -r -d '' file; do
        stripped="$(strip_comments "$file")"

        while IFS=$'\t' read -r pattern reason; do
            [ -n "$pattern" ] || continue

            if hits="$(printf '%s' "$stripped" | grep -nE "$pattern" || true)"; [ -n "$hits" ]; then
                while IFS= read -r hit; do
                    printf '%s:%s: %s\n' "${file#"$repo_root"/}" "${hit%%:*}" "$reason"
                    violations=$((violations + 1))
                done <<<"$hits"
            fi
        done <<<"$patterns"
    done < <(find "$full" \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) -print0)
done

if [ "$violations" -gt 0 ]; then
    printf '\ndeterminism-guard: %d violation(s). See CLAUDE.md §3 and §6.\n' "$violations" >&2
    exit 1
fi

echo "determinism-guard: clean (${#sim_dirs[@]} sim directories scanned)"
