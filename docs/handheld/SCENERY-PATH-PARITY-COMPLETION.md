# Scenery and Path Construction Parity — incremental checkpoint

Goal: 100% ordinary OpenRCT2 gameplay through controller-native Pocket at 512×384.
Base canonical `8008b9c32fe7052d9a2b4ed6045202fc5be060d9`, local
`52033feb483492ba20f8fc0d008e740f14466830`, matching tree
`bdd51dab6f2e60e95789918f36ed91bef26c117e`. Esperanto remains untouched and excluded.

## Phase 1 — scenery groups and native sign styles

Scenery options adds Filter scenery group. Loaded, researched groups with a loaded,
researched member of the current native category enter the list. All available
objects remains the default. Membership comes from SceneryGroupEntry::SceneryEntries;
IsSceneryAvailableToBuild and SceneryGroupIsInvented remain authoritative. No
research/object state is modified in production. Category changes reset the filter;
current objects are retained when still members. A missing/unresearched group is
not silently presented as an empty completed category. Existing trees remain intact.

Point at a real banner/scrolling wall/large sign, open options → Edit sign style.
Native banner no-entry, text colour and supported primary colour use
BannerSetStyleAction. Wall/large sign supported primary/text channels use
SignSetStyleAction. Native no-entry changes path allowed-edge routing flags through
its action. Each draft changes one setting; no compound atomicity is invented.
No-entry is offered only for native path banners. Unsupported channels reject.

D-pad chooses; A opens/adjusts; Left/Right adjusts one bounded value; A on Review
opens Cancel-first confirmation; B returns/discards. Each action revalidates copied
location/object/part/banner identity, loaded object descriptor, colours, text,
banner flags, linked ride and text colour. Native query owns ownership/pause/errors.
Repeated or changed drafts reject; no unsafe map pointer survives actions. Settings
persist through native park serialization; no Pocket gameplay state is serialized.

Validation: Release GUI/CLI, SceneryPathParityTests production SDL/native fixture
checks actual membership, missing groups, BannerSetStyleAction no-entry/text colour,
SignSetStyleAction scrolling-wall colour, unsupported wall no-entry, invalid bound,
changed text/repeated drafts, native accounting, Cancel-first duplicate-A handling
and save/reload. Original sprites are privately available. Captures are genuine
native X8 viewport plus production Pocket components from initialized fixtures,
not whole SDL frames or physical Brick evidence. Existing ConstructionTests and
PresentationLifecycleTests are required before this phase publication.

Full scenery/path parity remains PARTIAL. Area clearing and connected entrance/exit
starts/undo are the next independent phases; arbitrary filter search, eyedropper,
bulk placement, full banner/sign exhaustive object validation and physical Brick
performance remain on the 100% roadmap. Publication SHAs and final capture manifest
will be appended after guarded publication.


## Recovery and Phase 2 — native area clearing

Phase 1 was published as `83588d25266100e9f3bc896b3730943467ac03c1`, tree
`5931f7b7db57eee099259cc618a4ecc5b52856bc`. A workspace rollback removed the
unpublished area-clear changes. Recovery local commit
`f353bce3849b82076b9f2920226fcc7c50864d9a` exactly matches that published tree;
Esperanto was preserved. Phase 2 was recreated and freshly validated.

Build → Scenery → Clear area uses the existing native rectangle selector. Select
opens options; Left/Right changes bounded 1–64 tile width/height; A toggles native
small/large/wall/path/furniture categories. Paths and furniture default OFF.
A in the park opens a Cancel-first review of selected elements and queried cost.
B cancels without clearing. Native ClearAction determines ownership, skipped
obstructions, whole large-object removal and accounting. This is not an atomic
bulk operation: native partial-area skips remain visible in the review description.

Reviews copy element data and loaded scenery descriptors, including every native
large-object sequence outside the brush. Changed geometry, styles, cash, park flags
or quote invalidate the review. No map pointer survives actions. Successful reviews
are consumed. Menu-owned pause is released for native query/execute; externally
owned pause is preserved. Native rectangle selection is cleaned when the tool ends.

LargeSceneryRemoveAction now accounts all native sequences once after footprint
validation. Previously a multi-tile ClearAction query marked the same selected
sequence repeatedly and incorrectly omitted its removal cost/refund. The native
execute price is unchanged. A native two-tile regression checks full and partial
brush quotes against exact cash, and removal of the sequence outside the brush.

Phase 2 regression also covers invalid bounds, empty categories, changed quotes,
native recolour staleness, Cancel-first controller cancellation, repeated submit,
path preservation by default and explicit opt-in path removal. Capture driver uses
private original sprites with production viewport/menus over action-built fixtures.
Sign style menus now hide unsupported native channels rather than offering a
setting which can only reject.

Phase 2 publication validation: Release GUI/CLI build PASS; SceneryPathParityTests
(native actions + production SDL/controller + private sprite captures) PASS;
ConstructionTests and PresentationLifecycleTests PASS after recovery.

Connected native entrance/exit starts and connected undo remain incomplete pending
the next independently validated phase. Terrain surface colour remains a documented
gap. Full scenery/path desktop parity remains PARTIAL.


## Phase 3 — connected access starts and guarded native undo

Rides → Access appends Connected entrance queue and Connected exit path, preserving
all previous indices/workflows. It uses the actual selected station's native
outside tile/height/direction. The first landing stays fixed until explicit A;
existing native paths are selected without deleting/replacing them automatically.
Native FootpathPlaceAction supplies queue association, slope/elevation, connection
edges, cost/error and real placement. Subsequent directions/grades reuse the
existing sequential adapter. No independent routing or automatic route is added.

Select opens path options: Resume, Turn, Rise, Descend, Flat, Restart endpoint,
Change surface, Undo section. All eight rows now render in the compact indigo rail,
including the focused undo row. Undo opens a native FootpathRemoveAction quote with
Cancel initially selected. Only new sections created in the current session enter
history; joining/converting an existing path clears history. No existing junction
can be deleted through this undo. A/B review/cancel preserves the previous context.

Copied native path baselines, session revision, cash, queried refund and copied
ride generation/type/object/date/station/outside identity are checked again.
Pending actions reject repeated input. Successful removal restores the previous
endpoint while retaining the current direction/grade choice (including a removed first access landing); adjacent native
edges are refreshed after the action. Menus preserve context and remove provisional
ghosts; load/exit ends transient tools/history. Real paths persist in native saves.
No Pocket gameplay state is serialized.

Coverage: ConstructionTests exercises flat/rise/elevated/descent build, native
undo query/execute/refund/repeat/context recovery and existing-junction protection.
FamilyOperationTests uses production RideAccess handoff and native queue/normal
landing actions for Monorail, Dinghy Slide and Flying coaster, stale generation,
held confirm, Cancel-first undo, explicit rebuild, elevated extension, native
station association and Open/save. RideAccessWorkflowTests tests menu connected
handoff without accidental placement and B recovery. These fixtures demonstrate
native connectivity/access, not sustained guest throughput of the new routes.

Phase 2 canonical `b216ca3e17185a11407914ef19f09c8dcc221f63` / local
`c4e2845a6c4b3b3fba924d242db956ea8f93d777` share tree
`3cf89bb707f018908b1c561acded4a2af7dd5ed8`.

Remaining ordinary gaps: scenery eyedropper, broader search/filter presentation,
bulk placement, terrain surface colour draft, exhaustive sign/object combinations,
connected history recovery after save/load and richer queue/normal junction editing.
Existing native placed paths remain inspectable/removable after load; transient
undo history is intentionally not serialized. All prior advanced ride/design gaps
remain in their checkpoints. Full ordinary scenery/path parity remains PARTIAL.


## Validation and limitations at Phase 3 publication

Release GUI/CLI PASS. Native/SDL PASS: SceneryPathParityTests, ConstructionTests,
PathActionsTests, PresentationLifecycleTests, RideConstructionTests,
SeatRotationTests, FamilyOperationTests, MazeConstructionTests,
MiniGolfConstructionTests, StaffCompletionTests and GuestCompletionTests.
CTest input/launcher/save-preservation PASS 3/3. A stricter whole-history path guard
rejects externally changed native edges; explicit fresh endpoint selection recovers
construction. Own native placement/removal refreshes retained edge baselines.

Private African Diamond Mine DesignCapture, RideConstructionCapture and original
Wooden CustomCoasterCapture PASS; Factory Capers MazeConstructionCapture PASS.
The original Wooden regression observes a natural queue participant over its
simulation window; this does not prove sustained throughput on the new connected
routes. An extra ElevatedAccessCapture attempt on Factory Capers correctly rejected
because that scenario has no supported researched coaster for its fixture; no
research or live park state was patched to make it pass.

Private original sprites and objects are available. New captures are production
native viewport + Pocket components over native action-built initialized fixtures,
not full SDL gameplay frames, actual original scenarios, or physical Brick captures.
No AArch64 build, physical Brick input/performance or sustained new-route routing
is validated in this run. Dummy SDL hardware-renderer warnings are not hardware
validation. Private original archives/parks/exported test designs are excluded.

Capture list: 01 native scenery group filter; 02 no-entry Cancel-first review;
03 native banner text colour draft; 04 area selection; 05 dense native categories;
06 quoted clear/refund review; 07 after native clear; 08 queue access landing;
09 normal exit landing; 10 native undo review; 11 elevated access extension;
12 dense eight-row path controls; 13 genuine invalid area-bounds feedback.
All 512×384 PNGs are visually inspected; category On/Off states, review labels,
landing prompt and undo focus clipping were fixed and affected captures regenerated.
Final package contains dimension/SHA256/provenance manifest. Final publication SHA
is recorded externally to avoid a self-referential source commit hash.


## Published implementation history

| Phase | Canonical commit | Local safety commit | Matching source tree |
| --- | --- | --- | --- |
| Group filtering/styles | 83588d25266100e9f3bc896b3730943467ac03c1 | f353bce3849b82076b9f2920226fcc7c50864d9a (recovery) | 5931f7b7db57eee099259cc618a4ecc5b52856bc |
| Area clearing | b216ca3e17185a11407914ef19f09c8dcc221f63 | c4e2845a6c4b3b3fba924d242db956ea8f93d777 | 3cf89bb707f018908b1c561acded4a2af7dd5ed8 |
| Connected starts/undo | be0bca16661905774608d5c640f758f373ab6322 | 679a802e62b91323ea3c14074cf985d6e20caa1a | 205d8ae411920019eb58314a3c560e8c323d2c94 |

All updates used verified remote native parents, exact tree equality and guarded
expected_sha/force=false fast-forward publication. Esperanto SHA256 remains
`e37ba559b4d9d761e74f2bb284d1ec762b813e789655c0495a882c83651890b5`.

Recommended next substantial subsystem: native Park Map and Information, currently
missing as a Pocket player workflow in the matrix. Reuse native map rendering,
legends, object/peep overlays, news/awards and park histories; link existing locate,
roster and construction flows rather than repeating delivered features. Retain
all unfinished ordinary construction, design, guest and staff functionality.

Continuation (2026-10-10): the recommended native Park Map and Information core now has controller navigation, native map/peep layers, linked subject inspection, news/awards and park history graphs. See PARK-MAP-INFORMATION-COMPLETION.md and the updated matrix; construction gaps above are unchanged.
