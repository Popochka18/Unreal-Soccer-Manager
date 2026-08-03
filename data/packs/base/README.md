# base — the official data pack

Empty until M1. Human-editable `json5`/`csv` sources plus images, compiled into `world.db` by
`/tools/packc`. Humans never edit `world.db`; they edit what is in here.

M1 target: 4 nations, 8 divisions, fully populated.

## Everything is data

Every rule, competition format, attribute weight, formula coefficient and string lives here
rather than in source (§3). If the simulation needs a number, it comes from this pack.

## Layering

`base` → `official-update` → `user-mod-*`. Later layers patch earlier ones **by stable string
uid** (`club.eng.northbridge_utd`), never by numeric row id. The compiler reports conflicts
with the pack names involved and never resolves them silently.

## Content rules (§14)

Fictional only. No real player names, club names, crests, kits or competition names, and no
scraped third-party datasets. Names are generated from per-nation phoneme and name-part
tables under `naming/` — never from a hardcoded list.

Every asset row carries a `license` field, and CI fails on `unknown`.
