# Native single-tile tree ghost — 2026-10-07

Continues `98cc070254f50452e72d31363d54f2f35878f326` on
`handheld/modern-ui-paths`. Completed placement, presentation and controller
mappings are preserved. Private RCT assets remain outside git.

## Behavior and architecture

The existing Trees tool previews one eligible loaded full-tile tree through
native SmallSceneryPlaceAction with ghost/noSpend/allowDuringPaused flags.
The ordinary action query runs first: eligibility/research/restrictions,
ownership, clearance, pause and affordability remain authoritative. The ghost
cannot appear merely because ghost flags would bypass funds or pause checks.
No object loading, research mutation or native simulation changes.

Trees.cpp owns only the requested tile/object and the native ghost identity
(tile/object, actual returned base height and quadrant). No map pointers survive
native actions. Tree selection remains in Explorer, independent of preview state,
so a future image picker can select an object without rewriting placement.
Y still cycles; X still rotates the camera; L/R still zoom.

Native scenery clearance skips ghost elements. Cost/error queries therefore
continue to use the real normal action while the preview remains present, rather
than replacing the ghost to query the tile. Unchanged valid target/object/height
updates retain the same element. Changed target, object or native ground height
replaces it. Availability, ownership, clearance, cash and pause changes are checked
on updates; invalid state removes the preview immediately. Missing externally
removed previews can be rebuilt on a subsequent valid update.

A clears the preview, rechecks the real action, then queues exactly one normal
placement. Ghost placement/removal never spends cash. Removal uses the native
ghost-only SmallSceneryRemoveAction identity, so it cannot select a real tree;
its native ghost query also permits cleanup after ownership changes or a
forbid-tree-removal rule. Desktop global scenery ghost state is not commandeered.

## Lifecycle and save safety

Explorer clears the preview on exit, tool changes, modal/focus loss and quit.
Handheld menu opening clears it before pausing. Existing EndTerrain hooks cover
park/map/progress/load transitions and UI shutdown. Returning to an eligible tool
revalidates on the next explorer update. Single-item Y cycling does not rebuild
an unchanged preview.

Trees participates in the existing ProvisionalElements remove/restore hooks,
including the ScenarioSave scope guard. Suspension removes only the ghost and
keeps its request; restoration revalidates. A depth counter supports nested
suspensions, and ClearPreview discards restoration intent so an exited tool is
not resurrected. Manual construction-window closure also clears tree previews.
Existing path provisional behavior and autosave neighbour-edge protections are
unchanged. There are no save format changes.

## Validation

Native GUI/CLI build passes. Existing CTest suites pass 3/3. Expanded native
integration passes both headless and with the real UI context, private assets
and SDL dummy drivers. All previous paths/ghosts/Path Options, water, surface,
scenery removal, research and save checks remain enabled.

New coverage checks one ghost/no real tree/no charge; native identity and ground
height; unchanged-update retention (ghost metadata detects replacement); ordinary
cost queries with an existing ghost; target/tree changes; changed ground height;
nested provisional suspension; clear-during-suspension safety; invalid targets;
research/restriction/ownership/funds/pause invalidation; ghost cleanup after
forbidden tree removal; unchanged terrain bytes after preview cleanup; actual
pause/build/tool/map/focus/shutdown cleanup; autosave restoration and reload
without ghosts; manual save; one real queued placement and exact queried charge;
occupied-tile rejection and preservation of the real tree during preview cleanup.
The real UI run dispatches the production save/provisional and lifecycle hooks.
The dummy headless context explicitly suspends previews around autosave because
it does not dispatch UI intents. Test-only object/research/terrain setup is confined
to fixtures. The native unloaded-surface test error remains expected.

Reproduce with `tools/handheld/run_native_test.py --build BUILD --data ENGINE_DATA
--ninja NINJA`, adding `--assets PRIVATE_RCT_ROOT` for real UI lifecycle coverage.

## Captures and limitations

The existing `tools/handheld/render_trees.py` capture harness now produces
`tree-preview.png` and `tree-placed.png`, each exactly 512 × 384. It loads the
user's **Mines of Africa**, selects its already loaded/available **Palm Tree**,
and uses a native camera/hit-test search to show target 2368,672. The preview
uses the game's native pale ghost rendering and $2.10 queried cost. The second
frame shows one real placement with an exact charge assertion. Production
panel/footer/context helpers are reused. No fixture trees are added, no assets
are committed, and the user's scenario file is never overwritten. These are
automated native scenario captures, not physical Brick/controller session images.

Ghosts can be occluded by native world scenery, just as desktop previews can.
Native palette ghost remapping is preserved; solid handheld panel colours and
all presentation geometry remain unchanged. Fresh AArch64 packaging and physical
Brick testing are still pending.

## Next milestone

Y-to-cycle is intentionally unchanged in this milestone. The next narrow UX slice
is a controller-native picker showing native in-game images for currently loaded,
available full-tile trees, opening from Y and returning a selection to this tool.
It must preserve placement/camera mappings outside the picker and this ghost/save
lifecycle. Other scenery categories, height/recolour controls, brushes, management,
coaster construction, keyboard and persistent top HUD remain deferred.
