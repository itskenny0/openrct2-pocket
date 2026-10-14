# Native design authoring and advanced editing — incremental checkpoint

Goal: 100% ordinary OpenRCT2 gameplay on controller-native 512×384 TrimUI Brick.
Construction remains PARTIAL. No missing ordinary feature is removed from the roadmap.

## Phase 1 — player-facing native export

Dashboard → Rides → Inspect → Save design opens the existing UTF-8-aware 32-code-point
keyboard. OK creates an explicit Cancel-first export/overwrite review. A confirms
only the selected choice; B returns to naming/inspection without writing. Existing
held-input handoff consumes one action per transition. Full-width review text shows
the destination basename. Successful export returns to inspection with filename feedback.

Eligibility preserves desktop Ride::saveToTrackDesign: tested flag AND native ratings
are required; native design creation must succeed. No incomplete-ride shortcut exists.
Ride identity retains generation, type, object, build date and location. Each review
and write recreates the native design, compares actual native T6Exporter bytes,
status and source name, and rejects changed/departed/reused sources. The payload
includes native geometry, vehicles, operating settings, appearance and statistics.

Names retain existing UTF-8 and delete whole code points through TextEntry. Empty,
long, control/path-symbol and trailing-dot/space names reject. Native user track
folder and native TD6/TD7 extension are used. Symlink/nonregular destinations reject.
Overwrite requires a fresh review with the existing file bytes unchanged. Export
writes a unique temporary file, revalidates source/destination, then renames it.
Native TrackRepositoryScan refreshes the picker. Repeat confirmation rejects; menu
close and existing load/exit tool teardown clear all transient authoring context.
No simulation/accounting state is changed and no Pocket gameplay state is serialized.

Validation: Release GUI/CLI build PASS; RideConstructionTests five-family native
query/execute/ghost/cost/refund/identity/manual-height/save regression PASS; CTest
input/launcher/save preservation 3/3 PASS. Expanded asset-backed DesignCapture PASS:
native testing produces tested status and ratings, then real export/import round-trip,
UTF-8/unsafe names, repository refresh, Cancel-first/held-A controller workflow,
repeated execute, overwrite cancellation, stale destination, native appearance-action
source change, reused identity, menu cleanup and exact unchanged cash. The existing
design placement/research/access/testing/save-reload suite also passes. No live
ratings, ownership, cash or research is patched to obtain export eligibility.

## Source preservation and publication

Restored local checkout was db96e886864a04bfbc60398493f59a221c68c55d (staff checkpoint).
49 unmodified tracked files were updated from canonical 0ff46584fe0d3689c081ad2b28c0952bba70d23a
through authenticated GitHub blob reads; each blob and the full staged tree matched.
Local synchronization commit 7ab997b preserves prior local history; its tree is
exactly e84ead2956618c94d5526db887ae5254613bfe99. Esperanto remains untouched and excluded.
Phase publications use canonical native parents, exact source-tree equality and
expected_sha/force=false guarded fast-forward updates. Phase SHA table follows.

## Phase 1 limitations (superseded in part by phases below)

At the phase 1 checkpoint export had no scenery selection. Manual individual/area scenery selection,
substitution choices, covered/inverted/alternative variants, seat rotation and other
special parameters, backward construction and selected-block replacement remain
ordinary gaps. Native maze/conventional placement, manual elevation, disconnected
forward continuation and selected speed/removal workflows are preserved.
Asset-backed export uses the privately supplied African Diamond Mine scenario,
Data/ObjData/Tracks. Captures are production native X8 viewport plus Pocket component
renders, not complete SDL gameplay frames or physical Brick captures. AArch64,
physical Brick, sustained routing and per-family operation remain unverified.
Private archives, parks and generated designs are excluded from publication/delivery.

## Phase 2 — native nearby scenery authoring (partial desktop parity)

Save design now opens Design scenery: Name/export, Select native nearby, No scenery,
then a paged native-object roster. D-pad selects; A toggles inclusion (+/-) for one
native descriptor. The actual selected count appears in options and final review.
The native nearby helper selects normal paths, this ride's queues, small scenery,
walls and large scenery within its own native distance/eligibility rules. Large
scenery aggregation/deduplication and path legacy mapping remain native. Unsupported
native object formats are not independently serialized by Pocket.

TrackDesignSaveSelectNearbyScenery and CreateTrackDesignScenery supply native
selection, relative rotation/height, size limits and TD6/TD7 conversion. Only copied
object/coordinate/flags/colour descriptors survive menu transitions. Each action and
export recreates the full native nearby set; changes even to an excluded entry reject
the old selection. Individual selection changes are locked during review. Native
selection pointers/descriptors are cleared synchronously after collecting/converting,
including failures. An active desktop scenery selection is rejected, not overwritten.

Release GUI/CLI PASS; expanded real-asset DesignCapture PASS: actual 276-entry nearby
selection, one-entry exclusion, native scenery export/import exact membership,
out-of-range rejection, reviewed draft lock, native colour-action stale rejection,
cleanup and controller paging/toggling/Cancel-first/overwrite. All seven new 512×384
production-native viewport/component captures inspected: readable full filename,
counts, gold focus and dense last-page scenery; no clipping/overlap/footer collisions.

Existing native missing-scenery placement warnings/omit option, automatic/manual
height, scenery ghosts and access recovery remain unchanged. Vehicle substitutions
are not exposed: the current strict action requires the exact loaded/researched
vehicle, so a safe substitution review/action dependency remains. Arbitrary scenery
outside native nearby selection, spatial selection/overlay/locate, banners/furniture
not supported by native design export, and per-family scene round-trip coverage
remain precise ordinary scope/validation gaps. No full design-authoring parity claim.

| Phase | Canonical | Local | Matching tree |
| --- | --- | --- | --- |
| Tested/rated export | 90480687ceb6e7c7b05ec2e60aa6e69de498c917 | 5bb23969ecba119ddc4e75b33eda5bd4c53a2753 | 85cbd89213963053c210a0022f8b138933057b5c |

Nearby-selection publication will be recorded in the following validated checkpoint.

## Phase 3 — native seat-rotation placement (partial parameter parity)

Applicable rides advertise hasSeatRotation through native RTD. Select → Track pieces
now offers Seat -, Seat + and Seat default (4); values are strictly 0–15. Existing
bank/chain/speed and Brick camera utilities remain. Unsupported rides/pieces do not
show these options. Desktop's TD6 shared-bit rule is mirrored: ordinary brake/booster
speed pieces omit seat controls; block brakes and newer native pieces may support both.
The displayed value is the native rotation index, not an invented degree conversion.

Native SequentialTrackPlaceAction visits/serializes the immutable placement value,
rejects unsupported/out-of-range parameters and forwards it to native TrackPlaceAction
for both query and execute. TrackPreview uses the same value; changing it removes the
old ghost. Busy/queued construction rejects draft changes. Native map elements and
park saves own persistence. Selected block snapshots and removal/speed actions now
also retain/revalidate native seat baselines.

Release GUI/CLI PASS; RideConstructionTests five-family existing regression PASS.
SeatRotationTests PASS against real Multi-dimension native actions: default/bounds,
valid versus invalid strict action, queried cost, ghost without cash, station/straight
native stored values, busy lock, held-A picker handoff, unsupported family/shared-speed
visibility, stale selected seat/action rejection and native save/reload. Initialized
64×64 native scenario fixture uses private original sprites for two production viewport/
component captures; these are labelled fixture captures, not a live scenario operation.
Both inspected for controls, focus, clipping and readability.

Covered/inverted/alternative variants, selected seat changes and other special
parameters remain ordinary gaps. Desktop selected-seat editing calls
GetTrackElementOriginAndApplyChanges with TrackElementSetFlag::seatRotation, rather
than an existing validated seat action. Pocket does not copy that direct-write route.
A dedicated native validated action or removal/rebuild workflow is required.
Seat placement does not establish Multi-dimension circuit/operation/guest routing.

Phase 2: canonical 086bd7e248307a332a4354e870256028d1849d72;
local f54788d9384eb5cd4d0bfc9ab71507429c03fce7;
matching tree a82e58b15dec0abfa98427f1d03d54a58c56c121.

## Phase 4 — guarded selected-block removal/rebuild (partial editing parity)

Closed compatible conventional rides expose Replace block when a real connected
predecessor exists. Native trackBlockGetPrevious, TrackIsConnectedByShape and TED
sequence clearance normalize the predecessor across multi-tile pieces; snapshots
retain coordinates/type/speed/seat/chain/inverted data, never live map pointers.
Station blocks, inverted blocks/predecessors and missing/self predecessors are not
offered through this adapter. Maze half-cell geometry remains separate.

Review is Cancel-first, shows the native refund and explicitly states removal leaves
a gap. Confirmation revalidates selected ride/block, predecessor, native quote and
closure/object/date/view baselines. SequentialTrackPlaceAction carries the predecessor
snapshot through parameter visitation/serialization and checks it again on native
query/execute, preventing a queued stale predecessor from passing a UI-only guard.
Native TrackRemoveAction owns removal, accounting, station/circuit/guest effects.

Only successful removal resumes the existing editor from the predecessor's native
forward gap, restoring the supported original piece, speed, seat and chain draft.
The player explicitly presses A to build, or chooses another native-compatible piece.
No replacement is automatically created. Cancelling before confirmation preserves
track/cash; leaving after confirmed removal preserves the native gap/refund, recoverable
through the existing block roster after menus/load. Normal endpoint undo and explicit
selected removal remain; replacement is not an atomic transaction or dedicated undo.

Release GUI/CLI PASS. RideConstructionTests PASS: five-family multi-tile predecessor
normalization, native straight→brake→straight replacements, exact costs/refunds,
stale predecessor/quote/native-action guards, repeat prevention, disconnected recovery
and native save/reload alongside all prior coverage. SeatRotationTests SDL/native
PASS: Cancel-first replacement, unchanged cancel, explicit removal with no auto-build,
restored seat draft, explicit rebuild and persistence, genuine native invalid bounds.
Three new production fixture captures inspect cleanly (review, gap ghost, invalid
ownership/bounds feedback); prior seat captures regenerated with actual endpoint framing.

Backward construction, atomic replacement, station/inverted replacement, spatial block
selection, covered/alternative/inverted variants and broad per-family operation remain
ordinary gaps. This phase completes a usable two-step workflow, not full desktop parity.

Phase 3: canonical 4e59a4901d53026382b1d81aee0ac83100ada1e4;
local 445efae0780e69bab332b67f9dd84cf35d19e316;
matching tree 12007b7d84164142edd5deddea2bfa2eb9491b9f.


## Final implementation publication and validation

Phase 4: canonical 9777437e90ef2175e23e862584d236a7e32abbe0;
local e2e1fd69e8f2a7bfea3b046b19866d912dcfed96;
matching tree 2424175695a4c223cc9fd290c4bc8fbcf372f2ff.
All four implementation phases were published independently before subsequent work.
The final documentation-only publication is identified in the delivered manifest and
final report; it does not change the validated implementation.

Final Release GUI/CLI build PASS. Nine native/SDL suites PASS: RideConstructionTests,
MazeConstructionTests, MiniGolfConstructionTests, ConstructionTests, PathActionsTests,
StaffCompletionTests, GuestCompletionTests, PresentationLifecycleTests and
SeatRotationTests. Input/launcher/save-preservation CTest checks PASS 3/3.
Asset-backed DesignCapture, RideConstructionCapture, MazeConstructionCapture and
CustomCoasterCapture PASS using privately supplied Mines of Africa assets. The ride
capture validates Stand-up Roller Coaster / Ostrich Trains controller construction.
The Wooden regression observes natural queue use, not sustained riding/throughput.
No live research, cash, ownership or ratings were patched to obtain these results.

## Delivered capture set

OpenRCT2-Pocket-Design-Authoring-Captures.zip contains twelve genuine 512×384 PNGs
and a checksum/dimension/provenance manifest. Screens 01–07 show naming, native
export/overwrite review, completion, nearby selection, individual exclusion and dense
scenery paging in the actual African Diamond Mine scenario. Screens 08–12 show seat
preview/picker, removal/rebuild confirmation, the gap preview and native invalid
placement in an initialized Multi-dimension native fixture with private original
sprites. These are production native viewport plus Pocket component captures,
not full SDL gameplay frames. Every delivered image was inspected for clipping,
overlap, focus, controls and HUD collisions. No material presentation defect remains.
No private archives, parks or generated test designs are packaged.

## Remaining ordinary gameplay gaps

Construction remains PARTIAL toward the explicit 100% ordinary gameplay goal.
Arbitrary outside-nearby/spatial scenery selection, placement substitution choices,
covered/inverted/alternative variants, selected-seat changes and other special
parameters remain. Backward construction, atomic replacement, station/inverted or
no-predecessor replacement, spatial block selection and broader per-family operation
are still missing. Native export format limitations remain explicit; unsupported
banners/furniture are not serialized through an invented format. AArch64, physical
Brick, sustained special-family guest operation and broad routing remain unverified.
The highest-value next milestone is native backward editing, ride variants and
representative family operation, preserving all delivered authoring workflows.

Advanced follow-up: ADVANCED-RIDE-EDITING-COMPLETION.md delivers conventional native
rear-gap/backward core, covered/inverted variant eligibility and placement, Flying
inverted-start stations and representative native Monorail/Dinghy/Flying circuit,
access/Open/persistence validation. Earlier missing statements are superseded only
for this scope. Complex/diagonal coverage, positive variant transitions, other special
parameters, atomic/station/inverted replacement and sustained guest use remain PARTIAL.

## Save/load/file browser follow-up — 2026-10-14

Native authoring/export/scenery/overwrite remains unchanged and asset-backed
DesignCapture passes again. SAVE-LOAD-BROWSING-COMPLETION.md adds repository/folder
file inspection and native placement return with stale file/descriptor guards;
32-code-point UTF-8-retaining naming and native authoring extension/atomic publication
policy remain. It does not close the unrelated substitution/flat-design gaps.
