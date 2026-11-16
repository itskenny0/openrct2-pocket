# Flat ride access checkpoint

Continues canonical `handheld/modern-ui-paths` checkpoint
`9be21df3cd3b54ea11a3a1ce0c4baee650780c4b`. The interrupted source was compared
with recovery `2e199cecd576d10037bf0839b857471be4bcae7b`: all eight inspected
production/workflow blobs matched the surviving local edits exactly. Completed
placement, picker, ghosts, management and solid-palette presentation were retained.
The recovery branch remains an explicitly unfinished historical checkpoint.

## Optional, recoverable player workflow

Completing a supported flat ride's entrance/exit now offers the existing Dashboard
Rides shell's **Access** page. **Back to park** is the initial selection. Nothing
is built or opened automatically. Existing Dashboard ride inspection and direct
park ride inspection both offer **Access / paths...**, so leaving construction or
loading a saved park never requires rebuilding a ride to reconnect it.

Access offers entrance queue targeting, exit normal-path targeting, opening review,
ride inspection and return to park. Each path choice starts the existing Paths
mode with its existing surface defaults, targeting, ground following, queried
price/errors, options, removal, ghosts and controls. Native viewport locating
centres the outside access tile immediately, preserving zoom and rotation. The
selecting A press is suspended; a fresh press is required to place a path.
B from guided Paths ends the tool/ghost and returns to Access. Start and Select
retain their existing Dashboard and Path Options meanings. Pause/options leave
the optional context recoverable; changing tools, load, stale ride identity and
normal tool teardown discard it. No construction state is serialized.

The player still builds each tile. The interface does not generate a path route,
spend preview money, auto-convert surrounding paths, or move guests. Opening
routes through existing RideManagement drafts and Cancel-first confirmation.
Native status queries/actions remain authoritative, including prerequisites,
pause and restrictions; successful opening has no invented upfront charge.

## Native connectivity and honest feedback

`RideAccess.cpp/.h` is a read-only adapter over station coordinates and real native
path elements. It derives each outside tile from native entrance/exit direction,
checks native path endpoint heights and reciprocal edges, and excludes ghosts.
Entrance attachment requires a queue associated by the native path actions with
this ride and station zero. Queue traversal uses real reciprocal connections,
sloped endpoint heights, ride/station association and a visited set. It reports
whether that queue joins a normal path. Exit attachment requires a normal path.

The presentation distinguishes missing queue, attached queue needing extension,
queue joined to a normal path, and normal exit attachment. **Attached does not
prove an isolated normal path reaches the park network.** The page explicitly
asks players to link both access points to park paths. Native ride opening can
succeed without guest-reachable paths; this milestone does not change that rule or
mislabel native opening as proof of successful guest operation.

No new gameplay action, path algorithm, research bypass or simulation rule was
introduced. Real placements reuse FootpathPlaceAction; native path actions own
edge updates, queue association, validation and accounting. Existing path ghost
and provisional-save handling are unchanged. The palette remains panel 118,
border 119, selection 53, primary 21 and secondary 18, with no dithering.

## Files and architecture

New:
- `src/openrct2-ui/handheld/RideAccess.cpp/.h`: native access state/presentation and viewport-to-Paths handoff.
- `test/handheld/RideAccessWorkflowTests.h`: integrated native fixtures and controller tests.
- `test/handheld/RideAccessCapture.cpp`: real scenario construction/connections/guest observation/save checks.
- `tools/handheld/render_ride_access.py`: build-linked capture runner.
- This checkpoint.

Modified:
- `Explorer.cpp/.h`: optional context, completion/B handoff, stale cleanup. Existing
  semantic frame dispatch is extracted unchanged into `HandleExplorerInput`, used
  by the regular update and mapped-controller tests; no mappings or fake mouse.
- `Menu.cpp`: Access routing in the existing shell, inspection entries, existing opening review.
- `RideManagement.cpp`: append access entry for supported flat rides, retaining existing indices.
- `FlatRideWorkflowTests.h`: invoke the access tests and reacquire identity after reload.
- `PathActionsTests.cpp`: real-asset mode loads native graphics and configures 512×384.

## Verification and fixture corrections

- Native host GUI and CLI builds pass.
- Full UI-linked native integration with private assets passes, including all earlier suites.
- Full engine-only integration passes, including all earlier suites.
- CTest input, launcher and save-preservation suites pass **3/3**.
- Capture runner Python compilation and `git diff --check` pass.

The interrupted focus failure was a 0×0 context/viewport in a graphics-disabled
fixture, not evidence that native locating should be replaced. Real-asset tests
now load actual sprites and create a 512×384 SDL-dummy window at the access slice.
They close prior expected error windows and enter normal native input state
(the event loop normally transitions from reset). Native pixel hit testing checks
the exact outside tile at all four camera rotations and every configured native
zoom level. No hit-test assertion was removed or replaced with fake-mouse targeting.
SDL's dummy backend emits hardware-display resize warnings; native software
sprite hit testing and separately rendered 512×384 captures still pass.

Coverage includes automatic optional completion, safe initial return-to-park,
direct inspection recovery, semantic duplicate actions, actual mapped SDL
held-A/release/fresh-A and held-B handoff, B recovery, pause/options ghost cleanup,
native queue association, dangling-vs-joined feedback, exit queue rejection,
one real path per confirming frame, exact queried monetary costs, ownership/funds/
pause rejection, no-money cash preservation, Cancel-first review and confirmed
native opening, stale/reused identity rejection, and save/reload with an actual
live path ghost. Real UI loading directly tears down the tool. DummyUiContext
has no scene WindowManager; engine-only loading explicitly uses the same tool-exit
hook as the handheld load menu after saving, while still checking saved ghost-free
paths, association and exact cash. Existing autosave/provisional-neighbour tests
remain enabled in both full suites.

## Native scenario proof and captures

The previous picturesque location at 640,768 lacked a level route. The capture
driver instead searches unmodified scenario terrain for clear, owned native
footprints with two level approaches to existing normal paths. It places a
Merry-Go-Round at world **832,960**, station height **80**, in **Mines of Africa**.
No terrain, ownership or clearance cheats are used. Entrance outside: **864,864**;
exit outside: **736,992**. One native queue tile and one native normal tile connect
to existing park paths. Every committed tile uses production Paths query/action
and asserts its exact cash charge; previews assert no spending.

The route search is test automation only, not a new gameplay feature. It cannot
manufacture path edges or queue ownership. Native construction, station actions,
path actions, ride opening and park opening execute normally. For reproducibility,
the driver sets only the native scenario RNG seed to `0x1234567F, 0x789FABCD`.
It never sets guest destinations, needs, money, state, ride settings, customer
statistics or connectivity. Some unseeded runs naturally failed to attract enough
guests before the time bound; these failures were retained as validation failures,
not passed off as successful captures.

The deterministic run observes native guests queuing, the same peep IDs boarding,
and a boarded guest returning to walking with native hasRidden, plus a native
customer count above zero. The first complete customer appears after 60,000
simulation ticks. Saving/reloading retains real connections, open ride status,
customer data and the exact final simulated cash balance.

Fourteen captures are exactly **512×384**:
- `flat-picker`, `flat-ghost`, `flat-entrance`, `flat-exit`.
- `access-review`, `access-queue-ghost`, `access-exit-ghost`.
- `access-connected` (Dashboard feedback), `access-connected-park` (unobstructed paths).
- `access-opening-confirm` (Cancel-first).
- `access-guests-queue`, `access-guests-riding`, `access-guests-exited`.
- `access-dashboard`.

These are native host scenario renders using production components, not fixture
world captures, generated art, or physical Brick screenshots. Guests/path tiles
are small and can be occluded by native entrance/scenery graphics; the native
state assertions supply the operation proof. Assets, captures and saved parks
remain outside git.

Reproduce using the existing build/data paths:

```sh
python3 tools/handheld/run_native_test.py --build BUILD --data DATA --assets PRIVATE_RCT --ninja NINJA
python3 tools/handheld/run_native_test.py --build BUILD --data DATA --ninja NINJA
python3 tools/handheld/render_ride_access.py --build BUILD --data DATA --assets PRIVATE_RCT \
  --scenario 'PRIVATE_RCT/Scenarios/Africa - African Diamond Mine.SC6' --output CAPTURES --ninja NINJA
```

## Limits and next step

Ground-level access for supported flat rides only; no bridges/tunnels, automatic
routing, coasters or operating settings. Native connectivity feedback does not
perform a complete park-network accessibility audit. Long names use the existing
ellipsis and the Access page uses the existing Dashboard density. Host networking,
scripting, TTF and OpenGL are disabled; ARM/physical Brick and multiplayer tests
are not claimed. Canonical publication fast-forwards from the canonical base only
after validation; the GitHub checkpoint tree must match the tested local source.

A substantial next step into coaster construction is a **complete native saved
track-design placement workflow for one researched wooden-coaster type**: native
available design picker/preview, position/rotation/query, one complete placement,
existing entrance/exit/access recovery and Dashboard test/open/save. Use native
TrackDesignAction and validation, support only designs that can be completed
honestly, and keep custom piece-by-piece track editing for a later milestone.
