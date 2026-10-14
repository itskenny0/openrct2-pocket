# Native track editing and design authoring checkpoint

The goal remains 100% ordinary OpenRCT2 gameplay on the 512×384 TrimUI Brick.
This milestone is incremental; overall construction remains PARTIAL.

## Phase 1 — selected conventional section continuation

The existing track-block roster now offers **Continue section...**. Select any
real sequence-zero block of a closed, researched, compatible conventional ride.
Native `trackBlockGetNext` and `TrackIsConnectedByShape` follow that component to
its forward gap, including disconnected components without a station. Native TED
sequence-zero clearance translates the selected origin to its actual element.
No independent track geometry, map writes or serialized Pocket markers are used.

The normal forward editor retains queried costs, native placement/clearance,
removal/refunds, circuit detection and entrance/exit recovery. Selection validates
ride identity and the block's type, orientation, height, chain, inverted and speed
baselines before menu handoff. The transient anchor uses coordinates/type, never
retained element pointers. A missing anchor ends editing instead of jumping to an
unrelated station component. Undo of an extension retains context; removal of the
anchor itself ends it safely. Menu return preserves the active editor. Load/exit
uses existing tool teardown; players reselect a real block after load.

Controls: Dashboard → Rides → Inspect → Edit track blocks → select block →
Continue section. Existing A build, B endpoint undo, Select piece/camera picker,
L1/R1 piece, L2/R2 slope/height, X bank and Y chain/orientation remain unchanged.
Existing selected removal remains Cancel-first. A continuation is not destructive.

Validation: Release GUI/CLI build; RideConstructionTests five-family native
query/execute, disconnected continuation, extension/undo, missing anchor cleanup,
stale identity, costs/refunds and save/reload. Factory Capers Junior asset-backed
RideConstructionCapture exercises real menu/controller handoff, held-A Cancel,
existing speed/removal/access/design and persistence. Captures use native X8
viewport plus production Pocket components, rather than whole SDL gameplay frames.

Remaining: backward construction, replacement, broader selected parameters and
variants, player-facing design export/scenery selection, manual design elevation,
substitution choices and full special-family operation are not yet delivered by
this phase. The native block picker remains a roster; spatial selection is not
claimed. AArch64 and physical Brick are untested.

## Publication

Base canonical: 57d36683439bb192c6210c95fb5aa441cbbd6797.
Base local: 20542113fba2b5066a4ed3a1900c327b1aae4ba4.
Base source tree: 61bbf89aef9c6642c231577e4b416e86fa115977.
Canonical/local phase SHAs will be recorded after guarded publication.
Esperanto remains a separate preserved local edit.

## Phase 2 — native placement speed parameters (partial)

Applicable native brake/booster pieces now expose X speed + and Y speed - before
placement, bounded to 2–30 native units. The panel uses native STR_VELOCITY
formatting. The exact speed travels through SequentialTrackPlaceAction parameter
visitation/serialization, cost query, ghost and native TrackPlaceAction execute.
Non-speed pieces accept only the default parameter; out-of-range values reject.
Existing selected-block speed reviews remain unchanged. The strict sequential
context now checks ride type and overall-view baseline for every operation.

RideConstructionTests verifies native placement value 12, invalid 1/31 and
persistence alongside existing selected speed baseline/refund cases. The Junior
asset-backed capture uses real X controller dispatch to choose speed 8 and renders
its ghost, price and controls. GUI/CLI Release and native/SDL checks pass.
Seat rotation, covered/inverted alternatives and other native special parameters
remain explicit ordinary gaps. This phase is not full parameter/variant parity.

Phase 1 published 4edb3e8e69665595080992b97cb8b68aee79a7b0;
local 5dc429f56d7f5efed0617d5b50a79846548b8078;
identical tree 9001e84356042ba40e38004b333fe23633221934.

## Phase 3 — manual design placement elevation (partial)

X opens compact Design options while placing a saved design. Height +/- selects
an exact offset from the native terrain-fit calculation in 16-unit increments,
bounded -128..+256. Automatic height restores the existing bounded native height
search. Manual mode does not silently search upward after an invalid query.
Native placement actions continue to own geometry, height validity, permissions,
scenery, costs and execution. The reviewed location/cost is immutable; height and
scenery controls reject during review/busy. Ghosts are removed before draft changes.
The offset is transient and resets when the tool ends, not serialized gameplay.

Select still turns the design; Y still opens designs; L2/R2 scenery shortcuts
remain. X options includes camera rotation; L1/R1 zoom and D-pad camera remain.
B returns from options without discarding the placement draft. The placement panel
and Cancel-first review show manual height. Review is still native cost guarded.

Release GUI/CLI build and RideConstructionTests pass five-family exact manual
height, bounds/invalid-height, locked review, manual query/ghost/execute/accounting
and native save/reload. MazeConstructionTests passes manual height/review locking,
automatic recovery and ghost cleanup. Junior asset-backed controller/options/
manual ghost/review/access/persistence captures pass; material truncated hint fixed.

Player-facing design export/scenery selection and substitution choices remain
ordinary gaps. Native desktop export requires tested status and native ratings;
those rules must be preserved in a future player-facing authoring adapter. Flat
and other incompatible special design categories remain partial. No export screen
is claimed or fabricated. Full Mini Golf/maze/transport/water operation is pending.

Phase 2 published c9f62be479ce2e74c53440245dcd393adf987b7b;
local c3ca773d104f5bfe381a92421e13b489c186942d;
identical tree 6b105c58aee3348612c8a89fa812dfeaf7ac91a2.

## Phase 4 — demonstrated operation and regression evidence (partial)

MiniGolfConstructionTests now builds a native station/hole-A/four-corner closed
course through the production editor and real actions, verifies every quoted
construction cost, completes native entrances/exits, queries/executes
RideSetStatusAction(open) and verifies Open survives save/reload. All five holes
in four orientations remain covered separately. This is a public native fixture;
asset-backed Mini Golf operation and sustained players are not claimed.

MazeConstructionTests queries/executes native opening and closing after access.
Factory Capers MazeConstructionCapture additionally opens the action-built maze
using its existing researched objects/ownership/cash and captures the production
ride inspector with actual Open status. No live research/cash/ownership/ride or
map-element patches are used. Opening does not prove guest routing or throughput.
Representative transport/water operation remains individually unvalidated.

The design camera regression now exercises X → Design options → Rotate camera,
asserting unchanged design orientation. A transition frame containing X and A
opens options only; it cannot create a review or adjust height. Failed selected
section handoff reports a native-compatible endpoint error instead of silently
returning. Existing cancel/menu/ghost/input paths remain covered.

### Final validation

- Release GUI and CLI targets: successful build (existing host configuration).
- RideConstructionTests: five native families, stations/turns/inversion where
  compatible, disconnected forward recovery, speed placement/baselines/bounds,
  costs/refunds, manual design height/ghost/execute and save/reload: PASS.
- MazeConstructionTests: geometry, native actions, access, disconnected selection,
  designs/manual height, opening/closing, guards and persistence: PASS.
- MiniGolfConstructionTests: all five holes/four rotations, native closed course,
  access, Open and persistence: PASS (public fixtures).
- ConstructionTests, StaffCompletionTests, GuestCompletionTests,
  PresentationLifecycleTests: PASS.
- PathActionsTests with private assets/SDL dummy: PASS, including existing
  management/construction/access/input and ghost lifecycle regression coverage.
- Asset-backed RideConstructionCapture: Junior Factory Capers and Looping Build
  your own Six Flags Magic Mountain: PASS, controller parameter/options handoff,
  speed/removal review, native design previews, station/access and save/reload.
- Asset-backed MazeConstructionCapture: Factory Capers: PASS, including native
  Open inspection. Asset-backed DesignCapture: PASS, native design/research/ghost/
  camera/cost/cancel/controller/stations/access/testing/persistence.
- Original CustomCoasterCapture: PASS, action-built Wooden circuit, native opening,
  elevated access and natural guest queue use through 30,000 ticks. This does not
  establish sustained throughput for other offered families.
- CTest: 3/3 PASS (controller input, launcher mapping and save lifecycle).

### Captures and limits

Deliverable: OpenRCT2-Pocket-Track-Editing-Design-Captures.zip, with actual 512×384
PNGs and checksum/dimension/provenance manifest. Representative captures cover
block roster, dense operations, selected continuation, placement-speed parameter,
Cancel-first removal, invalid ownership, manual-height options/ghost/review and
native maze Open inspection. Captures are asset-backed native X8 viewport plus
production Pocket components/menus, not full SDL gameplay frames. Original
archives, scenario/save files and generated TD6s are excluded. All packaged images
are visually inspected; truncated essential options hint was fixed and regenerated.

No capture of unimplemented export, replacement, backward or variants is included.
Private Data/ObjData/Scenarios/Tracks were available for the above named scenes;
Mini Golf full circuit uses public fixtures. AArch64 builds, physical Brick input,
hardware rendering/performance and whole-frame HUD interactions remain untested.

### Remaining ordinary gameplay scope

Overall construction remains PARTIAL toward 100% ordinary gameplay. Remaining:
backward and selected replacement; spatial conventional block selection; selected
seat rotation/further parameters; covered/inverted/alternative variants and their
native vehicle/station rules; player-facing tested/rated design export with
scenery selection; manual native substitution/warning choices; unsupported flat/
special design categories; entrance/exit and maze initial/segment ghosts; individual
transport/water-family operation; asset-backed Mini Golf circuit/guest use and
sustained maze use. Existing guest/staff/construction roadmap gaps remain intact.

Phase 3 published e9f256775bbc47d9ce8e720ec73bf6e7c1d831fb;
local 13084ebcf01aea36a6702d640f9314f0fe478499;
identical tree 53ebad9e0d6d12e0df15ce0a4e6c36517a010694.
Phase 4/final publication is recorded in a subsequent documentation checkpoint
and the capture manifest to avoid a commit self-reference.

## Published implementation history

| Phase | Canonical published commit | Local safety commit | Identical source tree |
|---|---|---|---|
| Selected section recovery | 4edb3e8e69665595080992b97cb8b68aee79a7b0 | 5dc429f56d7f5efed0617d5b50a79846548b8078 | 9001e84356042ba40e38004b333fe23633221934 |
| Placement speed | c9f62be479ce2e74c53440245dcd393adf987b7b | c3ca773d104f5bfe381a92421e13b489c186942d | 6b105c58aee3348612c8a89fa812dfeaf7ac91a2 |
| Exact design elevation | e9f256775bbc47d9ce8e720ec73bf6e7c1d831fb | 13084ebcf01aea36a6702d640f9314f0fe478499 | 53ebad9e0d6d12e0df15ce0a4e6c36517a010694 |
| Operation/input evidence | 0a915988a0264402ff50a161430efd9ed5b1b715 | dbc8ffbe9614263d0a313e3bc66118020c3486da | a0d4fd685289d0b83cb438b0604c395c9b1b051a |

Every canonical commit uses the current native remote parent and guarded
expected_sha/force=false fast-forward update. Each tree matches its local commit;
Esperanto is excluded and remains untouched locally. Final documentation commit
SHA/tree is recorded externally in the delivered manifest/next prompt.

## Native systems reused

Native TED sequence metadata, trackBlockGetNext/TrackIsConnectedByShape, loaded
RideObject/research/vehicle-group eligibility, RideManagement identities,
SequentialTrackPlaceAction guards, TrackPlaceAction, TrackRemoveAction,
TrackSetBrakeSpeedAction, TrackPreview borrowed native ghosts,
TrackDesignImport/GetZPlacement/PlaceVirtual, SavedTrackDesignPlaceAction native
query/execute/cost guards and provisional cleanup, native entrance/exit actions,
RideSetStatusAction, native accounting and ScenarioSave/load. No custom geometry,
simulation or gameplay save fields are introduced.

## Capture inventory

Fourteen genuine 512×384 asset-backed viewport/component PNGs are packaged:
Junior block roster; selected speed review; Cancel-first removal; dense operations;
invalid unowned placement; continuation editor; placement speed; continuation
selection; manual-height options; manual-elevation ghost; manual-elevation cost
review; Looping continuation editor and placement speed; native maze Open inspector.
The manifest records per-file SHA-256, dimensions, scenario provenance, renderer,
validation limits and latest canonical/local source tree. All were inspected for
clipping/overlap/focus/control readability; essential shortened hint regenerated.
Existing shell HUD is not drawn in these component captures, so whole-frame HUD
collision and physical-display readability remain hardware validation gaps.

Highest-value next work: native player-facing tested/rated design authoring with
scenery selection, plus the remaining backward/replacement and variant editing
adapters, with independent validated publication and operation evidence. Do not
repeat delivered selected forward recovery/speed/manual elevation.

Follow-up: DESIGN-AUTHORING-COMPLETION.md delivers player-facing native tested/rated
export with safe naming, overwrite review, source/file guards and repository refresh.
Earlier export-gap statements describe that checkpoint and are superseded only for
this delivered scope; scenery authoring and other editing/variant gaps remain.

Native authoring follow-up through canonical 9777437e90ef2175e23e862584d236a7e32abbe0
adds tested/rated native export, nearby scenery inclusion/exclusion, bounded native
seat placement and guarded two-step selected-block removal/rebuild. Earlier export,
seat and replacement gaps are superseded only to this scoped extent. Backward,
variants, arbitrary scenery/substitutions and full desktop parity remain PARTIAL.
See DESIGN-AUTHORING-COMPLETION.md for final tests, controls and capture provenance.

Advanced follow-up: ADVANCED-RIDE-EDITING-COMPLETION.md delivers conventional native
rear-gap/backward core, covered/inverted variant eligibility and placement, Flying
inverted-start stations and representative native Monorail/Dinghy/Flying circuit,
access/Open/persistence validation. Earlier missing statements are superseded only
for this scope. Complex/diagonal coverage, positive variant transitions, other special
parameters, atomic/station/inverted replacement and sustained guest use remain PARTIAL.

## Selected-track editing follow-up — 2026-10-14

See SELECTED-TRACK-EDITING-COMPLETION.md. Ordinary orthogonal no-predecessor,
station and supported inverted/covered selected blocks now have exact-context
Cancel-first native removal and explicit fixed-origin rebuild with parameters,
accounting and counted access deletion/recovery. Connected four-family station/
access/Open/save evidence and inspected production captures are recorded there.
This supersedes those specific pending entries above; atomic restoration, direct
selected-seat editing, diagonal/special/exhaustive geometry, positive transitions,
design substitution/arbitrary scenery and hardware/sustained-guest gaps remain.
