# Maze and Special Ride Construction — incremental checkpoint

Goal: 100% ordinary OpenRCT2 gameplay through controller-native Pocket.
Continues canonical `7c3c8ffaabae26273f6d859efa8f7d5691f399f8` / local
`1180d161d86fdccc259c4b5ca0c0ee9898767190`, identical tree
`a7c492dcc5bf35fc066fb899e96c295c463e15d9`. Esperanto is preserved and excluded.

## Phase 1 — native maze adapter

Build → Rides offers actual loaded/researched maze objects alongside existing
flats and conventional track families. RideCreateAction creates the selected native
maze. MazeSetTrackAction owns initial placement, 16-unit half-tile directional
build/move/fill, wall connections, tile removal, station state and native accounting.
Pocket never interprets or mutates maze wall bits. Review snapshots retain opaque
native entries for the selected and neighbouring tiles; changed geometry, identity,
research, closure, no-money state and costs invalidate removal confirmation.

A builds one segment; X moves through existing native maze tiles; Y reviews filling
/removing the current half-cell. Removal defaults to Cancel, L1/R1 changes the
choice, A applies, B cancels. L1/R1 selects direction outside review. L2/R2 adjusts
initial height in native 16-unit increments. D-pad pans, Select toggles camera
controls (L1/R1 zoom, X rotates), Start opens Dashboard. B leaves construction to
ride inspection; Finish entrances/exits and Access reuse native entrance and path
workflows. Existing mazes offer Edit maze from inspection. Resume selects the first
native maze tile; native Move navigates existing adjacent tiles. No map pointers
survive actions. Empty newly-created rides cancel through RideDemolishAction.

Native MazeSetTrackAction query now rejects missing/wrong rides before dereference,
open/indestructible rides, invalid direction/granularity/mode and missing previous
fill targets. Maze access uses adjacent native maze tiles; native entrance/exit
actions remove entrance hedges. Conventional station sequence-side rules remain.

Validation: Release GUI/CLI build, public native MazeConstructionTests (research,
creation, half-cells, multiple tiles, movement/walls/fill, costs, cancellation,
stale geometry, reused identity, bounds, ownership, access, save/reload). Factory
Capers private assets exercise production controller dispatch, duplicate actions,
SDL virtual held-A handoff, native maze entrance/exit completion and save/reload.
Captures are native viewport + production Pocket components, not full SDL frames.

### Precise remaining maze / ordinary construction gaps

Maze construction remains PARTIAL on the 100% roadmap. Selection footprint/arrow
and native queries are delivered; individual segment placement ghosts, arbitrary
existing-cell picking across disconnected components and direct wall-only toggles
are not delivered. Native build/fill supplies desktop segment/wall semantics.
Sustained maze operation/natural guests, full-gameplay-frame HUD verification,
AArch64 execution and physical Brick validation are not claimed.

Saved maze designs and special-family audit follow in separately validated phases.
All previously documented construction/editing/design-export/elevation/substitution
and entrance/exit-ghost gaps remain. Exact canonical publications and final captures
are recorded after publication, rather than predicting commit IDs.
