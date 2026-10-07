# Native tree image picker — 2026-10-07

Continues `1688dffbcd53c0868b00224fce4f703ab4cf2a6c` on
`handheld/modern-ui-paths`. Existing placement, tree/path ghosts, compact styling
and gameplay are retained. Private RCT assets remain outside git.

## Controls and selection

Y in the Trees tool now opens Choose tree instead of cycling. D-pad navigates a
three-column, six-item paged grid; moving down/up crosses pages as needed.
Horizontal movement stays in the current grid row. A selects and returns to the
existing placement mode; B cancels without changing the placed-object selection.
Start discards unconfirmed picker focus and opens the existing Park/Pause screen,
with the prior tree selection retained. Resume returns to the tool. Placement A,
B exit, D-pad targeting, Start pause, X camera rotation and L/R zoom are unchanged
outside the picker. All other tool mappings remain unchanged.

Opening uses the current tree as focus. The tree-only adapter includes only loaded
full-tile trees passing Trees::Eligible (native research/scenario restrictions),
without loading objects or unlocking research. It refreshes on open/navigation;
confirmation rechecks eligibility. A newly unavailable selection cannot be
activated; the list refreshes and displays feedback. Empty lists have a safe,
compact unavailable state, and B/Start still work. Tool/map/load/shutdown changes
reset the underlying tool; a stale picker closes on its next update.

Explorer exposes a validated selection setter separate from preview/placement.
The menu never submits a construction action. The existing controller menu handler
is extracted into HandleMenuInput, retaining its input suspension/one-action
boundary. Selecting A cannot fall through into park placement, and held A stays
disarmed until release; only a fresh press can place a tree.

## Reusable component and native rendering

ObjectPicker.h owns presentation-only grid navigation, paging and rendering.
Entries carry ObjectType/index identity, name and native preview zoom. Eligibility,
activation and gameplay state belong to adapters/callers. TreePicker.h is the only
category adapter implemented. Future path surface, shop/facility, ride or scenery
adapters can provide entries without changing the shared grid or input model.
No conversions for those categories are added here.

Previews use the already loaded Object::DrawPreview implementation used by native
desktop object selection, clipped to each card and minified by the native sprite
renderer. The tree adapter derives a 0/1/2 zoom from existing sprite dimensions,
so small trees remain visible and tall trees fit. No exported images, placeholder
art, parallel asset pipeline, object loading or replacement world graphics. With
graphics disabled, the adapter avoids sprite-table access and keeps a safe default
zoom for metadata-only native tests.

The picker retains solid panel/border/gold/text palette tokens. Its left panel is
244 × 286 for a full page, shrinks to 194 pixels high for one row and 102 for an
empty list. Six native images and compact labels are shown at once, with a larger
focused name and page counter. Long names use existing ellipsis rendering. The
28-pixel bottom strip shows D-pad Choose / A Select / B Back / Start Pause.
The transparent window uses full screen bounds to render that strip, as existing
Path Options does; only the compact panel and strip obscure the park.

## Ghost and save safety

The existing menu-open hook clears previews before pausing. No placement ghost
remains while the picker is open. Selection/cancellation does not spend money or
change the park. Closing restores the existing pause state and the next eligible
Explorer update revalidates the selected tree preview. Native placement queries
and actions still own clearance, funds, ownership, pause and pricing. Trees.cpp,
path ghost logic and save/autosave protection are unchanged.

## Validation

Native GUI/CLI build passes. Existing CTest suites pass 3/3. Native integration
passes headless and with the real UI context, private assets and SDL dummy drivers.
All previous native terrain/water/surface/removal/path/tree/ghost/research/save
checks remain enabled, including manual save/autosave/reload ghost safety.

New checks use loaded native tree fixtures across multiple pages: grid edges,
page crossing, partial pages, identity-preserving refresh, eligible-only data and
empty-list safety. Real UI tests cover current-tree focus, preview removal on
entry, cancellation/restoration with unchanged cash, newly restricted selections,
Start opening the existing pause screen, empty confirmation, and picker closure
when the tool ends. A native SDL virtual controller exercises the actual menu
handler: selecting A changes only selection, held A does not plant, release emits
no placement, and a fresh A reaches native placement and charges exactly once.
Fixture-only object/research setup is confined to tests. The expected unloaded
surface error from earlier negative tests remains present.

Run tools/handheld/run_native_test.py with build/data/ninja arguments; add
--assets PRIVATE_RCT_ROOT for real UI/controller handoff coverage.

## Native scenario captures

`tools/handheld/render_tree_picker.py --build BUILD --data ENGINE_DATA --assets
PRIVATE_RCT_ROOT --scenario SC6 --output DIR --ninja NINJA` captures the user's
Mines of Africa using production picker data/rendering and native world rendering.
The scenario has 37 eligible loaded trees across seven pages. Three exactly
512 × 384 frames show the first page, the partial final page (White Poplar Tree),
and that selection's native ghost with its $1.70 queried cost. The capture asserts
unchanged cash and adds no fixture objects or real construction. Private scenario
files are never overwritten. These are automated native scenario snapshots, not
physical Brick/controller session images.

## Limits and next milestone

Only loaded, available full-tile trees are supported. Native names can be generic
or ellipsised in cards; the focused name is larger. Previews use native palette
sprites and discrete zoom rather than a new smooth thumbnail pipeline. No search,
category tabs, height/recolour controls or other tool conversions are added.

Fresh AArch64 packaging and physical Brick verification remain pending. The next
small milestone should package this complete checkpoint for the Brick and provide
a focused smoke-test checklist before adding further UI categories or gameplay.
Preserve all completed behavior and do not introduce new features in that step.
