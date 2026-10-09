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
and native queries are delivered. Individual segment placement ghosts remain
missing. Disconnected-cell selection was delivered in Phase 3 below. Native
build/fill supplies desktop segment/wall editing semantics without a Pocket wall algorithm.
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

## Phase 3 — existing-cell recovery and special-family audit

Maze camera mode now supplies controller-native selection of any real tile belonging
to the same closed native maze, including disconnected sections. D-pad pans, L2/R2
chooses one of four native quarter positions, A selects and resumes segment editing;
Select returns without selection. Ghosts, other rides, invalid map coordinates and
stale identities are excluded. Native MapGetTileQuadrant/getMapSelectQuarter owns
the selection overlay. This closes the disconnected-cell picking gap above.

Mini Golf uses regular native station endpoints, native miniGolfHole track group
and two/three-sequence TED footprints. The existing conventional adapter already
supports it; no duplicate adapter or geometry was introduced. New
MiniGolfConstructionTests validates all five native holes in all four orientations,
actual placement, multi-tile block identity, native costs, removal/undo refunds,
recovery, unsupported coaster inversions and save/reload. Mini Golf is validated
with public/native scenario fixtures, not asset-backed full circuit operation.
Transport/water/special families still need individual native operation validation;
this audit does not declare all offered ride types complete.

Updated maze mappings: Select camera, L1/R1 zoom and X rotate while in camera;
L2/R2 chooses cell corner and A selects an existing maze cell after initial placement.
Y removal is available outside camera mode. Review overrides camera controls and
always starts on Cancel. Start/menu and B/inspection recovery remain unchanged.

Phase 2 canonical `e3770c0482dff4dc0e580068623ec10e677c02a2`, local
`c79f7c4cc464864ff6aeb44d015790aacb1fd15e`, identical tree
`b43f9fb88b881ae73fbbc508a12d915244f0796e`.

## Final verification

Release GUI and CLI pass. MazeConstructionTests additionally passes disconnected
quarter selection, missing design objects and native insufficient-funds rejection
in a separately initialized zero-cash scenario. No live park cash/ownership/research
patches are used to make these tests pass. MiniGolfConstructionTests passes.
ConstructionTests, StaffCompletionTests, GuestCompletionTests,
PresentationLifecycleTests, RideConstructionTests and full asset-backed
PathActionsTests pass. CTest: 3/3 (input, launcher, save preservation).
Asset-backed CustomCoasterCapture (Wooden circuit, access, native open/natural
visitors), DesignCapture, and RideConstructionCapture Junior/Looping pass.
Capture drivers now count the new maze picker rows when selecting existing families.

Factory Capers maze capture also verifies menu context recovery, native entrance /
exit preservation in a design, held-A Cancel-first review handoff, quarter selection
and native persistence. SDL dummy hardware-renderer warnings are expected; PNGs
use the native X8 renderer and production panels. No sustained maze/Mini Golf guest
operation, other family operation, AArch64 execution or physical Brick behavior is
claimed. Original assets, archives, parks and generated TD6 test designs stay private.

## Published phase checkpoints

| Phase | Canonical GitHub commit | Local safety commit | Exact source tree |
| --- | --- | --- | --- |
| Maze construction/access | bbf017d6834c5f332e398b171b4651165456c5e2 | c6c6934a04fc3734ef3a77c741030f5b490a6e1f | 5aee5e2e7dcf11881bdaa6efd956e502a9b2f1bf |
| Maze saved designs/review | e3770c0482dff4dc0e580068623ec10e677c02a2 | c79f7c4cc464864ff6aeb44d015790aacb1fd15e | b43f9fb88b881ae73fbbc508a12d915244f0796e |
| Cell selection / special-family audit | bf05719e3457fca5831a4aad2f5df48b7bc839a8 | 6ea751b0a49d0d902ed01224991430dcbed5c6ca | 5548d13e823ab76b13fc174d36b69b8360816d6b |

Each publication checked current canonical parent, exact local/canonical source-tree
equality and guarded expected_sha / force=false fast-forward update. Local safety
history remains intact. The final documentation publication is identified by its
exact SHA in the delivered ZIP manifest and final report, avoiding self-reference.

## Capture deliverable

OpenRCT2-Pocket-Maze-Special-Ride-Captures.zip contains 14 genuine 512×384 PNGs,
checksum/dimension/provenance manifest and a README. All are Factory Capers native
asset-backed viewports with production Pocket components/menus. They are not full
SDL game frames and do not verify HUD overlap in a complete gameplay presentation.
The public native Mini Golf tests are geometry fixtures; no Mini Golf gameplay
capture or sustained operation is claimed.

| PNG | Actual feature |
| --- | --- |
| 01-maze-object-picker | Loaded/researched maze among native ride objects |
| 02-maze-initial-placement | Native placement footprint, height and queried cost |
| 03-maze-invalid-placement | Native ownership rejection |
| 04-maze-segment-wall-editing | Native multi-cell directional build/walls and cost |
| 05-maze-cancel-first-removal | Cancel is the initial selected removal choice |
| 06-maze-removal-review | Explicit remove choice, native zero-cost fill semantics |
| 07-maze-dense-camera-controls | Compact camera/quarter selection controls and feedback |
| 08-existing-maze-recovery | Production resumed maze editor |
| 09-maze-entrance | Native entrance placement stage |
| 10-maze-exit | Native exit stage with action-built entrance visible |
| 11-maze-design-ghost | Native ghost of exported/imported maze and access |
| 12-maze-design-rotation | Rotated native ghost at a queried valid site |
| 13-maze-design-cost-review | Production Cancel-first native design purchase review |
| 14-existing-maze-cell-picker | Native existing tile/corner selection |

All captures were inspected. Removed overlong panel text, clarified review controls,
hid disabled commands, made review override camera mode, showed native corner index
and preserved native error feedback. A rotated design was recaptured at a valid
native site; invalid placement was not presented as a successful preview.

## Remaining ordinary gameplay / next subsystem

Construction stays PARTIAL. Maze individual segment/initial object ghosts and
entrance/exit placement ghosts remain missing. Sustained maze and Mini Golf guest
operation, other transport/water/special ride families, backward/disconnected
conventional endpoints, replacement and covered/inverted variants, further native
piece parameters, manual design elevation, player-facing export and substitution
choices remain on the 100% roadmap. Scene assets cannot establish AArch64/Brick
behavior or sustained routing/performance without further evidence.

Recommended next milestone: complete native track editing and design authoring,
with backward/selected geometry recovery, replacement/parameters, manual design
elevation/export and asset-backed Mini Golf/transport/water operation validation.
Do not redo the delivered maze, design, ride-management, staff, guest or construction
work. The complete next prompt accompanies the capture deliverable and final report.
