# Native design availability and substitution — incremental checkpoint

## Recovery and baseline

Canonical baseline `2fa2c56152ee5a6ae23563739ccff7acba25d0c2`, complete tree
`1f1a0e9c0ca94a8c21028246a105b5a4f02565fc`. Workspace had reverted to Staff.
All 111 differing blobs were recovered and individually verified, complete staged
tree equality established, and local recovery safety commit `fe1a4d2` made before
compilation. The existing Esperanto modification was preserved outside the
checkout and remains untouched/excluded. No clone/reset/clean/restore/history rewrite.

## Native audit

Desktop TrackList.cpp imports a design and renders native availability flags;
vehicleUnavailable warns before TrackDesignPlace. TrackDesignAction delegates
missing-vehicle fallback to RideCreateAction / RideGetEntryIndex, using native
object-manager ride-type entries. This is not an arbitrary cross-family vehicle
compatibility contract. Native RideEntryGetSupportedTrackPieces and the regular/
covered/inverted track drawer descriptors establish the track groups supported by
a candidate object; actual native placement/operation still owns its rules.

TrackDesignPlaceSceneryElementGetEntry resolves native objects/research and legacy
path mappings/default loaded surfaces and railings. Missing/unresearched optional
scenery sets the native sceneryUnavailable flag; TrackDesignAction requeries and
places **without all design scenery**, rather than guaranteeing a partial list.
Path mappings/defaults may differ from a legacy requested object. There is no
arbitrary named scenery replacement API. Native placement is sequential and
TrackDesignAction demolishes its newly created ride after a reported failure;
that does not establish rollback of separately placed scenery or other map changes.

## Phase 1 — availability inspection

Build → saved designs → X Browse files → select supported native file opens
inspection. The existing Place/Back rows are followed by paged native required
vehicle identity/loaded/research status, ride research state, native compatible
loaded/researched vehicle count, station-style/default fallback, and each optional
scenery object's native resolution. Inspection does not load/unlock objects,
construct rides, spend cash or change the source design. Native parser errors and
unsupported family/geometry remain distinct from missing/unresearched objects.

The read-only TrackDesignResolveScenery function is extracted from native placement;
placement retains its flag-setting wrapper. UI availability reuses this same
resolver, never a second scenery mapping algorithm. Vehicle compatibility uses
exact native ride_type membership plus every actual track group's native vehicle
sprite support and normal/covered/inverted drawer support. Flat/special designs
remain outside the existing Pocket adapter. Alternatives are inspection-only in
this phase; original-vehicle placement behavior is preserved pending Phase 2.

Validation and guarded publication are recorded after the phase checks complete.

Phase 1 verification: Release GUI and CLI full rebuild after recovered headers;
focused initialized native/SDL DesignSubstitutionTests with original assets verifies
missing required object versus loaded/unresearched, native ride-type incompatibility,
alternative enumeration independent of absent original, optional scenery resolution,
unchanged native placement flag, cash/free ride slot/research/RNG. CTest 5/5 passes.
Long availability rows support Left/Right bounded UTF-8 detail; D-pad rows page and
B returns to the selected file. Subsequent substitution/execute and final asset-backed
workflows remain pending. Physical Brick/AArch64 are unconfirmed.

## Phase 1 publication

Canonical `e428f2cf9f58ce754ae624137cc5190ba8094b64`; local safety
`7e3fa20b1c5aa97f703ba130d7bab0b273d0afac`; identical complete tree
`b07b672d8c0c47d64d7b5c11be6cf690a27d5627`. Every uploaded blob, native
parent/tree, guarded expected_sha/force=false update and resulting ref verified.

## Phase 2 — explicit vehicle and scenery policy

File inspection → Choose native vehicle lists already loaded/researched native
ride-type entries whose actual vehicle sprite/normal/covered/inverted track groups
support every design piece. The selected descriptor is explicit; no automatic object
installation, unlock or source-file rewrite. No native group/geometry support is
inferred from vehicle names. Empty lists and stale object/file selections remain
recoverable. Original supported vehicles remain eligible. Missing original vehicles
are still unavailable until the player explicitly chooses an eligible alternative.

Placement X options retains existing height/scenery/camera/Back indices and adds
Choose vehicle and Availability / warnings. Choices preserve orientation, manual
height and scenery policy, clearing only provisional preview. Native TrackDesign
copies carry the chosen descriptor through preview, query and SavedTrackDesignPlaceAction
into TrackDesignAction / RideSetVehicleAction. Native vehicle/settings/station rules
remain authoritative; a compatible geometry list is not an operating guarantee.

Cancel-first full-screen review shows selected vehicle, actual cost, origin/direction,
height, scenery status, native settings-clamping warning and no universal scenery
rollback guarantee. Existing L2/R2 scenery Off/On remains. If native resolution finds
unavailable scenery, review explicitly says **ALL design scenery omitted**. Native
legacy path mapping/defaults remain, with their resolved descriptor visible in
availability. There is no arbitrary individual scenery replacement UI or API.

Reviews bind immutable copied design choices/location, original full-byte file
identity, native quote/money mode and exact transient loaded-object descriptors,
versions, research/vehicle support/station-default context. The native action repeats
context/file/cost checks in Query and Execute, including after action serialization.
Shared core FileIdentity is the unchanged browser identity algorithm extracted for
reuse; it does not add park-save fields. Stale reviews reject before native creation.
A blocked/failed query leaves placement available; B/Cancel returns unchanged. Held
or repeated confirmation cannot submit twice. A reported native Execute failure uses
the existing TrackDesignAction cleanup; independent scenery rollback is not promised.

Phase 2 PASS: Release GUI/CLI rebuild; initialized native/SDL DesignSubstitutionTests
builds/exports an actual closed Monorail circuit, preserves an absent-original source,
explicitly chooses the other native Monorail vehicle, checks all-scenery omission,
query/ghost/cancel map/cash purity, unchanged height/rotation on vehicle change,
changed descriptor and source rejection, bound-action serialization and direct stale
Query/Execute rejection, actual virtual held-A cancellation, native insufficient-funds
failure/recovery, exact final subtype/cost, duplicate rejection and .park roundtrip
with transient cleanup. CTest 5/5 rerun after shared file-identity extraction.
End-to-end browser/access/Open/capture and broad preservation evidence follows.
