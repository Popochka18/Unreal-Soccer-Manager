# Golden replays

Empty until M2. Will hold ~200 recorded matches: an input blob plus the BLAKE3 hash of the
resulting event stream (§6).

**CI fails on any hash change.** That is the point of them. If a change is intentional,
regenerate in a dedicated commit titled `golden: <reason>` and review it on its own — never
bundled with the change that caused it.

Before guessing at a cause, run `/tools/replaydiff`; it prints the first divergent tick
between two streams.

Files here are marked `-text` in `.gitattributes`. They are byte-exact, and a CRLF
normalisation on a Windows checkout would silently invalidate every one of them.
