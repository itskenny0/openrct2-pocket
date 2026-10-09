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
