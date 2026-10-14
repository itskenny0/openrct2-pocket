# Native selected-track editing — incremental checkpoint

Repository source and this checkpoint supersede older pending lists only for the
work explicitly delivered below. Overall construction remains PARTIAL.

## Phase 1 — native block, station and access inspection

Dashboard → Rides → closed ride → Edit track blocks → selected block → Inspect
connections / station. D-pad pages the bounded native detail rows; A on Back or B
returns to that block, then to its stable roster identity. Inspection covers native
no-predecessor blocks, ordinary station blocks and existing inverted/covered blocks.
Direction, native sequence count, shape-connected predecessor/successor and station
index/length/entrance/exit coordinates come from actual native elements and stations.
Disconnected ends and missing access are explicit. Removed or changed selections
recover to the roster instead of dereferencing saved tile pointers.

Removal/speed reviews now capture exact typed ride track/station/access geometry,
vehicle object identifier and adjacent access path geometry. The transient review
vector is checked again inside SequentialTrackPlaceAction Query/Execute. It contains
no hash, padding, live pointers, moving-train/queue counters or park-save extension.
The review still checks native ride type/object/build date/view, research, selected
parameters and actual quote. A stale context rejects without native removal or cash
change. Rendering, selecting, inspecting and querying are read-only.

### Native audit and failure boundaries

Desktop RideConstruction uses native TrackRemoveAction and TrackPlaceAction. Native
TrackRemoveStationElement can split a station, convert neighbouring begin/middle/end
station types and renumber native station/access references during validateStations.
Its query checks available station capacity; it does not provide a replacement
transaction. TrackRemoveAction Execute removes sequences progressively; malformed
maps can fail after earlier native mutation. No universal rollback guarantee exists.
Pocket therefore must describe removal and subsequent construction separately.
Access changes remain visible and must use native RideAccess recovery, never direct
station/entrance writes. Merely inspecting a station never alters its access.

### Phase 1 verification

Release GUI/CLI build, focused initialized native/SDL SelectedTrackEditingTests and
CTest results are recorded with the validated publication below. Further replacement,
variant and end-to-end evidence belongs to subsequent sections, not this phase.

Phase 1 PASS: Release GUI/CLI; SelectedTrackEditingTests with actual original assets
and SDL dummy UI (semantic controller frames); CTest 5/5 input, launcher, save
preservation, file browsing and named save. Native station extension changes the
ride's overall-view identity; the fixture deliberately reacquires the new native
identity and proves the old review rejects. Physical controller results unconfirmed.

## Phase 1 publication

Canonical `04bacd08744f91c5f96f298f7b60d815de16ff23`; local safety
`d9dd9314b2ce0d9542ae0c1d928f588b83914528`; identical complete tree
`690298351886988fb1876cfe10e20d0bc6fbb71f`. Every uploaded blob and complete tree
verified; expected_sha / force=false update and branch readback succeeded.

## Phase 2 — explicit native removal and exact-origin rebuild

Replace block now admits ordinary orthogonal-endpoint blocks without a predecessor,
station blocks and supported inverted/covered blocks. It retains a Cancel-first
removal review and the actual native refund. The review explicitly says removal
leaves a gap and stations/access may be renumbered. Source block, loaded vehicle/
station descriptors (names, versions, DAT flags/checksum), relevant native vehicle
support/flags, all real ride track sequences, station/access references and adjacent
access paths are bound to the review and checked inside the native action.

After successful native removal, the exact original origin/direction/height becomes
a transient fixed-origin construction draft. The ride identity is reacquired only
when ID/type/vehicle/build date/lifecycle generation still agree; a native view move
is accepted here after removal, never as a stale-review bypass. Original chain,
applicable speed/seat, inverted/covered type and native colour-scheme index are kept.
Station begin/middle placement uses the native endStation construction request;
TrackAddStationElement owns actual begin/middle/end normalization and renumbering.

D-pad camera never moves this origin. L1/R1 and Select choose supported native pieces,
X/Y and picker utilities retain applicable speed/seat/chain/variant controls. A
explicitly queries/builds the replacement with its separate native cost. Incoming
and outgoing coincident native connections check pitch/bank; native TrackPlaceAction
owns clearance, station capacity, object eligibility, funds and placement. Exact
post-removal geometry/descriptor context and cost are rechecked at execute. Failed
queries keep the gap/draft/refund and expose the native error. Repeated input cannot
remove or build the same block twice. B cancels this draft to the ride's native
Continue track / Access controls without removing another predecessor.

This is deliberately two native actions, **not atomic replacement**, automatic
restoration or a dedicated replacement undo. Leaving after removal retains the real
gap/refund. Save/load stores only native track/station state; transient removed-origin
memory is cleared. A completely trackless ride resumes ordinary native station
placement after cancellation/reload; its deleted origin is not serialized. Diagonal
endpoints remain explicitly unsupported by this replacement adapter. Special/flat
footprints use their existing native adapters; positive manual variant transitions
and exhaustive family/complex geometry coverage remain unclaimed. Native partial
Execute failures on malformed maps retain native limitations; no rollback simulator.

### Phase 2 focused validation

SelectedTrackEditingTests exercises actual solitary station and disconnected straight
removal/rebuild for Multi-dimension, Dinghy covered and Flying inverted native
objects: query purity, changed quotes, exact refunds/costs, repeated submissions,
fixed origin despite cursor movement, native invalid-colour failure preserving gap
and cash, parameter persistence and native save/reload. Actual SDL virtual held-A
cancels the station review without a repeated action. Existing SeatRotationTests and
RideConstructionTests preserve conventional predecessor/rebuild, seat/speed bounds,
forward/backward recovery, ghosts and save/reload. Release GUI/CLI and CTest 5/5 pass.
End-to-end connected station/access validation and captures follow in Phase 3.

## Phase 2 publication

Canonical `82aae60d0f4b15a78f1d9ca93c4ed5f87ef40443`; local safety
`9da32d375a75de7ed02a2fe5b0074644053ff561`; identical complete tree
`0f2d8e1600995a96884994823e2161a47d36975f`. Guarded fast-forward and ref
readback succeeded before Phase 3.

## Workspace recovery during Phase 3

The workspace reverted to Staff `db96e886864a04bfbc60398493f59a221c68c55d`
during capture closeout. The protected Esperanto file and old recovery targets were
preserved outside the checkout. Canonical Phase 2 remained intact. All 110 differing
files were recovered from exact canonical blobs; every blob and the complete staged
tree matched `0f2d8e1600995a96884994823e2161a47d36975f`. Recovery safety commit
`3a7a1eafa24b1ddbc0478a4ddfe6b4f3c22b47a5` preceded compilation. No reset,
restore, clean, clone or history rewrite. Original test archives and native public
objects v1.8.0 were recovered; all final validation below was rerun on this workspace.

## Phase 3 — connected station and access recovery

Native `Ride::validateStations()` can **delete** entrances/exits facing the removed
station tile, as well as split and renumber stations. Inspection and both station
removal reviews now show the actual adjacent entrance/exit deletion counts, state
that native stations may split/renumber, and require explicit access rebuilding.
`AccessRemoval` counts real same-ride, same-height entrance/exit elements facing the
selected ordinary station tile; ghosts are excluded. It does not claim a prediction
for malformed/hacked geometry. Exact station/access/path context still guards the
review, so a changed access geometry cannot reuse that count.

After native rebuild, actual station type normalization is reacquired at the placed
block. Connected completion uses the existing production UpdateExplorer handoff to
native entrance/exit placement when missing, or RideAccess when complete. Native
placement restores access only after an explicit player action; existing queue/path
workflows reconnect it. No entrance is fabricated or silently restored. RideAccess
still distinguishes attached paths from connection to the park entrance.

Reviews additionally bind native track colour values and the current no-money mode.
A changed money mode rejects an old refund review even when the numerical quote
happens to match. Native-context failures now use a short, readable `reselect`
message in the production two-line error panel. Failed unsupported handoffs end the
transient draft instead of leaving an invalid active tool.

### Final validation, all after recovery and affected-object rebuilds

Release GUI and CLI PASS. Production source was fixed during compilation; cached
source lists were regenerated to include the recovered Mazes source, and affected
objects were rebuilt before linking drivers. CTest **5/5 PASS**: input, launcher,
atomic save publication/preservation, file browsing and named Save As.

`SelectedTrackEditingTests` PASS: native inspection/query/cancel purity; source/
station/descriptor/money-mode/quote guards; SDL virtual held-A Cancel-first review;
solitary stations and genuine no-predecessor straights; covered Dinghy and inverted
Flying rebuilds; fixed origin, repeat rejection, native refund/build costs; B cancel
without predecessor deletion; invalid-parameter failure and actual insufficient-funds
failure preserving the native gap and draft; recovery after native funds restoration;
native block/parameter/cash save/reload and transient load cleanup. Restriction probes
use native CheatSetAction for money mode/funds; no live cash/track rollback writes.

`SelectedTrackEditingCapture` PASS with actual original sprites: begin, middle and
end station removal/rebuild in four complete native circuits (Monorail, Dinghy,
Flying, Multi-dimension). Exact full block origins/types/variants/seat/speed/chain/
colour indices and accounting match after rebuild; non-default native colour index 2
and Multi-dimension seat 9 are retained. Native deleted entrances/exits are explicitly
replaced through existing native placement; queue/normal-path attachments recover.
Each circuit queries/executes native Open and survives native save/reload. Provisional
save while a gap ghost is active preserves the draft/cash and real gap. Native loaded
object version changes reject placement without cash spending; restoration recovers.
The empty Monorail vehicle moves over 4096 native ticks; no new sustained guest-use
or excitement/rating claim for these flat fixtures.

`SaveLoadBrowsingTests` PASS again: original asset-backed native browsing/import and
save/design round-trips, virtual held-A review cancellation, serialization/publication
failure preservation and save-and-exit ordering. Its temporary rerun images were not
used to regenerate the already delivered browsing ZIP.

All **14 preservation drivers PASS** against final rebuilt objects:
SceneryPathParityTests, ConstructionTests, PathActionsTests,
PresentationLifecycleTests, ParkMapInformationTests, GuestInformationTests,
GuestCompletionTests, StaffCompletionTests, FinanceResearchInformationTests,
RideConstructionTests, SeatRotationTests, FamilyOperationTests,
MazeConstructionTests and MiniGolfConstructionTests. Each argc contract was inspected;
only drivers accepting the fourth original-assets argument received it.

All **4 asset-backed regressions PASS**: DesignCapture, RideConstructionCapture,
MazeConstructionCapture and original Wooden circuit/access/natural-guest
CustomCoasterCapture. Their actual six-argument contracts use native engine data,
private original RCT2 installation, scenario, output and profile. Private files remain
outside public artifacts. After the final native money-mode header change, all 14
and all 4 drivers were relinked and rerun. The later funds-probe addition changes only
test code and passed separately; production remains fixed.

### Captures and exact remaining limits

`OpenRCT2-Pocket-Selected-Track-Editing-Captures.zip` contains **40 inspected genuine
production 512×384 PNGs**, dimension/SHA256/provenance manifest and README. Ten states
per native family show block/station inspection, Cancel-first refund and counted
access deletion, unchanged cancel, real gap/ghost/cost, native camera/covered/inverted/
seat utility choices, readable stale-descriptor recovery, connected result and access
return. All final PNGs were individually inspected for clipping, overlap, focus,
contrast, controls and panel/HUD collisions; long picker labels use their existing
full selected-detail row. Frames use actual native ViewportRender plus production
Pocket windows/panels in initialized 64×64 native fixtures, not mockups or manually
composed fake UI. Private archives, source parks and generated designs are excluded.
Exact publication/ZIP identifiers belong to the subsequent closeout section.

This closes the audited ordinary orthogonal selected-block remove/rebuild workflow.
It does **not** claim atomic replacement, automatic restoration, universal raw native
Execute failure rollback, persistent deleted-origin memory, direct in-place selected
seat changes, diagonal/special/flat-footprint parity, positive manual variant-transition
parity or exhaustive slope/bank/family coverage. Native malformed multi-sequence Execute
can fail after earlier mutation: this is source-audited, not a fabricated successful
rollback test. Physical Brick/AArch64, whole-game dense-park performance and sustained
new-family natural guest use remain unconfirmed.

Construction remains PARTIAL toward 100% ordinary gameplay. Further ordinary gaps
include native design vehicle/scenery substitution and warnings, arbitrary export
scenery selection, flat/special design/family operation, maze/entrance/exit ghosts,
scenery drag/automatic-Z/network review, shop appearance and remaining matrix items.
The next substantial milestone is **native design substitution and placement-warning
recovery**, preserving this editing, browsing and native eligibility/accounting work.
