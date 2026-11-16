# Native ground-level trees — 2026-10-07

Continues `e7d2dec0c8d3cbdf6a125435b4ebd08f97728a11` on
`handheld/modern-ui-paths`. Completed UI, Terrain, Water, surface painting,
targeted scenery removal and path ghosts are retained. Private assets stay
outside git.

## Behavior and native authority

Build → Trees activates the existing single-tile explorer pattern. D-pad pans
and moves the centre tile target; A submits one SmallSceneryPlaceAction; Y cycles
eligible loaded trees; B exits; Start opens the existing pause menu. X still
rotates the camera and L/R still zoom. Existing modes and mappings are unchanged.

`Trees.h` includes only loaded SmallSceneryObject entries flagged isTree and
occupiesFullTile, excluding half- and three-quarter-tile shapes. It uses native
IsSceneryAvailableToBuild for research and scenario restriction checks both when
cycling and before querying/submitting. No production object loading, research
changes or bypass flags. No eligible entries produces an unavailable state;
Y can refresh the selection after availability changes.

The action uses native automatic ground height (`z=0`), full-tile quadrant 0,
and desktop's default direction 3 and colours (bordeauxRed, yellow, darkBrown).
There are no height, rotation or recolour controls for the placed tree. Native
query/queued execution govern ownership, ground/slope/water clearance, funds,
pause and cost. Queries cannot place trees or charge money; A queues exactly one
normal action. Missing, off-map and non-aligned targets are rejected before
submission. No native actions or simulation code are modified.

## Presentation and lifecycle

The 164 × 146 compact rail uses existing solid palette fills, gold selection,
bounded object-name text and quiet Ground level status. A small tree glyph is
shared by the Build entry and tool row. The 28-pixel strip shows D-pad Move /
A Place / Y Tree / B Back. Existing native tile selection, reticle and queried
cost/error feedback are reused. No UI polish redo, textures or extra regions.

Trees is an explorer terrain variant; existing focus/modal gating, Build/B exit,
pause, load/map transitions and shutdown cleanup apply. Entry ends the path tool
and ghost. No scenery ghost or provisional element is created. Save/autosave
protection and all seven Path Options actions remain unchanged.

## Validation

Native GUI/CLI target builds successfully. Existing CTest input, launcher and
save-preservation suites pass 3/3. Expanded PathActionsTests passes headless and
with the real UI context plus private RCT assets/SDL dummy drivers.

New integration checks exercise loaded eligibility, non-tree/unloaded rejection,
one-item cycling, two-item wrap with unavailable entries skipped, research and
scenario restriction rejection, mutation-free query, native queue semantics,
one real tree and exact queried cash, automatic height on raised ground, invalid
and absent targets, ownership, insufficient funds, pause, occupied-tile rejection,
unchanged terrain state and neighbouring element bytes, and exact tree/cash/
neighbour persistence after native save/reload. Fixture-only object/research
setup is confined to tests. Native real-UI Build entry checks previous path ghost
cleanup. All earlier water, surface, removal, path/ghost, autosave and research
checks remain enabled. The unloaded-surface test's logged native error is expected.

Commands: run `tools/handheld/run_native_test.py --build BUILD --data ENGINE_DATA
--ninja NINJA`, optionally with `--assets PRIVATE_RCT_ROOT` for real UI coverage;
run CTest in the existing handheld test build.

## Native scenario captures

`tools/handheld/render_trees.py --build BUILD --data ENGINE_DATA --assets
PRIVATE_RCT_ROOT --scenario SC6 --output DIR --ninja NINJA` loads the private
**Mines of Africa** scenario. It selects an already loaded, available **Palm Tree**
without loading objects or unlocking research. Camera search changes no park
state and uses native viewport hit testing to keep the ground target visible.
Two exactly 512 × 384 native-renderer frames show target 2368,672 before and after
one real placement, with a $2.10 native queried cost and exact charge assertion.
The user's scenario file is never overwritten. Production panel/footer/context
helpers are used; captures use an automated camera in an isolated native scenario
context, not a physical Brick session. No fixture scenery is added.

## Limits and next milestone

Only loaded, available full-tile trees are offered; other small scenery and
partial-tile trees are intentionally excluded. Object names may be ellipsised
within the existing compact row. Native restrictions can reject sloped/wet or
occupied tiles. There is no placement preview ghost yet. Fresh AArch64 packaging
and physical Brick verification remain pending.

The next narrow milestone is a native single-tile ghost for this tree tool,
following the established path preview lifecycle and save/autosave protections.
Do not add other scenery, height/recolour controls, brushes, management, coaster
construction, keyboard or top HUD in that milestone.
