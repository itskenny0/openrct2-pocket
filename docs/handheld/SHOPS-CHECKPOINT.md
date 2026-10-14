# Controller Shops / Facilities placement — 2026-10-07

Continues the tree picker source checkpoint `c9f6b0c17c9166afbf3471601f7ae683cde86989`
on `handheld/modern-ui-paths`. This adds a playable selection → preview → turn →
place/open → save/reload workflow. Existing tools, picker styling, research and
world rendering are retained. Private RCT assets stay outside git.

## Build, picker and controls

Build now offers **Shops / Facilities** after Trees. It opens the same ObjectPicker
three-column/six-card paged grid, using already loaded native RideObject previews
and names. D-pad navigates; A selects and enters placement immediately; B returns
to Build without activating a tool. Start opens the existing pause page.

In placement: D-pad targets ground; A places one building; Y reopens the picker;
Select turns the building through four orientations; B exits; Start opens pause.
X still rotates the camera, and L/R still zoom. No other mode's controls change.
Reopening focuses the current object. B or Start discards unconfirmed focus and
retains the current object/orientation. Selecting A suspends the input stream;
held A and its release cannot purchase anything. A fresh press is required.

The shared menu grid dispatcher now serves both Trees and Shops. Category adapters
supply eligible entries; gameplay remains outside ObjectPicker. Existing tree APIs
remain compatible. No other picker conversions or abstract framework are added.
The Build list displays at most eight rows and scrolls to its retained Back entry.

Only loaded, researched native shop/facility ride types using a one-tile
flatTrack1x1A/B start piece are eligible. Both native ride-type and ride-entry
research flags are checked, with the engine's existing ignore-research cheat
honoured. Availability is refreshed on navigation and revalidated on selection
and at queued execution. Empty/stale lists cannot activate anything. Production
never loads objects, inserts research or overrides native validity.

## Placement and transaction

The compact rail, cost badge and 28-pixel strip reuse the solid palette tokens.
There is no texture/dithering or replacement art. Default colours and ground
height follow native shop construction; MapGetHighestZ accounts for ground slope
and water, then native TrackPlaceAction determines legality and support costs.
No manual height, recolour, brush, flat ride or coaster construction is included.

ShopPlaceAction is a registered, serialized queued composition of native
RideCreateAction, TrackPlaceAction and RideSetStatusAction. Its query uses native
create/track queries; its executor creates one initialized ride, places its one
native track block, then asks the native status action to open it. Opening still
uses native validation. Nested operations do not charge; the outer command returns
the native construction cost and charges once. Failed track execution discards
its newly created empty record. The native actions own ownership, clearance,
pause, funds, prices, path connections and operating rules. The command ID is
appended, preserving existing command IDs. No existing simulation action changes.

A shop does not need a separate entrance/exit construction tool. It must face an
appropriate adjacent path to serve guests. A valid isolated placement can open,
but normal native path-access rules still determine whether guests can use it.
Management and pricing interfaces are later work.

## Native ghosts and lifecycle

TrackPlaceAction with native ghost/noSpend flags draws the actual building and
orientation. Native track rendering requires a ride record. PreviewRide borrows
an unused native slot with default type/subtype/colour/station state, avoiding
RideCreate's naming/random/pricing side effects. Cleanup restores the exact old
slot contents. No permanent empty ride or money change remains. Ordinary queries
ignore ghosts and enforce real constraints; unchanged requests retain one ghost.
A query can reuse its own preview record rather than allocating another slot.
Target/height/rotation/object changes replace it; invalid/unavailable/funds/pause
states clear it. Only the tool-owned ghost identity is removed with native
TrackRemoveAction, never an existing real ride.

Menu/picker entry, focus loss, tool changes, progress/load/map changes, and shutdown
clear both preview and borrowed record. Native provisional remove/restore hooks
cover save/autosave, using the existing ScenarioSave guard. Nested suspension and
clear-during-suspension cannot resurrect an exited tool.

A shop preview record must also be absent from finance, ride ratings and queued
construction, not just the pedestrian update. Shop-specific UI intents suspend it
around the entire gameStateTick and gameStateUpdateLogic scopes, including paused
queues/network/snapshot/autosave paths. Nested scopes restore only after the outer
scope finishes. Other provisional tool timing and desktop construction remain
unchanged; UI-free contexts receive no-op intents. Native preview suspension can
recreate a ghost between simulation updates, as existing provisional tools do;
unchanged presentation updates do not rebuild it.

## Validation and captures

Native GUI/CLI build and existing three CTest suites pass. Expanded native
integration passes headless and with the real UI context, private assets and SDL
dummy drivers. All earlier terrain/water/surface/removal/path/tree/picker/ghost and
save/research checks remain enabled.

New checks cover eligible-only paging, empty/stale research filtering, Build entry,
selection/cancellation/Start, actual SDL held-A/release/fresh-A handoff, native
ghost identity/unchanged retention/rotation/target replacement, nested suspension,
invalid targets/ownership/funds/pause/clearance, neighbouring terrain/path metadata,
exact charges, one real shop and facility, opening, autosave without ghost/empty
ride, manual save/reload, focus/tool/load/shutdown cleanup, and comparison of a
native simulation tick with/without a ghost. Fixture objects and research-list
setup exist only in tests. The earlier unloaded-surface error remains expected.

Reproduce with tools/handheld/run_native_test.py --build BUILD --data ENGINE_DATA
--ninja NINJA; add --assets PRIVATE_RCT_ROOT for actual UI/controller lifecycle.

Three exactly 512×384 PNGs from the user's **Mines of Africa** show the production
picker, Burger Bar ghost with native $30.00 query, and one real/open Burger Bar facing an existing normal path.
The loaded scenario offers 16 eligible shops/facilities across three pages. These
are automated native scenario snapshots, not Brick/controller session captures.
No fixture objects/research are added to captures, no proprietary assets are
committed, and the source scenario is never overwritten. The harness asserts
unchanged preview cash, one real placement, native open state, exact charge and
the native connection edge on that adjacent path.

Reproduce with tools/handheld/render_shops.py --build BUILD --data ENGINE_DATA
--assets PRIVATE_RCT_ROOT --scenario SC6 --output DIR --ninja NINJA.

## Limits and next complete workflow

Only native one-tile shop/facility objects are supported. Discrete native sprite
zoom/ellipsis match the existing picker. Normal path access and world occlusion
remain native. Multiplayer/replay and physical Brick verification are not exercised
by this host build (network/scripting are disabled); no fresh ARM package is claimed.

The next complete player workflow is **shop/facility inspection and operation**:
inspect a placed building, read its native sales/profit/status, open/close it,
adjust its native product/admission prices, confirm demolition with queried
refund, and save/reload the result. Keep it scoped to shops/facilities, reuse the
existing handheld Ride inspection entry and solid style, and preserve all completed
construction. No naming keyboard or broad park-management redesign is needed.

## Shop appearance / preview lifecycle closeout supersedes earlier next steps

Shop placement, management and shared naming remain. Native object-supported
building colours and random merchandise now have controller-native inspection,
paged palette, Cancel-first native cost review, exact allocation/object/baseline
guards and persistence. See SHOP-APPEARANCE-COMPLETION.md. The paused full-shell
preview discrepancy was reproduced: native focus/modal/input-reset cleanup must
not resurrect ghosts. Documented and forced-shell PathActions pass; nested
provisional/autosave/real map/cash/borrowed-ride preservation remains covered.
No new independent simulation or preview save state was introduced.
