# Ride Construction Breadth and Track Editing Completion

Goal: 100% ordinary OpenRCT2 gameplay in controller-native Pocket at 512×384.
Continues canonical `1818fb28daa62309710880a7e9c353ddbf6915ed`; local safety
history and equivalent canonical source trees are preserved. Esperanto is untouched.

## Phase 1 — conventional native track families

Build → Rides lists loaded/researched object/type pairs with native type names,
including conventional coasters and transport/water tracked rides whose descriptors
have regular end-station geometry. Each explicit picker identity retains the native
ride type; multiple native types on one object do not silently choose another type.
Existing Wooden picker keys, flat rides and construction shortcuts remain valid.

TrackConstructionSupport shares descriptor/entry/research eligibility between UI
and native sequential action. Maze and single-piece flat footprints stay separate.
Regular station recovery/entrances and RideAccess now accept these descriptor-backed
families. Native station, vehicle support, funds, height, ownership, clearance and
accounting remain authoritative. No objects are loaded or research unlocked in play.

The piece picker now admits all native TED groups supported by the selected loaded
vehicle and ride descriptor, with matching incoming pitch, roll and diagonal shape.
Native inversion, helix, photo and other conventional special-piece metadata replace
the former non-inverting whitelist. Maze/flat footprint types are explicitly rejected
by the UI and guarded sequential action. TrackPlaceAction/TrackRemoveAction still own
geometry and cost; Pocket supplies no independent shaping algorithm or track route.

Controls retain A Build; B native endpoint Undo; L1/R1 compatible piece; L2/R2 slope
or station height; X bank; Y chain/station turn; Select paged pieces/Return to park;
Select picker → Camera controls, then L1/R1 zoom and X camera rotation;
Select picker → Piece controls returns. Legacy L3 mapping remains available. Start preserves menu recovery.
Extended native pitch/roll pieces remain selectable through the paginated picker.

Phase 1 validation: Release GUI/CLI; new public-metadata RideConstructionTests builds
Wooden, Looping, Corkscrew, Twister and Monorail through native actions. Stations,
straights, multi-tile turns, compatible vertical-loop approach/inversion, query,
pending-repeat rejection, exact costs/refunds, unsupported maze geometry, ghost
cleanup, resume and save/reload pass. Full asset-backed native/SDL PathActionsTests
passes. Original asset-backed CustomCoasterCapture explicitly selects Wooden from
the broader picker, builds/opens its circuit/access and proves natural guest use,
accounting and save/reload without any guest teleportation or state patches.

Phase 1 published: `fd35c9b06a80cdebf057e18a9bc078b6a6621022`.

## Phase 2 — native saved designs and scenery

Saved designs now enumerate native repository files for every compatible conventional
ride family, with exact loaded/researched vehicle/type eligibility. No vehicle
substitution is invented. Maze and single-piece flat designs remain separate gaps.
Native import, thumbnail rendering, TrackDesignAction, native design placement,
orientation, height and station/access recovery remain authoritative.
L2 excludes design scenery; R2 includes it. Native scenery availability is probed
through TrackDesignPlaceVirtual; unavailable scenery is explicitly reported and
omitted according to native desktop fallback. A opens a Cancel-first exact-cost
review; scenery cannot change within that review. Ghost removal includes native
track and scenery. No new serialized park state is introduced.

Five-family tests export actual action-built rides through native T6Exporter,
import them, query scenery off/on, verify extra native cost and real scenery
placement, prevent duplicate confirmation and check ghost cleanup and save/reload.
The original asset-backed Wooden design regression explicitly selects its exported
fixture from the expanded list; picker identity assertions accept the real list size.
Phase 2 published: `2fa67fbd3105e52b2787d7d803860ae867e14c28`.

## Phase 3 — selected native blocks and recovery

Closed conventional rides offer Edit track blocks from their park inspection and
Dashboard inspection. The paged roster enumerates real non-ghost sequence-zero
blocks and derives each origin from native TED clearance/rotation metadata.
Rows show the shared piece name and tile/height. Select a block to locate it,
review its native removal refund, or edit supported brake/booster/block-brake
speed in native two-unit steps. Both reviews default to Cancel. A confirms,
B returns without execution; the menu closes before the single queued action.
Select → Camera controls now provides camera mode without requiring an analog
stick button. Existing station, zoom/rotation and return-to-park controls remain.

TrackEditing revalidates generation-aware RideManagement identity and exact
block type/origin/direction/height/chain/inversion/speed. The serialized native
SequentialTrackPlaceAction adds selected removal and speed operations with
ride object/build date/type/overall-view, closed status, research, bounds,
block baselines, no-money mode and quoted refund checks repeated at execution.
Native TrackRemoveAction and TrackSetBrakeSpeedAction own all map/state changes.
Speed editing checks native ownership and limits before calling the native action.
Endpoint-only Undo retains its original no-following-block guard; selected removal
can remove a middle block. The native gap then becomes recoverable through the
existing forward Continue track workflow. No independent track geometry, map
insertion/removal, simulation algorithm or serialized Pocket park state is added.
Menus retain their own block draft while navigating; stale or loaded/departed
identities are rejected. A menu-owned pause is released when entering this
construction workflow; externally paused construction still follows native rules.

Validation: Release GUI/CLI; native five-family/design/scenery tests; selected
middle removal, exact/stale refunds, repeated removal rejection, brake bounds and
baseline invalidation, unchanged speed-edit cash, gap recovery and save/reload.
Full asset-backed PathActionsTests (including Ride Management, RideAccess and SDL),
StaffCompletionTests, GuestCompletionTests, ConstructionTests and all three CTest
input/presentation/save checks pass. Original Mines of Africa Wooden regression
still proves native circuit/access, opening, natural guest use and save/reload.
Production RideConstructionCapture in Factory Capers proves non-Wooden Junior
Coaster picker/editor/design preview, both Cancel-first block reviews, confirmed
native speed change, SDL held-A suppression, Select camera handoff, entrance/exit
completion and save/reload. The same asset-backed workflow also passes for the
Looping Coaster in Build your own Six Flags Magic Mountain, including its inversion
preview. Native station height is raised explicitly and the driver selects sites
through native ownership/clearance queries; it never terraforms or edits ownership
to force success. PresentationLifecycleTests also passes SDL HUD/tool/dashboard,
load/shutdown cleanup and staff controls. No object/research or live simulation
patches are used.

## Captures and remaining ordinary parity gaps

The new capture driver renders native viewports and actual production Pocket
pickers, panels and menus at 512×384. Captures include category/object choice,
station/editor, native compatible pieces, design preview/review, existing block
selection, speed/removal reviews, dense block roster, entrances/exits and native
invalid placement. Original assets remain private; only PNGs and manifest are
packaged. Capture inspection and final published commit are recorded below.

This is substantial breadth and selected-block editing, **not full ordinary track
editing parity**. Backward building, explicit choice among multiple disconnected
endpoints, drag/map block highlighting, direct replacing/rebuilding a selected
block, lift/alternative-track changes on existing blocks, covered/inverted
variant controls, seat rotation and special-piece parameters beyond speed,
native UI design export/scenery selection, maze/special footprint construction,
maze/flat saved-design adapters and user-directed vehicle substitutions remain.
Availability is exact loaded/researched native objects; unsupported designs are
filtered, not parsed or silently substituted by Pocket. Broader conventional
families are descriptor-offered but require individual validation of ride-specific
station/vehicle geometry and operation. Goal remains 100% ordinary gameplay.
New family operation/guest use is not claimed from construction/capture tests;
the natural guest proof is the existing Wooden regression. AArch64 builds,
performance and physical Brick controls/operation remain unvalidated.
Manual saved-design elevation control and placement ghosts for station entrances/exits
remain ordinary UI gaps; this milestone retains native automatic design height and
existing station placement query/execute behaviour.

## Published phases and equivalent local safety history

| Phase | Canonical published commit | Local safety commit (equal tree) |
| --- | --- | --- |
| Descriptor/native family breadth | `fd35c9b06a80cdebf057e18a9bc078b6a6621022` | `07a674f31d157b1eaecb2a3a8dbe1219f1c3a545` |
| Saved designs/scenery | `2fa67fbd3105e52b2787d7d803860ae867e14c28` | `36a10ab8e671f75e70cd8987bdf5f03958bdbf05` |
| Selected blocks/speed/recovery | `66daab28ccc55dc03d660acd00b0d02afef76377` | `4490ef67ff8854ca63e78cd468b7d9c0589ecb05` |

Each canonical commit has the previously validated canonical parent. Publication
checks the remote branch head, constructs the incremental source tree, asserts
exact equality to the local safety HEAD tree, and updates the branch with
expected_sha and force=false. Existing local history is retained. Esperanto is
excluded from all staging and canonical publication. The final presentation/docs
follow-up uses the same guarded process; its exact SHA is in the capture manifest
and final report rather than a self-referential commit hash embedded in this file.

## Delivered capture package and visual review

OpenRCT2-Pocket-Ride-Construction-Captures.zip contains 29 genuine 512×384 PNGs,
a checksum/dimension/provenance manifest and README. LoopingCoaster has 15 captures
from Build your own Six Flags Magic Mountain; JuniorCoaster has 14 from Factory
Capers (no unsupported inversion screen). These are asset-backed native viewports
with production Pocket components, rendered using the SDL/X8 capture driver.
They are not full desktop screenshots, mockups, composited UI or invented values.
All displayed prices, native errors, object lists, track positions and speed values
come from the actual scenario/actions. The incomplete native test design is exported
by T6Exporter for preview validation, not presented as an operating circuit.

| File prefix | Actual implemented screen |
| --- | --- |
| 01 / 02 | Native loaded/researched object/type picker; custom versus saved-design mode |
| 03 / 04 | Elevated station ghost and sequential native editor |
| 05 / 06 | Native metadata-compatible piece picker; valid Looping vertical-loop ghost |
| 07 / 08 | Exported non-Wooden native design ghost and Cancel-first cost review |
| 09 | Real existing block roster with tile/height and shared piece names |
| 10 / 11 | Native speed draft review; Cancel-first native removal refund |
| 12 | Dense six-control brake operations with actual speed and block location |
| 13 / 14 | Native entrance and exit completion contexts, real station/access construction |
| 15 | Genuine invalid ownership/bounds feedback with no placement ghost |

Visual review checked every selected capture for clipping, overlap, contrast,
focus, footer readability, HUD collisions and panel positions. The selected-piece
icon had crowded the native loop direction; the gold row now gives its text the
full available width. The complete native inversion ghost is framed visibly in
the regenerated capture. Long object/piece names still use the existing intentional
ellipsis/picker pagination; no panel, control or confirmation label overlaps.
Release GUI/CLI and focused native tests pass after the presentation adjustment;
both asset-backed controller/capture scenarios are regenerated and pass.
Public-source validation is reproducible with tools/handheld/run_native_test.py;
private-asset captures with tools/handheld/render_ride_construction.py and supplied
Data/ObjData/Scenarios/Tracks. Original archives, parks and designs are not published.
