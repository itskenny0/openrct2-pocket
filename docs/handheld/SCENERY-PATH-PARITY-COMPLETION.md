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
