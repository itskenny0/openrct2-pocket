# Scenery selection and landscaping completion — incremental checkpoint

Goal: 100% ordinary OpenRCT2 gameplay through native controller Pocket at 512×384.
Base canonical `ae9d40723073248ff0c5c41c138e134b52d97ee0`, source tree
`166eb36e43f41106b85548005223f768f51f911c`. The workspace again reverted to Staff
`db96e886864a04bfbc60398493f59a221c68c55d`. Every existing edit/index/untracked
inventory was preserved before recovery. Ninety-one changed published files were
recovered atomically after individual blob verification. Full indexed tree equality
and an otherwise unchanged tracked working tree were verified before compilation.
Recovery safety `57320379e91750ad6b92fe265e1e7b58c8ca0533` exactly matches the
published tree. Esperanto remains unchanged with SHA256
`e37ba559b4d9d761e74f2bb284d1ec762b813e789655c0495a882c83651890b5`, excluded.
No clone/reset/clean/restore/history rewrite or force-push was used.

## Phase 1 — native scenery eyedropper

Scenery options appends Pick placed object, preserving existing indices. A closes
options into pick mode; D-pad pans, X rotates, L1/R1 zoom and Y/Select recovers
options. A over a real eligible native sprite selects its object/settings and
returns to placement; that press cannot also place. B ends the tool. The existing
Mode row also cycles through Pick. Target feedback shows actual hovered object,
or Point at scenery, rather than the previous placement object. HUD/indigo rail/
gold focus and contextual Pick/Place controls remain native production components.

Native desktop scenery eyedropper interaction flags supply small/large/wall/banner/
path-addition sprite hits. Existing Capture/Find copies category, native position,
height, direction/part, object descriptor, colours and banner metadata. Pick
re-resolves and compares the full copied identity, rejects ghosts/ambiguous/stale
identities and requires actual loaded/researched availability. No pointer survives
selection. Native map directions are retained (desktop button rotation is camera
relative). Small quadrants, wall/banner edges, native height and supported colour
channels are copied; unsupported channels retain current settings. Native large
sequence metadata normalizes child height to the placement origin; banner height
undoes the native banner/path offset. Furniture uses real path height/identity.
Group context is retained when it contains the object; otherwise it resets to
All available. Sign text/no-entry/routing are not cloned into a new object.

Selection is a read-only tool handoff, not a placement action. It neither charges
money nor alters the selected element. Placement, recolour/removal, ghosts and
serialization remain owned by existing native GameActions and provisional cleanup.
No additional Pocket gameplay state is serialized. Existing ordinary ambiguity
limits use native copied identities; no new persistent scenery IDs are invented.

Release GUI/CLI build PASS after refreshing the older cached source list to include
the already-published Mazes module. Production source was held fixed while building.
ConstructionTests PASS: all five categories, query-only tool stability, supported/
unsupported colours, unchanged native element/cash/ledger/ghosts, descriptor/research/
removed-target rejection, non-origin large sequence and elevated/rotated targets.
Expanded asset-backed native/SDL SceneryPathParityTests PASS: actual banner sprite
hit at the controller reticle, held/duplicate-A pick without placement, existing
native groups/banner styles/area clearing/cancellation and persistence. Existing
PresentationLifecycleTests PASS; CTest input/launcher/save-preservation 3/3 PASS.
The fixture camera uses native projection and original sprites; production does
not move peeps or invent hit geometry. Fixture compile corrections applied native
zoom separately to x/y and supplied the existing capture helper's explicit mode.

Three actual 512×384 native viewport/component captures were generated and inspected:
landscaping-01-native-eyedropper, landscaping-02-native-eyedropper-placement and
landscaping-03-eyedropper-options. They use original sprites over a native action-built
initialized fixture, actual tool/native HUD state and production drawing components.
They are not whole SDL/hardware frames or sustained gameplay evidence. Incorrect old
object feedback was fixed, rebuilt/retested and regenerated. Bounded ellipsis remains
intentional; no material overlap/focus/footer/HUD defect was found. Final dimension/
SHA256/provenance manifest and ZIP follow the remaining phases. Private archives,
parks and designs are excluded. No AArch64 or physical Brick result is claimed.

## Remaining requested phases

Native object search, shared native scatter/bulk placement and native surface/edge
colour controls remain pending. Delivered styles/groups/clearing/furniture/access/
undo, finance/research and guest history/follow/filter/map workflows are preserved.
All broader ordinary construction/design/save/map gaps stay on the feature matrix.
Canonical phase SHA/tree will be recorded after guarded expected_sha/force=false
publication with exact local/canonical source-tree equality.


## Phase 1 publication

Canonical `143a98cc10ce7db7194224dee92c3877925034d1`, local `e258bbf32994eb2b77e391e4325a6eead75d6e00`,
matching tree `0e3868982f6bdd06af307a310400dd6cdb1dbe33`. Native parent ae9d407,
changed blob hashes and exact source-tree equality were verified before the guarded
expected_sha/force=false fast-forward. The following phase preserves that source.

## Phase 2 — native eligible object search

Scenery options appends Search objects after existing actions. X from any scenery/
furniture picker opens the existing controller keyboard directly. D-pad/A types;
OK applies to the same category/group picker, B/Back discards the draft and recovers
its parent/focus, Clr/OK removes the query. Sp/Aa labels keep space/case readable.
Start remains the existing menu/picker recovery control. The bounded keyboard retains
UTF-8 and accepts up to 32 code points; new keys use its existing ASCII alphabet.

Search uses desktop Scenery's native case-insensitive String::contains over actual
object names, identifiers, authors and filenames. It intersects loaded/researched
eligibility and existing native group membership; no object loading or unlocking
occurs. Applying a query changes only transient view state, leaving placement
selection unchanged until explicit A on a result. Empty results offer X Search/B Back.
Category changes clear query/group context; eyedropper keeps matching context or
explicitly resets filters when picking an object outside them. No gameplay/save state
is added. Ghost suspension/cleanup and all prior placement/styles/clearing remain.

Scenery picker identities now carry the native descriptor key. Live refresh retains
that object across filtering/index changes; confirmation checks the actual current
eligible category/group/query and descriptor, rejecting reused/unavailable slots.
Native Object::DrawPreview supplies images. Other picker behavior is unchanged;
only scenery/furniture uses its optional X Search footer instead of the Start label.

GUI/CLI build PASS. Expanded asset-backed native/SDL SceneryPathParityTests PASS for
mixed-case native names, exact descriptor identity, stale descriptor rejection,
bounded input, group intersection/research exclusion, apply/cancel/clear, unchanged
placement choice/cash/ledger, empty results, picker focus and selection handoff.
Existing ConstructionTests and PresentationLifecycleTests PASS. CTest 3/3 remains
passed; final sweep follows later phases. A string_view descriptor was explicitly
copied into its stable key and the fixture typing loop scope was corrected without
weakening assertions or compiler warnings. Production source was held fixed during
compilation. The shortened keyboard hint/labels were rebuilt and captures regenerated.

Four genuine production 512×384 native fixture captures (04 keyboard, 05 results,
06 empty, 07 cleared) were inspected. Controls, focus, previews, park area and native
HUD-yield contract are readable without material overlap/clipping. Long metadata
uses existing bounded ellipsis; the keyboard retains the complete query. Final ZIP/
manifest follows remaining phases. Captures are native viewport/component fixtures,
not physical Brick or whole SDL/hardware frames. Private source assets are excluded.

Native shared scatter/bulk placement and terrain surface/edge colours remain pending.
All other ordinary construction, export, path-network, map/save and hardware gaps
remain on the 100% feature matrix. Phase 2 canonical SHA/tree is recorded after guarded
publication, without modifying the tested production source.

## Phase 2 publication and second recovery

Phase 2 canonical `f20751d08f26d9e25cac52f474df668a45341665`, original local
`836b9d61fb30d93ce7b18adba9fd9f59132bb9ce`, exact tree
`6a38c38c42aa80e60f27e7107209e4f4e80b33df`. The checkout/build directories
vanished during the subsequent scatter compile; the execution service disconnected.
The incomplete scatter changes and captures were lost. On 2026-10-13 access returned
at Staff db96e886 again. Every edit/index/untracked file was preserved; 93 canonical
files were recovered atomically after blob checks, complete tree equality verified,
and recovery safety `81092f6115d47b1f516dc67b3f3e08fa27bedca6` committed before
compiling. Esperanto retains its recorded SHA256 and remains excluded. No forbidden
Git recovery/history command was used. Private Library assets were rematerialized.

## Phase 3 — shared native small-scenery scatter

Desktop Scenery scatter and drag were audited. New SceneryScatter.h extracts the
existing desktop UtilRand quadrant/offset/even-size/non-rotatable-direction proposal,
quantity and selection bounds helpers; desktop and Pocket both call them. This is
not an independent scatter algorithm. Native small scenery only: options appends
Scatter enabled, size 1–64 and density 1–3. Native attempt count is size times density,
not area squared. Existing indices/categories/groups/search/styles are preserved.
D-pad pans; X camera rotation, L1/R1 zoom and Y/Select options retain the tool.
Native square selection and one native object ghost follow desktop preview semantics.
A first reviews the frozen native proposals without placing/spending; a second
released A applies. B cancels the tool/review. Changing tile/settings/object resets
review; native descriptor, availability, ownership and cost are rechecked. The cost
badge explicitly says estimate: independently valid proposals can collide later.
No atomicity is claimed. Native SmallSceneryPlaceAction owns every placement, height,
clearance, ownership, research, colours, accounting and persistence.

Native action callbacks track actual completed/successful/failed attempts and actual
cost. A pending batch rejects repeat submission; controller suspension prevents held
A from skipping review or duplicating apply. Partial result shows placed/attempted and
actual cost; Options exposes the native failure message. Transient draft cleanup
shares tool/ghost cleanup; native map-load/reset and shutdown also clear scatter
feedback/settings. Callbacks own detached result lifetime safely. No serialized
Pocket gameplay state was added. Native queued actions are not promised cancellable.
Network scatter is explicitly unavailable until native asynchronous network completion
can be represented safely; ordinary single-object placement remains unchanged.

GUI/CLI build PASS with source held fixed. Expanded ConstructionTests PASS: stable
query estimates and no map/cash/ledger mutation, read-only review/cancel, stale
ownership, native partial clearance, pending repeats, exact actual cash, multi-tile
native population/accounting, research/bounds, native save/reload and cleanup.
Asset-backed native/SDL SceneryPathParityTests PASS including real reticle controller
review/apply with duplicated A, actual partial result, native error recovery/options,
all previous groups/search/styles/area-clearing checks and persistence.
PresentationLifecycleTests PASS. An overlapping new fixture was isolated from the
existing area-clear fixture; no behavioral assertions were weakened. Native target
comes from actual reticle/map selection rather than assumed world coordinates.

Six genuine production 512×384 captures (08–13 options, native ghost preview, review,
partial result/cost, native error options and larger area preview) were inspected.
Overlong result detail was moved to Options, rebuilt/retested and regenerated.
Native viewport and production Pocket components over action-built native fixtures,
original sprites; not hardware frames or sustained-operation proof. Final manifest/
ZIP and broad sweep follow Phase 4. Private source archives/parks/designs excluded.

Remaining exact ordinary gaps: continuous native wall drag preview/release placement,
continuous repaint/eyedropper drag equivalents, desktop Shift automatic Z-search
modifier behavior, and network scatter callback/pending recovery. Native surface/edge
colour controls remain next. Broader matrix gaps remain; 100% ordinary gameplay is
still the goal, not a delivered claim. Canonical phase publication is recorded in the
following checkpoint after expected_sha/force=false and exact full-tree verification.

Phase 3 final correction: menu provisional cleanup retains completed native batch
feedback/failure until the next review or map-load/exit reset. SDL assertions verify
that Options does not erase the actual failure. The native error capture was
regenerated and inspected. ConstructionTests rerun PASS; CTest 3/3 PASS. Temporary
partial recovery files were moved into the preserved recovery archive, not discarded.

## Phase 3 publication

Canonical `ee2cbd3db0b334915455676542f429bdd7588a11`, local final safety
`a78766abe5aac24e5d050f305cf8c97af55578c5` (preceded by bedaf26), complete matching
source tree `ef54f2c2e2b222697731aeee3d6eaa05a962da9a`. Current native parent f20751d,
every changed blob and complete source-tree equality were verified before the
expected_sha/force=false fast-forward. Esperanto remains excluded and unchanged.

## Phase 4 — native terrain surface and edge colours

Area paint options appends Surface colour and Edge colour. D-pad left/right or A
cycles the actual desktop 72-colour dropdown mapping; native localized names are
used. Only loaded objects with TerrainSurfaceFlag/TerrainEdgeFlag::hasPrimaryColour
expose a channel; unsupported choices are explicitly unavailable and cannot change
settings. Changing styles uses the desktop LandTool reset helpers for native defaults.
No object is loaded/unlocked by the workflow. Quick surface Y opens the shared
one-tile paint/colour options with its selected native surface; all loaded surface/
edge choices remain available. Camera X and L1/R1 zoom remain native.

Native surface/edge icons with the same ImageId remaps as desktop Land provide
selected-colour previews alongside the native area selection. A in area paint opens
a Cancel-first review showing selected styles, supported colours and the native quote.
A Apply submits one native SurfaceSetStyleAction; B/Cancel returns to the active
brush without changing map/accounting. Review snapshots the selected native object
descriptors, actual terrain styles/colours/base height/slope/water/ownership and park
flags, and rechecks the actual quote. Changed/reused/missing objects, changed terrain/
flags/cost, invalid palette encodings and native restrictions/funds reject apply.
Pending and submitted guards prevent held/repeated actions; native callback completion
releases the pending guard. Map-load/exit cleanup detaches pending review lifetime.
No new serialized gameplay state or direct Pocket terrain writes were added.

A demonstrated native action inconsistency was corrected: edge style changes wrote
primary edge colour even when the new edge lacked the primary-colour flag, although
Query already ignored recolouring for that unsupported channel. Execute now uses the
same native flag gate as surface colour writes. Native style changes preserve both
unsupported encoded colour channels without altering quote/accounting policy. Tests
switch actual colourable Grid/Stucco to fixed Grass/Rock and verify this preservation.
Native actions still own range/ownership, restrictions, costs, grass and save encoding;
no all-tiles/atomicity promise is added beyond their existing semantics.

GUI/CLI build PASS with production source fixed. Expanded native ConstructionTests
PASS: actual supported palette/flags, both colour encodings and native save/load,
query/review map/cash/ledger purity, stale descriptor/ownership/quote, native landscaping
restrictions/funds, invalid palette, pending/repeated actions, exact native cost and
unsupported channel preservation. Asset-backed SDL SceneryPathParityTests PASS for
both controller colour selectors, real park-A review with held input, Cancel-first
cancellation, native apply/cost/both fields, pending guard, stale recovery, unsupported
controls and quick-surface handoff. PresentationLifecycleTests PASS. CTest final sweep
follows. Native multiplayer paint review remains unavailable until asynchronous
network completion/recovery can be validated; the ordinary single native action API
is unchanged.

Six genuine 512×384 captures (14–19 colour options, selected native icon/area preview,
Cancel-first quote, native result, stale review and unsupported controls) were generated
and inspected. Preview sprites were moved fully inside their indigo panel, rebuilt,
retested and regenerated. Product labels say Style previews, Cost and Apply paint.
Native world/components and real native data; not hardware/full SDL frames. Broad
preservation sweep, asset-backed regressions and final inspected ZIP follow this
independently published phase. Remaining exact drag/network/Shift-Z and broader
ordinary roadmap gaps from Phase 3 persist; 100% ordinary gameplay is not yet claimed.

## Phase 4 publication and interrupted capture closeout — 2026-10-13

Phase 4 canonical `3882849c5648fdc78c54ffc3279e07400117fe8a`, local safety
`0cd7bb74af184bd1d27a54b0dec32b55df029e2b`, complete matching tree
`92c2accbabe7381f38f99bdb5c2a6549fd2c9b1c`. Native parent ee2cbd3, every changed
blob and full source-tree equality were verified before expected_sha/force=false
fast-forward publication. All four requested implementation phases are published.

Final fixed-production-source sweep: PASS for all fourteen drivers:
SceneryPathParityTests, ConstructionTests, PathActionsTests,
PresentationLifecycleTests, ParkMapInformationTests, GuestInformationTests,
GuestCompletionTests, StaffCompletionTests, RideConstructionTests, SeatRotationTests,
FamilyOperationTests, MazeConstructionTests, MiniGolfConstructionTests and
FinanceResearchInformationTests. Native/SDL assets were supplied only to drivers
whose actual argc contract supports them. Separate temporary profiles were used.
Independent test links reused the completed GUI objects/definitions/includes and
removed only the GUI link dependency-file output flag, preserving its dependency
records. No production source changed during compilation/testing. CTest 3/3 PASS.

Asset-backed Africa - African Diamond Mine.SC6 preservation drivers returned confirmed
exit 0: RideConstructionCapture, MazeConstructionCapture and CustomCoasterCapture
(the original Wooden circuit/access/natural-guest regression). These are the actual
existing assertions, not proof of every ride family's sustained guest operation.
DesignCapture remains UNCONFIRMED: its result recorder failed when the checkout
disappeared before it could retain/report the return code and source identity.
Do not count that driver as passed without rerunning it.

The checkout disappeared again during this asset sweep; subsequent access reported
409 environment_offline: Environment is not connected. Retrying execution did not
restore access. All implementation code and its validation checkpoints were already
published safely. A further local documentation safety commit was impossible.
This documentation-only handoff uses the current native parent, verifies each changed
blob hash, independently computes the complete Git tree hash from every authoritative
tree entry, and guards the fast-forward with expected_sha/force=false. Production
source remains exactly the validated Phase 4 source. No clone/reset/clean/restore,
discard/history rewrite or force-push was used; Esperanto was last reverified at its
recorded SHA256 and is absent from every milestone commit.

Nineteen actual production native viewport/component PNGs were regenerated. Phase
captures 08–19 were inspected, material result overflow and preview containment
defects fixed/rebuilt/retested, and corrected images regenerated. Original 01–07
images were inspected in their earlier validated phases but lost in the prior wipe;
their newest regenerated bytes still need final inspection. The final per-file
dimension/SHA256/provenance manifest, ZIP and durable download could not be completed
or made available after the environment went offline. Do not claim that package
exists or that uninspected final bytes were approved. Private archives, parks and
designs have not been published.

Resume from the latest canonical branch/checkpoint, preserve and verify the workspace
again, and restore the exact latest tree before compiling. Preserve Esperanto with
SHA256 e37ba559b4d9d761e74f2bb284d1ec762b813e789655c0495a882c83651890b5 and exclude
it from commits. Do not rebuild the delivered four phases. Rerun unconfirmed
DesignCapture, regenerate all nineteen landscaping captures from the published
SceneryPathParityTests driver using actual original assets, inspect every final PNG,
and deliver OpenRCT2-Pocket-Scenery-Landscaping-Captures.zip with its actual manifest.
Update this checkpoint/matrix with completed closeout results and publication history.

Remaining ordinary gaps stay explicit: continuous wall drag preview/release,
continuous repaint/eyedropper drag equivalents, desktop Shift automatic Z-search,
native asynchronous network scatter/paint-review completion/recovery and all broader
matrix gaps. Full ordinary gameplay parity, AArch64 and physical Brick results are
not claimed. Completed finance/research, guests, staff, map and construction/design
source and regressions are preserved.


## Final validation and capture closeout — 2026-10-14 UTC

This section supersedes the interrupted handoff above. All four requested
implementation phases and their capture closeout are delivered; production source
remains unchanged from canonical 3882849c5648fdc78c54ffc3279e07400117fe8a,
code tree 92c2accbabe7381f38f99bdb5c2a6549fd2c9b1c.

Execution returned at Staff db96e886 again. Working/index patches, index and Esperanto
bytes were preserved before recovery. Ninety-seven canonical blobs were verified and
atomically recovered. Complete indexed tree equality with canonical b2db7e0 / tree
0c51d3de826b209ef8ac9ffc73b404b476305fdf was verified; recovery safety
050db1e4c743ef40a2b545e69446ae61bc8ac66c was committed before compilation.
Production source stayed fixed throughout compilation/testing. Native Release GUI/CLI
build PASS; asset-backed native/SDL SceneryPathParityTests rerun returned exit 0,
generating all nineteen landscaping images. CTest rerun 3/3 PASS: handheld-input,
handheld-launcher and handheld-save-preservation.

Previously unconfirmed DesignCapture was rerun on Africa - African Diamond Mine.SC6
and returned confirmed exit 0. Actual native authoring eligibility/UTF-8/round-trip/
repository/stale file/source/controller/cancel and picker/research/paging/ghost/
rotation/cash/stations/access/testing/save-reload assertions passed. Driver blob:
fe7e11da3fdccc098c534a46c1ad277083d2af3e. Landscaping driver blob:
9457e3def513243bebc69ac7ef8cc666282f4348. Independent fixture links reused completed
GUI objects and removed only the GUI dependency-file output flag. SDL dummy renderer
warnings do not establish hardware results. The previous fourteen-driver regression
sweep and confirmed RideConstructionCapture, MazeConstructionCapture and original
Wooden/access/natural-guest CustomCoasterCapture passes remain recorded at this same
production source; those already verified drivers were not unnecessarily rerun.

All nineteen newly regenerated final 512×384 PNGs were individually inspected for
clipping, overlap, focus, contrast, controller controls and HUD collisions. No new
material defect required a production change. Long native names/option values retain
intentional bounded ellipsis. 01–03 show eyedropper tool entry, native picked-banner
placement handoff and dense options; 04–07 controller keyboard, native eligible
result/preview, empty search and clear recovery; 08–13 shared native scatter controls,
ghost, review, actual partial placement/cost, native failure recovery and larger
area preview; 14–19 supported native surface/edge colours, native icons/area,
Cancel-first quote, actual result, stale review and unsupported channels.

OpenRCT2-Pocket-Scenery-Landscaping-Captures.zip is completed and saved: nineteen PNGs,
README and dimension/SHA256/provenance manifest, 133606 bytes, SHA256
 e95c7afb7952d2464964ee09e6fe162b81fc3b71130500e6fcc2bdc21f29c5b8.
Manifest records every actual image checksum/dimension, visual inspection, source
identity, native renderer/fixture and validation provenance. Original g1.dat SHA256:
a78acefbcd1b0f5c8dbbf201afb8cf1a3d9294505134efa155c1323242c55a21.
Private archives, source parks and generated parks/designs are excluded. These are
production native X8 viewport/Pocket component fixtures with original sprites and
native costs/results, not whole SDL/hardware frames. No physical Brick/AArch64,
sustained dense-park performance or sustained operation of every ride family is claimed.

The workspace reverted again before documentation publication. The saved package
survived and was recovered with its exact ZIP checksum and all nineteen image hashes
verified. Completed test outcomes were observed before that loss; missing logs are
not described as retained artifacts or new test runs. Current remote remains b2db7e0.
Another preservation/recovery verified every published blob and complete source-tree
equality; safety 59b722dd42de03162ced472e0375271371dd13f2 matches tree 0c51d3d.
Partial recovery temporary files were moved to the preservation archive. Esperanto
retains SHA256 e37ba559b4d9d761e74f2bb284d1ec762b813e789655c0495a882c83651890b5
and is excluded. No clone/reset/clean/restore/discard/history rewrite/force-push was used.
This documentation-only closeout verifies current native parent, every changed blob,
complete local/canonical source-tree equality and expected_sha/force=false publication.

Exact remaining ordinary gaps are unchanged: continuous wall drag preview/release,
continuous repaint/eyedropper drag equivalents, desktop Shift automatic Z-search,
asynchronous native network scatter/paint review completion/recovery and broader
matrix gaps. Scenery/landscaping and overall 100% ordinary gameplay remain PARTIAL.


## Verified closeout publication

Canonical closeout `6033832f91c7050c3ec35c92b70922b784913244`, local safety
`03a966a3e3e2f0d938827ffe8360828268586f9c`, exact matching complete source tree
`5cdc86afb52088f6ee36f097e307b364f5607e09`. Native parent b2db7e0, both changed
blob hashes, local/native parent trees, new complete tree and canonical commit parent
were verified before expected_sha=b2db7e0 / force=false fast-forward; the resulting
branch ref was read back and matched 6033832. This subsequent history-record commit
changes documentation only and preserves all tested production code and ZIP bytes.
