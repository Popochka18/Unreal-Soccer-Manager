---
description: Design or evolve a database entity end to end.
argument-hint: <entity, e.g. competition_rules>
allowed-tools: Read, Write, Edit, Bash
---
Entity: $ARGUMENTS

1. Read /core/db/schema/*.sql and /docs/contracts/db.md first. Restate current state.
2. Propose the table(s): STRICT, FK ON, `id`/`uid`/`pack` columns, indices justified by an
   actual query, no nullable column without a written reason.
3. Write the forward-only migration + the compat test that loads a previous-version save.
4. Update the JSON-Schema in /data/schema/ and the pack-compiler ingestion.
5. Update /docs/contracts/db.md and the base pack sources so the world still compiles.
6. Show me: migration SQL, schema diff, and the `packc` output proving validation passes.

Reject the task if the data belongs in an existing table — say which.
