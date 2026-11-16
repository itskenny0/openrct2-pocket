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

## Phase 2 publication

Canonical `7a32f72589ce64c661c077a34ad7b3713ec3d61e`; local safety
`8a9adb126ad52179fbd1b5dbe7455b00210f1350`; identical complete tree
`9f8a2238afd43b0896f69186802630b4fb113570`. Native parent, all changed blobs,
complete tree and guarded branch/ref readback verified before Phase 3.

## Phase 3 — native omissions, readable recovery and end-to-end evidence

Vehicle choices now show native object names with exact descriptor detail. The
placement rail says All scenery omitted without clipping. Failed confirmation
retains its real error in X → Availability / warnings, with bounded full UTF-8
selected-row detail; the rail announces Review rejected. New successful reviews
clear that transient error. Existing menu-owned pause/input/ghost rules remain.

Native TrackDesignPlaceSceneryElement may swallow optional scenery subaction
failures (notably small scenery and paths). An ephemeral native diagnostic counter
now records the actual failed subactions in the last native query/placement. It
resets inside TrackDesignPlaceVirtual, does not alter native outcomes/costs and is
never park state. Query omissions appear in the Cancel-first review; its expected
count is serialized with the native action and rechecked with the quote. Successful
placement reports the actual native omission count before the existing access
handoff. Missing-object all-scenery fallback and failed-subaction omissions are
separate: when all scenery is disabled by fallback, no scenery subaction is attempted.
No omission count is estimated from a UI layout or invented gameplay statistic.

DesignSubstitutionTests now builds and exports a real native closed Monorail
circuit and uses original assets for real rendering. An intentionally absent
legacy source vehicle demonstrates explicit Streamlined Monorail → Small Monorail
choice; the native parser/exporter owns the test TD6. The source stays unchanged.
The controller sequence covers dense repository-directory browsing, empty folder,
folder entry, required-object inspection, native alternative list/cancel, real
preview, selected-file browser return, manual height/orientation retention, scenery
choices, warnings, Cancel-first review, SDL virtual held-A cancellation, stale
version/source rejection, serialized native guards, funds failure/recovery and once-
charged placement. Native completion then hands off to actual entrances/exits,
queue/normal landings, native Open and .park save/reload. The substituted ride's
actual native subtype, cost, Open status and source identity are asserted.

A second native exported design requests two loaded/researched small-scenery
elements; one legal roof places, and one off-map request is skipped by the actual
native subaction. Native query/execute report exactly one omission, the real map
contains exactly one roof, and the once-charged native quote is exact. A changed
expected omission count rejects. No live map/cash rollback or manual object insertion
is used. Temporary original/research/ownership setup belongs to isolated test
fixtures; gameplay never installs or unlocks objects.

Automatic provisional saving while the selected alternative's ghost is active
preserves the real map/cash and selected draft, restoring the native ghost afterward.
Loading that autosave demonstrates no saved provisional ride/track or retained
review/error/draft; loading the final native park restores the actual substituted
Open ride. Existing browsing/load-failure/title/shutdown/pending cleanup is preserved
by the separately rerun regression drivers. Twenty-four final 512×384 native X8
viewport/Pocket component captures are individually inspected. These are production
component renders of actual native fixture state, not a physical Brick or full
SDL framebuffer recording. Original assets/generated parks/designs remain private.

## Final validation and exact environment

Production source remained fixed through the final GUI/CLI compilation and linked
drivers; header changes rebuilt affected native/UI objects. The final focused
fixture was relinked after compilation and after capture/test-only corrections.

- Release GUI and CLI: PASS.
- CTest 5/5: controller input, launcher, save-publication/preservation, file browsing,
  named save. Shared file identity extraction rebuilt dependent standalone tests.
- DesignSubstitutionTests: PASS, initialized native/SDL dummy plus actual virtual
  controller, original assets, all three phases including autosave and final reload.
- Preservation PASS: SceneryPathParityTests, ConstructionTests,
  PresentationLifecycleTests, ParkMapInformationTests, GuestInformationTests,
  GuestCompletionTests, StaffCompletionTests, FinanceResearchInformationTests,
  RideConstructionTests, SeatRotationTests, FamilyOperationTests,
  MazeConstructionTests, MiniGolfConstructionTests, SelectedTrackEditingTests.
- PathActionsTests PASS with SDL dummy and OPENRCT2_HANDHELD unset, matching
  tools/handheld/run_native_test.py's documented invocation. An extra invocation
  with the Pocket frame-mode environment forced failed its paused-shop preview
  fixture ("Paused native tick stranded shop record/changed cash"). This is
  retained as a separate failing result; no product fix or interactive diagnosis
  of that extra-mode fixture is claimed. It does not become a passing test by
  changing an assertion. Other named drivers use their own explicit mode setup.
- Asset-backed PASS: SelectedTrackEditingCapture (four families/station positions,
  no regeneration of its delivered package), DesignCapture, RideConstructionCapture
  (Factory Capers), MazeConstructionCapture (Factory Capers), and original Wooden
  CustomCoasterCapture (African Diamond Mine), including the existing native
  circuit/access/natural-guest 30,000-tick regression. Guest queue use passes;
  sustained throughput for substituted Monorail is not claimed.
- SaveLoadBrowsingTests: PASS, original-asset native/SDL/save/design/input failure
  workflows. Its delivered capture ZIP was not regenerated.
- Each actual argc contract was inspected; optional original-assets arguments
  were used only where accepted. Python capture-tool compilation and tracked
  diff whitespace checks pass, excluding the protected unrelated Esperanto file.

Reproduce captures and the focused fixture with:

```sh
python3 tools/handheld/render_design_substitution.py --build BUILD --data DATA \
  --assets PRIVATE_RCT2 --output CAPTURES --ninja NINJA
```

The helper keeps generated source TD6s, parks and profile outside the PNG output.
It links the actual completed GUI objects and removes only the desktop main and
linker dependency-output option. Original binaries/assets are never bundled.

## Exact remaining ordinary gaps

This closes the scoped explicit vehicle/warning recovery workflow, not 100% gameplay
or exhaustive desktop design parity. Eligible alternatives require loaded/researched
exact native ride-type and every actual track-group/variant support. Native operation,
train sizing, settings rejection/clamping, clearance, supports and access still decide
whether a particular design/vehicle/site can operate. Monorail supplies the new
end-to-end substitution proof; every family/variant combination is not established.
No arbitrary cross-family or individual scenery replacement, install/unlock, flat/
special footprint saved-design adapter, parser/format expansion, automatic station/
park-network routing or geometry repair is added. Existing native TD4/TD6 filters
and parsing/location limits from SAVE-LOAD-BROWSING-COMPLETION.md remain unchanged.
Missing-original files can be reached through folders even when absent from the
eligible native repository picker. Full availability detail uses the browser; the
small repository cards remain the existing preview selector.

The native placement API is sequential. TrackDesignAction demolishes its new ride
on a reported layout failure, but does not supply universal scenery/map rollback.
Ignored optional failures can produce a successful partial result; the diagnostic
counts actual failed native scenery actions, not a promise that every requested
object is present. Legacy path mapping/default surface/railing changes remain
native. Runtime native action serialization now carries local source-file identity;
multiplayer/replay portability of that filesystem guard is unvalidated. No new
Pocket gameplay park-save state exists. Native late park-import rollback limits,
32-code-point ASCII keyboard typing, direct selected-seat editing, diagonal/special/
exhaustive replacement, positive transitions, maze/access placement ghosts,
continuous scenery gestures/network review, sustained family guest operation and
physical Brick/AArch64/dense-park performance remain as recorded in the matrix.

Next substantial ordinary gap: broader native shop/facility appearance, using the
existing native naming/management and appearance actions. Naming already exists;
do not rebuild it or infer a colour/music capability unsupported by its native object.

## Phase 3 publication and final capture package

Validated implementation canonical `2f364a1213996a5fde66349423e7a0a7f7fbcd38`;
matching local safety `c167a945291ef9f86d96e2f5bb7398f604c37b5b`; complete tree
`832d344cf04c4d2c96f54de7ab1902cecb67faab`. Every changed blob and the full tree
matched before a guarded expected_sha/force=false fast-forward from Phase 2.
Canonical native parent/tree and the resulting branch ref were read back and verified.

Saved deliverable: **OpenRCT2-Pocket-Design-Substitution-Captures.zip**.
24 individually inspected production 512×384 PNGs, manifest.json, manifest.csv and
README.txt; 27 archive members, 185575 bytes. Every archived PNG hash and archive
integrity/dimension count verified. ZIP SHA256:
`bcf4bb8818213e040ad4bf0b6b152d4bc9a932dfc6726cdb50abd1de834e4d2b`.
Manifest records implementation canonical/local/tree, exact driver blob, real native
fixture/asset provenance and all validation limits. Only PNGs and metadata are in
the ZIP. Earlier scenery, browsing and selected-track deliverables were preserved.
This docs-only closeout follows the fixed tested implementation; its final ref is
reported in the handoff rather than embedded as a circular self-reference here.
