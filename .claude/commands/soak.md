---
description: Run and analyse a long headless simulation.
argument-hint: <seasons, default 20>
allowed-tools: Bash, Read, Write
---
Seasons: $ARGUMENTS

Run /tools/soak. Then report, per league tier and per season:
champion diversity · points spread · goals per game · squad age pyramid · wage/revenue
ratio · transfer volume and record fee · injury rate · youth intake quality · number of
clubs in financial distress · unemployed-manager count · DB size growth.

Flag every metric that drifts monotonically — monotonic drift over 20 seasons is a bug,
not a feature. Attach the CSV to /docs/soak/<date>/ and diff against the previous run.
