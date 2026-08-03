# ADR-0002: Defer the UE5 renderer target to M7, and place the Tauri shell in /app/tauri

- **Status:** Accepted
- **Date:** 2026-08-03
- **Amends:** CLAUDE.md §4 (repository layout), §12 (the M0 gate)

## Context

CLAUDE.md §12 defines the M0 gate as "one command builds everything", where *everything*
means three targets: the native core, the `/app` UI shell, and the UE5 project under
`/ue5/`.

Unreal Engine is not installed on the development machine, and the UE5 renderer is not
scheduled until M7. §4 also does not name a location for the Tauri v2 Rust crate, although
§9 requires one.

## Options considered

### 1. Scaffold `/ue5/` now from the specification, unbuilt

Write the `.uproject`, the plugin `.Build.cs` referencing `libpitchsim` as an external
static library, and the module stubs.

**Cost:** unverifiable code sitting in the tree for seven milestones. A `.Build.cs` that has
never been run by UnrealBuildTool is a guess. It would be stale by M7 — UE 5.4's build
conventions will have moved — and worse, it would *look* done, so the M7 estimate would be
made against work that has not actually started. The M0 gate would also have to be weakened
to "builds two targets and contains a third", which is the kind of quiet redefinition §0.5
exists to prevent.

### 2. Install UE5 and scaffold properly

**Cost:** a ~100 GB engine install and a substantial build to verify a skeleton whose real
work is seven milestones away. The linking approach is already settled by ADR-0001; nothing
about M1–M6 depends on validating it now. The information gained does not justify the cost
at this point, and will be cheaper and more accurate to obtain at M7 against a real event
stream.

### 3. Defer the target, record it explicitly *(chosen)*

Do not create `/ue5/`. Amend the M0 gate to two targets. Record the deferral here so it is a
decision with a date rather than an omission someone discovers.

**Cost:** the §2 architecture claim that UE5 links `libpitchsim` is untested until M7. If it
turns out UnrealBuildTool cannot consume our static library cleanly, we find out late. This
is mitigated by ADR-0001's escape hatch — the renderer can consume the event stream over IPC
instead of linking — and by the fact that `libpitchsim` depends on nothing but the STL,
which is the property that makes it linkable in the first place.

## Decision

**UE5 is deferred to M7.** `/ue5/` is not created at M0. The M0 gate is two targets: the
native core and `/app`.

`.gitignore` already excludes `ue5/**/Binaries/`, `Intermediate/`, `Saved/` and
`DerivedDataCache/` ahead of the directory existing, so that a first local editor run cannot
accidentally commit derived content.

**The Tauri crate lives at `/app/tauri/`.** §4 lists `ui/`, `ipc/` and `design/` under
`/app/` and says not to invent new top-level folders. `/app/tauri/` is not a new top-level
folder, but it is not in the §4 table either, so it is recorded here. It sits beside the
packages it launches, and it is excluded from the pnpm workspace because it is a Rust crate,
not a Node package.

## Consequences

**Easier:** M0 is honestly complete rather than partially fictional. No dead scaffolding to
maintain, no misleading progress signal.

**Harder:** the §2 "UE5 links libpitchsim" claim carries risk until M7, and M7 starts from
zero rather than from a skeleton. When M7 begins, budget time for project setup that this
ADR chose not to spend now.

**Watch for:** `libpitchsim` acquiring a dependency between now and M7 that makes it harder
to link into Unreal — a threading primitive, an allocator, an exception path. §7 forbids
these anyway; this is one more reason they matter.

## Rollback path

Cheap and clean. Install UE 5.4+, create `/ue5/PitchForgeMatch/` and
`/ue5/Plugins/PitchSim/`, restore the third leg of the CI matrix. Nothing built between M0
and M7 depends on the renderer's absence, because §2 already forbids the renderer from
owning state that anything else reads.

## Verification

Not applicable — this ADR records the removal of a target. The remaining two are verified in
`/docs/status.md`.
