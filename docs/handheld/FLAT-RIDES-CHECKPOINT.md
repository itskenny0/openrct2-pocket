# Flat ride construction checkpoint

Continues canonical GitHub checkpoint `fd53fd67b76c163e86d633951e7196c6262661dd`
on `handheld/modern-ui-paths`. This milestone adds one complete native flat-ride
construction flow without reworking completed tools, management sections or UI.

## Player workflow and supported scope

Build → Rides → native visual picker → place/rotate the ride → Entrance → Exit
→ park → Dashboard / Rides → inspect/locate/open through existing management
→ save/reload.

The existing six-card, paged ObjectPicker renders native RideObject previews and
names. Only loaded, researched ride entries with a supported native flat type
appear. Both ride-type and object research are checked; native ignore-research
cheats retain their native meaning. No objects are loaded by gameplay code.

Supported types must have `isFlatRide` and `hasSinglePieceStation`, without
`isShopOrFacility` or `hasTrack`, and a supported complete single-piece flat
footprint: flatTrack1x4A, 2x2, 4x4, 2x4, 1x5, 1x1A, 1x4B, 1x1B, 1x4C or 3x3.
A loaded station object is required where the native ride requires entrances and
exits. Towers, mazes, track rides and special multi-piece construction are not
presented as supported. The first loaded station object is used; this workflow
has no station-style editor or object-loading fallback.

The unmodified private Mines of Africa scenario offers seven eligible entries:
Diamond Ride, Merry-Go-Round, Crooked House, Dodgems, Top Spin, Enterprise and
Spiral Slide. This is that scenario's loaded/researched set, not a global limit.

| Mode | Controls |
| --- | --- |
| Picker | D-pad grid/page navigation, A select, B cancel/back, Start existing pause menu |
| Ride placement | D-pad target, A one placement, Select rotate, Y reopen picker, B exit/back |
| Entrance / Exit | D-pad target, A one valid placement, B safely leave unfinished ride |
| Camera | Existing X camera rotation and L/R zoom remain available in construction |

Start retains the existing construction-to-Dashboard/pause/menu behavior. Select
is consumed without changing anything during entrance/exit placement; direction
is resolved from native station adjacency. The placement A press is suspended
across each stage, requiring release and a fresh A press.

A purchased ride starts closed. Leaving an unfinished ride never silently
refunds or demolishes it. The existing Dashboard Rides inspection appends
**Finish entrances/exits...** only for a closed supported ride missing one of
those elements. It locates the ride and resumes the missing phase. Existing
inspection action indices and gameplay remain intact. Completing the exit ends
the tool cleanly on the next update, without a one-frame terrain panel fallback.
Rides without native entrance/exit requirements finish immediately after placement.

## Native actions and preview ownership

`FlatRidePlaceAction` is a registered/serialized composite of native
`RideCreateAction` and `TrackPlaceAction`. Query checks native creation and the
full native footprint using the existing borrowed preview-record mechanism.
Execute creates exactly one real ride, executes the native single-piece track
placement, and returns its queried cost through the outer action for one cash
charge. A failed track execution deletes only its just-created empty ride.
Ownership, clearance, supports, pause, funds, object validity and pricing remain
native. Ground height comes from the maximum native surface height under the
rotated footprint; there are no new height controls.

`TrackPreview` generalizes the existing shop ghost owner for both Shops and flat
rides: native ghost/noSpend TrackPlaceAction, native ghost/noSpend TrackRemoveAction
and a borrowed ride record restored when the owner clears. Existing shop adapters,
validation and suspension behavior are preserved. Flat requests retain their ghost
when target/object/orientation are unchanged, but still requery native validity
so changes to funds, pause, ownership or research cannot leave a misleading preview.

`FlatRideEntrancePlaceAction` wraps native `RideEntranceExitPlaceAction`, with a
serialized ride/object/build-date/station/entrance snapshot. It additionally uses
native track sequence entrance-side metadata to require a genuine station-adjacent
connection at the station height. The generic native entrance action alone does
not enforce the construction UI's adjacency rule. Native action queries still
own clearance, permission, funds and cost. Exit placement requires the existing
entrance and rejects duplicate/stale station state.

Entrance/exit previews use the game's native tile selection and direction arrow,
not an entrance-building ghost. Invalid targets show native errors and no valid
selection arrow. This avoids changing temporary station entrance references or
path connectivity during simulation/save. Ride footprint previews are actual
native ghosts, not substitute artwork.

`FlatRides` owns stage, selected object/orientation, generation-validated management
target, queued-action busy/revision guards and preview request state. `Explorer`
reuses current center targeting and semantic input suspension. `Menu` routes the
new entry through the existing picker rather than a separate component. Menus,
picker transitions, focus loss, tool changes, load/park changes and shutdown clear
preview ownership. Existing provisional save guards and temporary simulation
suspension include flat ghosts with nested depth handling; clearing during a
suspension prevents resurrection. Selection survives reopening the picker without
creating a second ride.

No simulation rule, native world graphics, research progression or financial
formula is replaced. New native actions are appended to the action enum/registry;
existing action values remain unchanged.

## Presentation and changed files

The existing solid palette is unchanged: panel/action strip 118, border/separator
119, focus gold 53, primary text 21, secondary text 18. No dithering. Construction
uses the compact 164-pixel rail, native cost/error badge and 28-pixel action strip.
The park remains dominant. Dashboard management keeps its existing shell.

New source:
- `ride/FlatRidePlacement.h`: supported eligibility, station objects and native adjacency.
- `actions/ride/FlatRidePlaceAction.*`, `FlatRideEntrancePlaceAction.*`: native action compositions.
- `openrct2-ui/handheld/FlatRides.*`: construction stages, queries, preview lifecycle and compact panel.
- `openrct2-ui/handheld/TrackPreview.*`: shared shop/flat ghost owner.
- `test/handheld/FlatRideWorkflowTests.h`, `FlatRideCapture.cpp`.
- `tools/handheld/render_flat_rides.py`.

Modified source:
- `GameCommand.h`, `GameActionRegistry.cpp`, `ShopPlacement.h`.
- `Explorer.cpp/.h`, `Menu.cpp`, `RideManagement.cpp`, `Shops.cpp`.
- `ProvisionalElements.cpp`, `WindowManager.cpp`, `PathActionsTests.cpp`.

## Verification

- Native host GUI and CLI builds pass.
- Full UI-linked private-asset integration passes, including every prior suite.
- Full engine-only linked integration passes, including every prior suite.
- CTest input, launcher and save-preservation suites pass **3/3**.
- Capture tool Python compilation and `git diff --check` pass.

New tests exercise eligible object/type research filtering, multiple picker pages,
selection/cancellation, Start/pause, duplicate semantic A and actual mapped SDL
held-A/release/fresh-press handoff. Every eligible fixture footprint is queried and
previewed in all four orientations. Coverage also includes unchanged ghost retention,
rotation/target replacement, ownership/funds/pause rejection, nested preview
suspension, ghost-free save/reload and exact cash preservation while previewing.

The full Merry-Go-Round integration tests one real placement with exact native
cost, duplicate/in-flight rejection, real station sides, disconnected/stale/paused
entrance rejection, entrance and exit one-time costs, no exit over entrance,
unfinished ride resume, no duplicate completed exit, native roster/locating,
native successful opening, stale management identity rejection and completed
ride/status/accounting/footprint save/reload. Existing shop/path/tree lifecycle and
all management regression suites remain enabled.

Reproduce linked tests after a native GUI build:

```sh
python3 tools/handheld/run_native_test.py --build <gui-build> \
  --data <engine-data> --assets <private-rct-install>
# Omit --assets for engine-only lifecycle integration.
python3 tools/handheld/render_flat_rides.py --build <gui-build> \
  --data <engine-data> --assets <private-rct-install> \
  --scenario '<private-rct-install>/Scenarios/Africa - African Diamond Mine.SC6' \
  --output <capture-directory>
```

The capture driver loads that real native scenario, renders production picker,
construction and Dashboard components at exactly **512×384**, and executes native
placement, entrance/exit and opening queries/actions with exact cash assertions.
It saves/reloads the completed ride and verifies its status and entrance/exit data.
Captures: `flat-picker.png`, `flat-ghost.png`, `flat-entrance.png`, `flat-exit.png`,
`flat-completed.png`, `flat-dashboard.png`. These are host native scenario renders,
not fixture screenshots, AI art or physical Brick captures. The ground-level
Merry-Go-Round costs native £460 at tile 640,768. Private assets and captures stay
outside git.

## Remaining limitations and next milestone

No ARM package/physical Brick or multiplayer verification is claimed. This host
build disables network/scripting/TTF/OpenGL. Full construction and save/reload are
proved for Merry-Go-Round; other eligible fixtures prove queries/ghosts/rotations,
not every ride's complete guest operation. Long card names are ellipsized, with
the selected object's full name in the existing picker caption.

Native opening succeeds with completed entrance/exit, but guests still require
queue and exit path connections. This milestone does not automatically build paths
or route guests. The existing independent native Paths tool is unchanged. The next
substantial player workflow should refine construction-to-path integration and
prove real guests can queue, ride and leave before attempting coaster architecture.
Do not recreate the picker/ghost/station workflow in that next pass.

Canonical publication must fast-forward the existing GitHub branch from the
Overview checkpoint above. The recovered local baseline has the identical source
tree `de04de776e042a9ae4de24b0ef5465fed96ae258` but reconstructed local ancestry;
the GitHub checkpoint uses the canonical Overview parent, with the same final
source tree as the tested local commit. No source or private asset is force-pushed.
