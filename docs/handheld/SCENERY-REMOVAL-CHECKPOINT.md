# Native targeted small-scenery removal — 2026-10-07

Continues `f61d56ac967a5c9be547bf4e35bbadde3838006f` on
`handheld/modern-ui-paths`. Completed UI, Terrain, Water, surface painting and
native path ghosts are retained. Private RCT assets remain outside git.

## Behavior

Build → Remove scenery activates removal of one visible small-scenery element.
D-pad pans the existing view/centre target, A queues one SmallSceneryRemoveAction,
B exits, and Start opens the existing pause menu. X camera rotation and L/R zoom
remain available; Y has no removal action. Other modes' mappings are unchanged.

The tool uses native GetMapCoordinatesFromPos with all interaction categories,
then accepts only a non-ghost small-scenery hit. Unsupported foreground types
are ignored rather than picking scenery through them. The existing native tile
selection and centre reticle mark the target, and the compact panel shows its
localized object name. Long names use existing bounded/ellipsised row text.

`SceneryRemoval.h` copies the native identity (tile, base height, quadrant,
object index), never retaining a map pointer across updates. The native action's
first-match lookup does not exclude ghosts for a normal removal. The wrapper
therefore rejects ghost matches and rejects a hit that is not that first identity
match. Queries/submission recheck that a real matching element still exists;
missing/stale targets cannot remove a different height, quadrant or object type.
No ghost flags, bypass rules, native simulation changes or fake mouse input.
Normal native action query/queue execution governs ownership, affordability,
pause, tree-removal restrictions and signed removal price. Positive costs charge;
negative costs refund. Existing modal/tool gating prevents competing desktop
construction input while the handheld tool is used.

## Presentation and lifecycle

The compact 164 × 146 rail uses existing solid palette/ConceptStyle primitives,
a gold target row, quiet secondary text, cost/error box and 28-pixel action strip
with D-pad Move / A Remove / B Back. No styling redo, extra permanent regions,
world graphics changes or dithering.

This is a terrain-explorer mode. Existing target refresh, modal/focus gating,
B/Build exit, map/load/progress changes and shutdown cleanup apply. Beginning
removal ends the previous path tool and preview. Native queued deletion is the
only permanent change; selection/query/save cannot delete scenery. Path Options,
save/autosave ghost suspension and research behavior remain intact.

## Validation

Native GUI/CLI build passes. Existing CTest suites pass 3/3. Expanded integration
coverage passes headless and with the real UI context and personal RCT assets
using SDL dummy drivers. New checks cover one exact native identity among same-tile
siblings at other quadrants/heights, mutation-free query, queued execution, exact
positive cost and negative refund (including zero-cash refund), stale/absent and
unsupported hits, ghost/ambiguous identity rejection, ownership, insufficient
funds, pause, forbidden tree removal and save/reload. Surviving same-tile and
neighbour elements retain their metadata. Real Build entry checks old path tool/
ghost cleanup. All previous path ghost, water, painting, research and save/autosave
integration checks still pass. The unloaded-surface test's logged error is expected.

## Native scenario captures

`tools/handheld/render_scenery_removal.py` loads the user's **Mines of Africa**
scenario. Camera search uses the same native sprite hit test/filter as production
and chooses an allowed small-scenery element; no fixture scenery is added.
The selected **Corrugated Roof Piece** is native small scenery, not large scenery.
Two exactly 512 × 384 native renderer snapshots show the hit before removal and
its absence afterward. The native query reports a $1.50 refund, displayed using
the existing signed cost formatter. The second frame re-hits the centre rather
than keeping a stale target. The user's scenario file is never overwritten.
These are automated native scenario captures, not physical Brick session images.

Reproduce: `tools/handheld/render_scenery_removal.py --build BUILD --data
ENGINE_DATA --assets PRIVATE_RCT_ROOT --scenario SC6 --output DIR --ninja NINJA`.
Run `tools/handheld/run_native_test.py` with build/data/ninja paths; add
`--assets PRIVATE_RCT_ROOT` for real UI entry coverage.

## Limits and next milestone

Native removal identities cannot express two overlapping elements with exactly
identical tile/height/quadrant/type. A hit of a later duplicate is rejected rather
than deleting the first. Small-scenery classification includes native building
pieces, not just trees. Paths, rides, walls, large scenery, entities and ghosts
are unsupported. This is not a bulk clear tool. Fresh AArch64 packaging and
physical Brick verification remain pending.

The next narrow gameplay milestone is ground-level placement of loaded, full-tile
trees via SmallSceneryPlaceAction, with native validation/pricing and the existing
compact UI. Keep placement ghosts, height/recolor controls, other scenery, brushes,
management, coaster construction, keyboard and HUD as separate later milestones.
