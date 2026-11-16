# Native single-tile water checkpoint — 2026-10-07

Continues `bb4606e30d1fe205174ec560876ef7618213ba83` on
`handheld/modern-ui-paths`. Completed solid UI polish and native path ghosts
remain intact. Personal RCT assets stay outside the repository.

## Behavior

Build now offers Water after Paths. Water reuses the terrain explorer:
D-pad moves the centre target, A queues one native action, Y switches raise/lower,
B leaves the tool, and Start opens the existing pause menu. Other mappings and
Build choices retain their semantics. There is only a single-tile brush.

`Water.h` wraps native WaterRaiseAction and WaterLowerAction over one MapRange.
Ordinary native Query and Execute remain authoritative for ownership, clearance,
funds, pause rules, water height changes and cost. Queries do not change cash or
water. No bypass flags, custom pricing or simulation changes are introduced.
The water target uses the desktop water tool's terrain/water hit test, so raised
water is targeted on its visible plane. Land and path targeting remain unchanged.

The production terrain panel gains water labels through a defaulted variant,
with the same 164 × 174 compact rail, solid indexed palette, gold focus,
28-pixel controller strip, native selection and queried cost/error feedback.
No UI redesign, textured fills or new permanent regions.

Water/land tool state and native selections now share cleanup on map changes,
loading/progress, leaving play and UI shutdown. EndTerrain still ends Paths,
so all prior ghost cleanup hooks are retained. Existing handheld pause and load
behavior, save/autosave provisional suspension and all seven Path Options
remain unchanged.

## Validation

Native GUI/CLI build passes. Existing CTest suites pass 3/3. Expanded native
integration tests pass headless and with the real UI context using the user's
RCT installation and SDL dummy drivers. New assertions cover querying without
changes, queued single raises, repeated raises, lowering, exact queried cash,
unchanged land/neighbour water, missing targets, unowned land, insufficient
funds, pause, native path clearance, rejected executions and save/reload of
water height/land/cash. Real UI tests enter Water through Build and verify the
old path tool/ghost is cleared. Existing ghost lifecycle, normal/queue paths,
slope/rotation/height, save/autosave safety, removal and research tests pass.

## Native scenario captures

`tools/handheld/render_water.py` loads the user's **Mines of Africa** SC6 and
renders real native water changes with production handheld UI at exactly
512 × 384. Captures show one raised pond and the same tile after lowering.
The script selects an owned, flat, unobstructed legal tile with visible water.
Camera search actions occur only in an isolated loaded scenario; no user's
park save is overwritten. These are automated native scenario snapshots,
not physical Brick/controller-session captures or fixture backdrops.

Reproduce with `render_water.py --build BUILD --data ENGINE_DATA --assets
PRIVATE_RCT_ROOT --scenario SC6 --output DIR --ninja NINJA` under
`tools/handheld`. Native integration runner: `run_native_test.py` with the
same build/data/ninja paths; add `--assets PRIVATE_RCT_ROOT` for real UI tests.

## Limits and next milestone

Native lowering on a dry tile is a zero-cost no-op; the UI reports the native
$0.00 result. Water has no path-style ghost or custom preview. The existing
native water sprites and indexed palette are unchanged. A fresh AArch64 package
and physical Brick verification remain pending; this checkpoint validates the
native host build, actions and renderer.

The next smallest gameplay slice is single-tile land surface painting through
SurfaceSetStyleAction, restricted to loaded terrain surfaces and preserving
land height, edge style, water and all completed tools. Larger brushes, scenery,
management, coaster construction, naming keyboard and persistent HUD remain
separate later milestones.
