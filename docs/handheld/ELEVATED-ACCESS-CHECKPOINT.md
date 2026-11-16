# Elevated ride access checkpoint

Continues `handheld/modern-ui-paths` from
`087ab3a92d51e32390aea343a5e0cc89d9974866`.

## Playable workflow

Existing ride inspection → Access / paths → entrance queue or exit normal path
→ existing Path Options height/slope controls → native placements → B returns
to Access → native connectivity review → Cancel-first opening confirmation.
Leaving Access entirely is safe. Reopen ordinary ride inspection later and choose
Access / paths to return to the current native station landing. Real unfinished
paths survive save/reload; construction does not need to remain active.

Mappings and presentation are unchanged: D-pad moves the view/target; A places
once; X toggles removal; Y changes surface; Select opens existing Path Options;
B leaves; Start retains Dashboard behavior; camera rotation/zoom remain native.
Existing height, slope, rotation, follow-ground and other Path Options remain the
only configuration controls. The player builds the route manually; there is no
automatic routing feature.

## Native geometry and reusable behavior

- `RideAccess` derives landing tile, height, orientation and station from native
  entrance/exit geometry. Multi-station inspection prioritizes a station still
  missing a walking connection to a real park entrance. No ride ID, coordinates,
  scenario name or exported-design special case exists in production.
- Guided Paths use `ScreenGetMapXYWithZ` at the selected fixed height. An elevated
  landing focuses the actual station plane rather than terrain underneath it.
  Changing height through Options retains the current tile at the new plane.
  All native camera rotations/zooms are supported. Ground-following and ordinary
  Build Paths keep their existing sprite/terrain hit testing.
- Guided placement supplies the incoming direction from the previous real path
  or compatible native station/adjacent path geometry. The same direction is
  used for query, ghost and real action. Native `FootpathPlaceAction` handles
  intervening wall clearance, connections, supports and price. Pocket never
  patches path edges, queue associations or walls itself. A successful action
  records only a transient preceding tile; late callbacks cannot restore exited
  construction state.
- Real reciprocal path edges and native slope endpoint heights provide read-only
  walking connectivity feedback. Ghosts and other rides' queues are excluded.
  Merely reaching an isolated normal path no longer reports a park connection.
  This feedback does not override native ride opening or guest pathfinding.
- Explorer validates the original guided ride identity, station and access-point
  position/orientation before targeting or acting. Removed/moved/stale access
  stops the tool and clears its ghost. An inspection changing its preferred
  station after a successful connection does not invalidate the active route.
- Existing provisional, menu, pause, focus-loss, park-load and shutdown cleanup
  remain responsible for path ghosts. No new serialized construction state or
  save format was introduced.

## Files

- `src/openrct2-ui/handheld/Paths.cpp`, `Paths.h`: height-plane targeting,
  stable height refocus and native connected placement direction.
- `src/openrct2-ui/handheld/RideAccess.cpp`, `RideAccess.h`: native network feedback,
  multi-station prioritization, landing handoff and geometry validation.
- `src/openrct2-ui/handheld/Explorer.cpp`, `Explorer.h`: guided target identity and
  stale-point protection.
- `test/handheld/ElevatedAccessCapture.cpp`: real-scenario integration and captures.
- `tools/handheld/render_elevated_access.py`: reproduce that native validation.
- This checkpoint document.

## Validation

Native GUI/CLI Release build passes. The existing `run_native_test.py` suites
pass both with real private RCT assets and with graphics disabled. They retain
coverage for all construction tools, path ghosts/options, research filtering,
flat rides, optional ride access, all management sections, held-button input
handoffs, native pause/no-money accounting and save/reload. Existing CTest
input/launcher/save-preservation checks pass **3/3**. Python compilation and
`git diff --check` pass.

The saved-design scenario regression also passes: genuine native export/import,
eligibility, ghost/research/cash, station recovery across two stations, elevated
landing targeting, Dashboard testing and save/reload. The existing native import
logs its documented rejected zero operation option; this pass does not alter it.
SDL dummy display warnings are expected in offscreen tests, not physical Brick
validation.

The new integration uses the original private **Mines of Africa** scenario and
its original two-station wooden coaster. Track, stations, operating settings,
ownership and guest state are not edited. After legally removing eleven native
queue tiles through queried removal actions, it rebuilds the lower station's
missing park connection using twenty normal-path pieces (including a native
rise and elevated supported tiles) and three queue pieces. Candidate tiles are
queried through native actions; test-only route search chooses a legal corridor
and drives real controller semantic placement. That search is not in the game.
Every real placement asserts its exact queried cash charge; previewing asserts
unchanged cash. Duplicate semantic A actions cannot place twice.

New checks cover all native zooms/rotations, height-change target stability,
invalid-target ghost removal, native queue/park connectivity, cancellation,
leaving construction entirely, inspection recovery, an unfinished-route
save/reload, Cancel-first opening/cancellation, exact zero-cost opening and final
save/reload. A native external entrance removal then verifies stale-target A
cannot place or retain a ghost. Existing held-button lifecycle tests are retained.

Native simulation observes guests genuinely queue **at the rebuilt lower
station**, then ride, then walk after exiting; guest identities link the phases.
No guests are teleported, given destinations, changed or forced onto the ride.
The boarded capture follows that guest's actual native vehicle because an
on-ride guest has no world position. Native progression continues normally.

Reproduce with private assets and outputs outside git:

```sh
python3 tools/handheld/render_elevated_access.py --build BUILD --data ENGINE_DATA \
  --assets PRIVATE_RCT --scenario SCENARIO --output EXTERNAL_OUTPUT --ninja NINJA
python3 tools/handheld/run_native_test.py --build BUILD --data ENGINE_DATA \
  --assets PRIVATE_RCT --ninja NINJA
python3 tools/handheld/run_native_test.py --build BUILD --data ENGINE_DATA --ninja NINJA
python3 tools/handheld/render_designs.py --build BUILD --data ENGINE_DATA \
  --assets PRIVATE_RCT --scenario SCENARIO --output EXTERNAL_OUTPUT --ninja NINJA
```

Nine captures are exactly **512×384**: queue ghost, elevated exit ghost, connected
bridge, connected park, connectivity review, Cancel-first opening, genuine queue
and genuine riding/exit. They use native scenario graphics
and production handheld panels, not fixture world art. Private assets, park files
and exported designs remain outside git.

## Limits / next workflow

Routing is manual and follows existing Paths controls; this is not a connected
bridge/tunnel builder or route planner. Connectivity feedback describes a
reciprocal walking path graph to a park entrance, not a full simulation guarantee
or transport-aware reachability check. Native no-entry signs, queues, crowding,
ride prerequisites and guest decisions retain their own rules. Existing native
opening validation remains authoritative. The scenery around this scenario's
station can occlude guests from some camera angles.

Supported access scope remains the existing flat rides and supported wooden
coasters. No track geometry, operating settings, UI palette or mappings change.
Physical ARM/Brick performance and ergonomics still require hardware testing.

Suggested next complete workflow: Dashboard Rides → inspect an existing
non-shop ride → review native demolition/refund → Cancel-first explicit
confirmation → native removal and roster/park cleanup → save/reload. Reuse shop
confirmation patterns while preserving all neighbouring paths/scenery and native
ride removal rules; do not expand into custom track editing yet.
