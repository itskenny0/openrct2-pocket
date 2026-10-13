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
