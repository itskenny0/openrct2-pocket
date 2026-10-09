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

## Phase 2 — native maze saved designs

SavedTrackDesignPlaceAction now distinguishes native maze designs (nonempty native
maze elements, no conventional track elements) from conventional designs. Both
require actual loaded matching vehicles, native type/object research and a loaded
station object. Designs enumerates native maze repository entries and imports them
through the existing native importer. No Pocket maze parser or geometry is added.

Existing design rotation, automatic native terrain-fit height, optional scenery /
native omitted-scenery warning, borrowed preview ride, query, native ghosts,
Cancel-first cost review, exact-cost execution and completion/recovery are reused.
Maze ghosts use native TrackDesignPlaceVirtual/TrackDesignPlace/MazePlaceTrackAction
and native ghost removal; tests count ghosts before and after cleanup for all four
orientations. Export in tests creates a genuine native design from action-built
geometry; a player-facing export workflow remains missing.

Validation: expanded MazeConstructionTests passes native access placement and
native maze export/import/rotation/ghost-cleanup/cancel/repeat/execute/exact-cash/
recovery. Factory Capers asset-backed production viewport/component captures pass
native design ghost/rotation/review/execute and entrance/exit preservation. Manual
elevation, substitution choices and player-facing export remain ordinary gaps.

Publication: Phase 1 canonical `bbf017d6834c5f332e398b171b4651165456c5e2`,
local `c6c6934a04fc3734ef3a77c741030f5b490a6e1f`, identical tree
`5aee5e2e7dcf11881bdaa6efd956e502a9b2f1bf`.
