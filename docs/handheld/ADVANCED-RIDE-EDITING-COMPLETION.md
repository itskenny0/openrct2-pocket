# Advanced ride editing — incremental completion checkpoint

Goal: 100% ordinary OpenRCT2 gameplay through controller-native 512×384 Pocket.
The overall construction domain remains PARTIAL; ordinary gaps stay on the roadmap.
Base canonical 3edb5322cd2b4b81525c87749d325992947211bb and local
a0d623778878c758a0c12e0362bf19ebc5dd0fa0 share tree
7573d924f18655b876caccd49435b49a05920916. Esperanto is preserved and excluded.

## Phase 1 — native backward endpoint construction

Dashboard → Rides → track block roster → selected block → Build backward resumes
that component at its native rear gap. Native trackBlockGetPrevious and
TrackIsConnectedByShape traverse existing sections; TED end coordinates produce
placement origins using desktop backward construction semantics. Multi-sequence
predecessors are normalized with native sequence clearance, never independent
track geometry. Only coordinate/type/parameter snapshots survive menu transitions.
Closed compatible conventional ride/object/research rules remain required; maze
half-cell construction is separate. Inverted sources remain withheld until a
validated variant route is available. Closed circuits cannot resume a nonexistent gap.

A queries and explicitly constructs one native piece; B uses existing native endpoint
undo/cancel semantics; L/R choose pieces, L2/R2 choose the outward slope, X/Y retain
bank/chain/speed and Select opens pieces/seat/camera utilities. The heading explicitly
reads Build backward. Bank/slope labels describe the outward start in backward mode.
Menus preserve context; ordinary load/exit teardown discards transient tools and
subsequent block selection reconstructs the native context. Removal of the selected
anchor ends construction rather than jumping to an unrelated component. No Pocket
construction marker is saved. Picker cycling now excludes utility keys before numeric
piece parsing, protecting eligible seat-rotation rides.

SequentialTrackPlaceAction carries backward direction plus immutable source speed,
seat, chain and inverted baselines through parameter visitation/serialization.
Query and Execute revalidate ride type/object/date/view/closure/research, source
geometry, rear gap, pitch/bank, incoming/outgoing connections, native quote and funds.
TrackPlaceAction/TrackRemoveAction own map changes, station handling, clearance,
ownership, costs/refunds and save persistence. No automatic construction or direct
map mutation is introduced. Native undo remains an explicit single press.

Release GUI/CLI PASS; expanded RideConstructionTests PASS (native backward query,
execute/repeat/cost, stale source parameters, multi-tile turn/undo, disconnected rear
recovery and native save/reload). Expanded SeatRotationTests SDL/native PASS (real
block roster and held-A handoff, native ghost/price, straight, multi-tile turn,
down-to-flat slope transition, undo and return-to-park). The attempted upward rear
transition is natively obstructed by terrain in this fixture; the permitted downward
transition tests elevated geometry without modifying terrain/ownership/cash live.
Production 512×384 native fixture captures record block selection, backward preview,
multi-tile preview and slope context. Asset-backed family operation is a later phase.

Remaining gaps: covered/inverted variants, backward diagonal/complex-family exhaustive
coverage, selected-seat controls, atomic/station/inverted replacement, spatial selection,
arbitrary export scenery/substitutions and broad operation/hardware validation.
Publication SHAs are recorded in subsequent checkpoints and the final manifest.

## Phase 1 publication

Canonical cf29892a2ebf51badcd8d3d95f1edcd6adf01142; local
1b101fbc6ab16a6918b64d80134b585f4e5c94d2; matching tree
4494b6fffe333252e3b31fab2598e110a8d12796. Guarded fast-forward publication
preserves canonical native parent history and all local history.

## Phase 2 — native covered and inverted variant placement

Covered availability follows TED alternativeType back to the original piece and
native Covered paint groups plus loaded vehicle sprite support. The ordinary
vehicle/group check cannot validate covered descriptors solely through their flat
placeholder group. Native mappings are cached from immutable TED metadata. Select
picker offers Covered track / Open track only when that alternative connects.
The native covered type itself supplies costs, footprint, query, ghost and execution.

Inverted variants require the native hasInvertedVariant flag, appropriate native
inverted paint groups, vehicle sprite groups and TrackGetActualBank2 compatibility.
Native startConstructionInverted now supplies initial station defaults and the
correct station-group eligibility (Flying previously failed regular station filtering).
Existing source variants are retained through forward and backward recovery. Draft
variant toggling is offered only when a native-compatible alternative piece exists;
otherwise it is withheld. Native supported transitions remain in the piece picker.
No native vehicle support or track groups are invented/unlocked. Selected blocks
show native variant and applicable seat index. Existing selected removal/rebuild
allows explicit seat changes through the existing picker after removal; no direct
selected-seat flag write or new atomic replacement is claimed.

SequentialTrackPlaceAction serializes the placement inverted flag and passes it
unchanged to native TrackPlaceAction query/execute. Preview identity includes it.
Connections on both sides use native actual-bank calculation, including sources
and closing connections. Source speed/seat/chain/inverted baselines are now guarded
for forward construction as well as backward construction. Busy draft changes
reject and clear previews only after successful changes. Removal uses the real
source variant even if the player changed the outgoing draft. Native saves retain
covered types and inverted/seat flags; Pocket serializes no gameplay state.

Expanded SeatRotationTests cover Dinghy covered controller utility/held-A,
query/ghost/execute, return to open draft, native removal/rebuild, unsupported inverted
requests, actual Multi-dimension native inverted source placement, forward/backward
extension/undo, Flying inverted-start station/straight and native variant persistence.
The Multi-dimension object lacks a compatible normal transition through its actual
sprite groups at the tested endpoint: the utility is withheld and rejected, not
advertised as usable. Native descriptors/actions remain authoritative.

Remaining: exhaustive covered slopes/curves, positive variant-toggle/transition
coverage for other eligible objects, selected-seat direct adjustment, other special
parameters, atomic/station/inverted replacement, diagonal/complex backward coverage,
arbitrary design scenery/substitutions and broad operation/hardware parity.

## Phase 2 publication

Canonical f79d077682b19caae380b1f807c9177c75c6155e; local
eac254e1de9fd1f1977e49fa7ce30f549f39c984; matching tree
bca2ea2d961798252fa9120b0022662b818084dd. Release GUI/CLI,
RideConstructionTests and expanded SeatRotationTests PASS before publication.

## Phase 3 — representative native family operation workflow

FamilyOperationTests initializes legal native 64×64 scenario metadata and loaded
Monorail (mono1), Dinghy Slide (ding1) and Flying (bmair) objects before any actions.
It then uses the production controller construction adapter and native GameActions
to build four-tile stations and closed circuits, including a real Dinghy covered
segment and Flying inverted track. Native entrance/exit recovery places actual
access elements; FootpathPlaceAction builds queue/normal landings and native
RideAccess inspection confirms both attachments. It does not claim a connection
to a park entrance or natural guest routing in the initialized fixture.

Each family passes native RideSetStatusAction query/execute to Open, exact placement
accounting and native save/reload with status retained. The empty Monorail vehicle
changes native coordinates over 4096 simulation ticks. Dinghy and Flying evidence
is limited to valid construction/access/Open/persistence; a flat coaster/water circuit
is not evidence of sustained running, ratings, guest throughput or ride excitement.
No guests, positions, ride status, vehicle flags, ownership, cash or research are
patched live to force these new workflows. Native fixture ownership/cash/research
initialization precedes construction. Private original sprites are used only for
production viewport/component captures; no private park/design/archive is published.

This validation closes demonstrated station/variant/access/Open workflow evidence
for these representative objects. It does not complete every transport/water/inverted
family, cover every special piece or establish live scenario guest use. Existing
Mini Golf/maze opening and original Wooden natural queue use remain regression
contracts. Full construction still remains PARTIAL toward 100% ordinary gameplay.

## Phase 3 publication and final validation

Canonical 60f1390003dfe86f4c638f66b3cf52158b132793; local
d29bf6809bb4d5a6bd28905a7a3909d4133d8373; matching tree
785de52feccfc6ceb65fff0634880b4368a94098. Each coherent phase was validated and
published before the next. Final documentation publication is identified in the
external capture manifest/report to avoid a self-referential commit hash.

Final Release GUI/CLI PASS. Native/SDL suites PASS: RideConstructionTests,
SeatRotationTests, FamilyOperationTests, MazeConstructionTests,
MiniGolfConstructionTests, ConstructionTests, PathActionsTests,
StaffCompletionTests, GuestCompletionTests and PresentationLifecycleTests.
FamilyOperationTests also passes without original graphics. CTest input/launcher/
save preservation PASS 3/3. Private African Diamond Mine DesignCapture,
RideConstructionCapture, MazeConstructionCapture and original Wooden
CustomCoasterCapture PASS. Existing Wooden natural queue evidence is retained;
no sustained riding/throughput is inferred. New family tests use native actions
following scenario fixture initialization; legacy regression restriction probes
retain their existing reversible state checks and are not new simulation bypasses.

## Capture package and visual inspection

OpenRCT2-Pocket-Advanced-Ride-Captures.zip contains seventeen genuine 512×384 PNGs,
a checksum/dimension/provenance manifest and next prompt. 01 selected block with
actual variant/seat; 02 backward straight; 03 backward multi-tile turn; 04 backward
slope; 05 inverted source preview; 06 covered utility; 07 covered ghost; 08 Flying
inverted station; 09 Flying dense native piece picker; 10–12 Monorail/Dinghy/Flying
completed circuits; 13–15 actual native Open inspectors; 16 native ownership error;
17 Cancel-first native removal/rebuild refund review. All are initialized native
fixtures with private original sprites and production native viewport/Pocket
component rendering, not full gameplay frames or live scenario family operation.
Only successful final fixture screens are packaged. Legacy failed-attempt captures
are excluded. No mockups, composited fake UI, private archives, parks or designs.

Every image was inspected for labels, focus, overlap, contrast, controls and panel
position. Dense block controls paginate; picker ellipses use the selected full-width
detail label. Native costs, refund and Open/profit information come from real state.
Whole-frame HUD collision, physical 3.2-inch readability, AArch64, input mappings,
performance and sustained family guests remain separate validation requirements.

## Precise remaining scope and next milestone

Backward core is delivered for conventional native gaps; exhaustive four-orientation,
diagonal/complex slope/bank/section coverage remains. Covered/inverted core placement
and native inverted-start stations are delivered; positive manual variant transitions
and exhaustive covered slopes/turns across supported vehicles need dedicated coverage.
Direct selected-seat/special parameter editing, atomic/station/inverted replacement,
spatial block selection, arbitrary export scenery, vehicle substitution and other
special ride families remain ordinary gameplay gaps. Maze/access ghosts and existing
park construction, staff and guest gaps remain on their previous checkpoints.
No gap is reclassified as unnecessary. Overall construction remains PARTIAL toward
100% ordinary gameplay.

Highest-value next subsystem: scenery and path construction parity, including native
banner/style controls, useful loaded/researched category filtering and bulk removal,
connected path starts from access endpoints and recoverable native undo/path networks.
Preserve all delivered ride editing/authoring, family evidence and management systems.
