# Native single-tile land surface checkpoint — 2026-10-07

Continues `5272d9db2cb470036260c3004167558fa8f8def5` on
`handheld/modern-ui-paths`. Completed UI polish, Water and path ghosts are retained.
Private RCT Data/ObjData/Scenarios remain outside the source repository.

## Behavior

Build → Surface starts single-tile painting. D-pad moves the existing terrain
centre target; A submits one native queued SurfaceSetStyleAction; Y cycles loaded
TerrainSurfaceObjects in slot order and wraps; B exits; Start opens pause.
Existing mode mappings and semantic actions remain unchanged. No larger brushes,
edge painting, recoloring controls or desktop tools are added.

`Surface.h` supplies loaded-object cycling, localized names and a single-target
native action wrapper. Missing, off-map, map-border or unaligned targets are
rejected before the native range clamp can redirect them. Missing selections
cannot invoke a null-style action. Unloaded object validation, affordability,
pause and landscaping restrictions remain native queries/actions, without bypass
flags or custom prices.

SurfaceSetStyleAction receives a null edge style and the target's current surface
and edge colors. It changes surface style without changing land height, slope,
water or edge style/color. Native terrain color interpretation and grass/litter
side effects remain native; no renderer or simulation logic is changed.

Native painting deliberately skips unowned land and returns $0.00 without a
change. Repainting the same surface is also a native zero-cost operation. These
are not overridden by handheld-only ownership/error rules. A successful submission
reports a change request; it does not promise a change before the queue runs.

## Presentation and lifecycle

The new panel reuses ConceptStyle's solid palette, 164 × 174 rail, selected gold
row, quiet Brush status, compact cost/error box and 28-pixel action strip. Only
new-mode footer labels change to A Paint / Y Surface. No dithering or world sprite
replacement. Long names use the existing bounded row text renderer.

Painting is a terrain-explorer mode, so existing menu/modal target gating, pause,
B/Build exit, load/progress/map change and shutdown cleanup apply. BeginSurface
ends any previous path tool and ghost. The save/autosave provisional lifecycle,
all seven Path Options, Water and research are unchanged.

## Validation

Native GUI/CLI build passes. Existing CTest suites pass 3/3. Expanded native
integration tests pass headless and with the real UI context using the personal
RCT installation and SDL dummy drivers. New tests cover loaded-only cycling and
wrap, mutation-free queries, queued painting, exact queried cash, repeated paint,
missing/off-map/border targets, null/unloaded surfaces, native unowned no-op,
insufficient funds, pause and forbidden landscaping. A sloped, wet fixture checks
height/slope, both colors, edge style, water, neighbour surface and save/reload.
Real Build-menu entry checks that painting clears an active path tool/ghost.
Existing water, path placement/removal, ghost lifecycle, save/autosave safety and
research checks still pass. The logged invalid surface-style message is expected
from the test of native rejection of an unloaded object.

## Captures

`tools/handheld/render_surface.py` loads the user's **Mines of Africa** scenario
and calls the native software renderer and production UI. Two exactly 512 × 384
captures show the same tile before and after one Grass paint, with its queried
$3.00 cost and subsequent $0.00 same-style cost. The camera search temporarily
paints/restores candidate tiles only in its isolated loaded park. It never writes
the user's park file. These are automated native scenario snapshots, not fixture
backdrops or physical Brick/controller-session captures.

Reproduce: `tools/handheld/render_surface.py --build BUILD --data ENGINE_DATA
--assets PRIVATE_RCT_ROOT --scenario SC6 --output DIR --ninja NINJA`.
Integration: `tools/handheld/run_native_test.py` with build/data/ninja arguments;
add `--assets PRIVATE_RCT_ROOT` for the real UI context.

## Limits and next milestone

Only loaded surfaces are offered; no object loading or palette editing. Water
can obscure the painted ground as in desktop rendering. There is no new painting
ghost, only the existing native tile selection. Fresh AArch64 packaging and
physical Brick verification remain pending; this validates the native host build.

The next small gameplay slice is targeted small-scenery removal using
SmallSceneryRemoveAction. Keep it limited to a single hit small-scenery element,
with native validation/pricing and existing compact UI. Placement, large scenery,
brushes, management, coaster construction, keyboard and HUD remain later work.
