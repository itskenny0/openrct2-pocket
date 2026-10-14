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
