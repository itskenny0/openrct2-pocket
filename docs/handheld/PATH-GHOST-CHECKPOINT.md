# Native single-tile path ghost checkpoint — 2026-10-07

Continues the solid-palette checkpoint on `handheld/modern-ui-paths`. No control
bindings, menu actions, world graphics or placement simulation rules changed.

## Behavior

The active controller path tool previews the actual selected normal/queue
surface, railings, ground-following/fixed height, slope and rotation through
native FootpathPlaceAction ghost flags. A uses the existing real queued action
and charges once; preview actions are local, temporary and never spend cash.
The ordinary placement query remains authoritative for pause, ownership,
clearance and affordability. Its cached result also supplies the cost badge,
so drawing UI windows does not remove the preview or query a ghost as a real path.

The cache includes target/configuration, simulation tick, cash and pause state.
Unchanged calls in a tick do not replace a preview. The existing native
provisional-element lifecycle removes it while guests update and revalidates
on restoration. Configuration changes, invalid targets, removal mode, menu
open, focus loss, tool end, map changes, loading/progress and shutdown clear it.
Options/pause return restores placement on the next eligible explorer update.
Ownership changes cannot strand a tool-owned ghost: if a native removal query
rejects changed ownership, the native ghost-only executor discards the temporary
path; real placement/removal validation remains unchanged.

Native path ghosts may temporarily affect neighbouring connection edges.
ScenarioSave now suspends/restores native provisional elements for the entire
save with a scope guard, including autosaves and exception paths. No save format
changes. Manual construction-window closure also clears the handheld preview.

## Validation

Native GUI/CLI build succeeds. Existing CTest suites pass 3/3. Expanded native
integration checks pass both headless and with the real UI context using SDL
dummy drivers. Tests cover one ghost, unchanged updates, temporary suspension,
modal clear/return, old-target cleanup, normal/queue/slope placement, exact cash,
invalid targets, ownership changes, insufficient funds, pause, real path
conversion, save/reload, autosave neighbour-edge safety, removal and research.
Real UI tests invoke Path Options and pause entry points and load a new park.
An AddressSanitizer harness run passed while checking the initial lifecycle
fixtures; it is not a full-engine sanitizer build.

## Captures and personal assets

The user's Data, ObjData and Scenarios archives were extracted locally, outside
the git repository. Their scenario loads as **Mines of Africa**. The three
512 × 384 PNGs render that actual loaded park and native ghost through the game
renderer, with production handheld components. They use an automated camera
and path configuration, not a reference-image background or mock world. They
are native scenario snapshots, not physical Brick/controller session captures.

Reproduce with `tools/handheld/render_path_preview.py --build BUILD --data
ENGINE_DATA --assets PRIVATE_RCT_ROOT --scenario SC6 --output DIR --ninja NINJA`.
The helper selects a legal owned tile where the preview is visible, then captures
ground, queue and elevated/rotated slope states without committing construction.
Use `tools/handheld/run_native_test.py --assets PRIVATE_RCT_ROOT` for additional
real UI lifecycle coverage; the script defaults SDL drivers to dummy in that mode.
The normal build/data/ninja arguments are still required.

## Limits and next milestone

Native ghosts intentionally do not cover an existing real path for a surface or
queue conversion. Those real actions still work. Ghosts use the native pale
preview palette; world scenery can occlude them, just as with desktop previews.
A fresh AArch64 package and physical Brick test remain pending. Connected
bridge/tunnel construction is not added in this milestone.

The smallest next feature slice is a single-tile water tool: native raise/lower
water actions with the existing terrain interaction pattern and polished style.
Preserve all completed path preview behavior. Management, naming keyboard and
persistent top HUD remain later work; no broad UI redesign is needed.
