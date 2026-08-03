---
description: Work inside the UE5 renderer or its PitchSim plugin.
argument-hint: <task, e.g. camera rig, crowd, kit materials, event->animation mapping>
allowed-tools: Read, Write, Edit, Bash
---
Task: $ARGUMENTS

Before writing anything, confirm in one line that this task is rendering/audio/input only
(§8). If it is not, stop and tell me which layer it actually belongs to.

Constraints: no gameplay state in UE5; no SQLite; no logic in Blueprints; assets are
streamed from the content store by hash; the renderer must survive server restart and the
server must survive renderer crash.

Deliver: C++ changes, the event→visual mapping table, and a note on how it behaves when the
event stream arrives late, out of order, or is cut off mid-match.
