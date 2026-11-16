# Native shop appearance and preview lifecycle — completion

## Baseline integrity

Canonical baseline `615906d9ab009679c2ad9bff0d4e205caea696ee`, local safety
`ee99b273fcdfdec785b1f49b03a8f95959b29fc4`, complete identical tree
`ca76d19079118a5f41ad781a8596f8ca3091660b` verified against the current branch.
No recovery required. Only the existing protected Esperanto edit was present;
its SHA256 remains e37ba559b4d9d761e74f2bb284d1ec762b813e789655c0495a882c83651890b5.
No clone/reset/clean/restore/history rewrite; that file is excluded from commits.

## Phase 1 — paused shop preview discrepancy resolved

The original PathActionsTests driver reproduced PASS with OPENRCT2_HANDHELD unset
and FAIL with it forced to 1. Diagnostic native state at the failing paused frame:
SDL focus false before/after, zero ghost, zero real track at the preview tile,
one real ride instead of real+borrowed, unchanged cash 997000, selected shop tool
still active. InputManager::process calls UpdateExplorer only in Pocket mode;
UpdateExplorer intentionally calls Shops::ClearPreview on lost focus. This clears
the requested preview during the native ShopPreviewScope suspension, so its
restoration correctly does not resurrect a cleared request. No production leak,
cash mutation or stranded borrowed ride was demonstrated by that failure.

The fixture now explicitly distinguishes actual Pocket focus-loss frames from
focused/no-shell native tick checks. Both must preserve real track/cash. Focus
loss must remove ghost/borrowed ride, retain tool selection and never resurrect;
focused/no-shell tick must retain the valid ghost/borrowed ride. No assertion was
simply skipped or removed, and production focus cleanup is unchanged. Existing
nested provisional/save/autosave/tool/menu/load/shutdown assertions remain enabled.

Phase 1 PASS: entire asset-backed native/SDL PathActionsTests under both documented
and forced Pocket environments, Release GUI/CLI, CTest 5/5. The SDL dummy window
is actually unfocused. A separate real focused-display run was attempted using a
locally extracted Xvfb, but this workspace cannot establish its Unix or TCP display
sockets. No focused-display/physical Brick result is claimed. Native no-shell
paused preservation and explicit focus-loss branches are both exercised; focused
full-shell display validation remains a host/hardware follow-up.

The following phases deliver native appearance and the final preservation/capture closeout.

## Phase 2 — native shop/facility appearance

Desktop audit: Ride.cpp excludes station flags from shop building colour support;
RideTypeDescriptor hasTrackColourMain/Additional/Supports and the loaded entry's
disableColourTab govern building controls. Ride::hasRecolourableShopItems governs
the random merchandise checkbox. Pocket uses exactly these predicates, the
existing native colour dropdown ordering/names and RideSetAppearanceAction.
No shop vehicle/station/music controls are invented. Inspection always offers
Appearance; objects without native controls show an explicit empty state.

Controller: shop inspection fifth row Appearance; D-pad selects native channel
and paged colour/value, A opens Cancel-first review, A on Cancel/B returns to the
unchanged editor, Apply runs the native action then returns to the same channel.
The review shows original/proposed native names and the queried native cost.
Shared naming, prices/common prices, status, statistics and demolition remain.

The optional native action guard serializes the exact reviewed shop context and
checks both Query and Execute. It includes native allocation generation (process
local, never park-save data), ID/type/subtype/build date, location/real block,
object descriptor/version/flags/items, supported channels, palette cheat state,
all building colour schemes and merchandise random-colour baseline. Invalid or
unchanged proposed values are rejected. Unguarded desktop actions retain their
existing behavior. Native action execution owns appearance and accounting; no
UI writes live ride fields. Pending UI callbacks use window lifetime/request
checks; busy input and shared edge-triggered controller handling prevent repeats.

Focused ShopAppearanceTests covers five real native objects: Burger Bar, Balloon
Stall, Toilets, Cash Machine and Information Kiosk. Native capabilities differ:
Burger/Toilets/ATM expose zero controls for these loaded objects; Balloon and
Information expose building main plus random merchandise colours. It exercises
all supported channels, unsupported station/invalid colour rejection, exact zero
native cost and unchanged map/cash/prices/status/name/common-price state,
serialized guards, changed baseline/object flags, demolition/reuse of the same
native slot, Cancel-first cancellation, repeated A, actual SDL virtual held A,
stale UI recovery and native .park round-trip. The fixture temporarily changes
object flags solely to demonstrate stale loaded-definition rejection and restores
them; production never edits object definitions.

Phase 2 validation: Release GUI/CLI builds, ShopAppearanceTests native/SDL with
original assets, and CTest 5/5 pass. The final source and capture closeout below supersedes this intermediate test record.

## Phase 3 — end-to-end validation and capture closeout

ShopAppearanceTests runs with the actual engine data, private test profile and
original RCT2 installation (argc 4). Native placement -> native inspection ->
supported channel editing -> Cancel-first cancel/apply -> same-channel return ->
management -> native .park save/reload passes. Unsupported Burger/Toilet/ATM
objects remain explicit. Balloon/Information building and merchandise values
persist; no customers, income, routing or statistics are manufactured.

Final additions cover loaded object version changes, unchanged-value feedback,
nested paused provisional suspension/restoration, actual native autosave and
borrowed-ride exclusion, exact camera targeting, native input-reset cleanup, the
normal-input paused full-shell tick, menu return and native queued-action/review
cleanup across successful load. Native UI callback lifetime checks also reject
closed/dead windows. There is no new park-save state or independent accounting.

The added full-shell fixture initially expected a ghost while a modal was still
open and input was in native post-load reset. Observed diagnostics: focus true,
input reset (0), no active desktop tool, unchanged cash, zero ghost, correct
camera target. CanExplore correctly rejects non-normal input; direct onDraw and
menu calls do not complete the ordinary native window/input frame. The fixture
now closes/culls windows using native APIs, validates the actual camera hit,
asserts reset-state removal with exact real map/cash preservation, completes the
native input reset, then asserts valid paused restoration/observed focus cleanup.
This does not change or weaken the original Phase 1 paused-preview assertion.

Final frozen-production PASS:

- Release GUI and CLI builds; affected Menu/header consumers rebuilt and every
  native driver relinked after the last production change.
- CTest 5/5: controller input, launcher, native atomic save preservation, file
  browsing and named-save helper suites.
- Seventeen native/SDL drivers: SceneryPathParityTests, ConstructionTests,
  PathActionsTests, PresentationLifecycleTests, ParkMapInformationTests,
  GuestInformationTests, GuestCompletionTests, StaffCompletionTests,
  FinanceResearchInformationTests, RideConstructionTests, SeatRotationTests,
  FamilyOperationTests, MazeConstructionTests, MiniGolfConstructionTests,
  SelectedTrackEditingTests, DesignSubstitutionTests and ShopAppearanceTests.
- Six original-asset regressions: SelectedTrackEditingCapture, DesignCapture,
  RideConstructionCapture, MazeConstructionCapture, original Wooden circuit/
  access/natural-guest CustomCoasterCapture, and SaveLoadBrowsingTests.
- PathActionsTests also passes separately with OPENRCT2_HANDHELD unset and
  forced=1 after Phase 2. Final forced=1 sweep passes after the small appearance
  feedback/callback corrections. The final corrections do not alter preview
  production or the paused fixture. Both environments are reported separately.

An initial reused-output DesignCapture run rejected the pre-existing Café Native
export left by a prior successful driver: its cancellation assertion correctly
requires an absent destination. Fresh isolated outputs pass that unchanged
assertion. No export was removed and no assertion weakened. Driver argc/env
contracts were checked; no missing test or physical result is claimed.

## Captures

OpenRCT2-Pocket-Shop-Appearance-Captures.zip contains 28 individually inspected
production 512x384 PNGs plus dimension/SHA256/provenance manifest, checksums and
README. All PNGs use actual native ViewportRender/X8DrawingEngine and Pocket menu
onDraw or shop overlay/footer/HUD. They are production component captures over
native data, not physical-device or whole SDL framebuffer screenshots.

ZIP SHA256: dc51c8d2a67244ea34fa9851262d6b46c00265ef3fc518cebaa09027c3f61e11

The native fixture uses original objects and native actions; paused-preview
captures explicitly use the native build-in-pause cheat in that isolated fixture.
Every final image was inspected at native pixel size for clipping, overlap,
focus, indigo/gold contrast, readable controls, park visibility and HUD collision.
The stale recovery message was shortened to remain fully readable. The capture
camera leaves the actual shop visible alongside the panel. No fake UI composition
or metadata is used. Private assets, source parks, profiles and generated designs
are excluded. Older completed ZIP packages were not regenerated.

Repository provenance: captures/shop-appearance-manifest.json records exact
production/driver/binary hashes, native asset hash, invocation and limitations.

## Publication history and remaining limits

Phase 1 canonical 4372a3da1b2db8a84824009ac8044e39e4e03030, tree
2574ff175c17eafd0bca08cfad3868cd4503dcae, matching local safety
af4031bfff56f98a82be3a57724233d65a7fbe16.

Phase 2 canonical 5e25be1470132c91b3bab728d2c2afddaaf5793c, tree
b547c214028b10cfc1735475d8f7aaf7ca1978e0, matching local safety
4634cf05fc0bc6de95820bc3accb46ab547d6bc6. Every changed blob, complete tree,
expected canonical parent and force=false lease update/readback were verified.
Phase 3 publication is recorded in the final closeout below and handoff.

Shop channels are exactly the native descriptor/object rules; no arbitrary
per-product colour, vehicle/station/music control or object installation is added.
Special palette entries follow the existing native cheat policy. Shared keyboard
retains UTF-8 but types its existing labelled ASCII/32-code-point scope. Native
network/replay and physical Brick/AArch64/dense-park performance remain unverified.
The logical SDL paused frame is covered; a real focused display/hardware run is
still separate evidence, and the attempted Xvfb display was unavailable here.

Unrelated ordinary gaps remain: direct selected-seat/further parameters;
diagonal/special/exhaustive selected replacement and positive variant transitions;
flat/special saved-design adapters and unsupported substitutions; maze/access
placement ghosts; continuous scenery gestures/automatic-height/network reviews;
native file-location/format/keyboard and late-import limits; sustained individual
family guest operation. Selected replacement remains explicit native removal and
rebuilding, without atomic replacement/restoration; native design placement stays
sequential with possible optional scenery omission. These are not claimed closed.

Next substantial scope: native maze and ride-access placement previews and their
provisional lifecycle, preserving all construction/action/eligibility/accounting.

### Final publication closeout

Phase 3 canonical e7abc11c4aaa82f2085a14102a560f6df7cf498f, complete tree
16e03999cba4bc6a0aedfa1db56a36a6990ebfd4, matching local safety
090a60939897385021b372c6f8134301bc271d92. Its native parent is Phase 2
5e25be1470132c91b3bab728d2c2afddaaf5793c. Every changed blob and complete tree
were verified before expected_sha/force=false fast-forward publication, then the
native commit parent/tree and resulting branch ref were read back. The final
publication-history documentation commit changes no production source, test,
PNG or ZIP. All three authorized phases are complete within the stated native
channels and host validation limits. Protected Esperanto remains unchanged and
outside every milestone commit. The ZIP is saved as an actual downloadable file.

## Native maze/access preview follow-up

Missing entrance/exit placement now renders isolated native ghosts without real
station/path/maze/cash mutation. Maze build/fill proposals reuse native geometry on
native ghost entries, with nested provisional/save and paused-tick cleanup. See
MAZE-ACCESS-PREVIEW-COMPLETION.md for completed native/SDL/asset validation,
29-capture package, guards, recovery and precise remaining limits. Existing access
relocation/removal and exhaustive family cases are not delivered by that follow-up.
