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
