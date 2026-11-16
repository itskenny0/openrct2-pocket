# Custom wooden coaster checkpoint

Continues `handheld/modern-ui-paths` from
`30452f4b42200e8daaae23eface35f41bd156dc6`.

## Playable workflow

Build → Rides → loaded/researched wooden coaster object → Custom build → station
→ sequential track → native closed circuit → existing entrance/exit placement
→ optional RideAccess / elevated Paths → Dashboard Rides Test/Open → native guests.
The same choice page retains access to the completed saved-design workflow.

The first A creates one station segment, not a prebuilt ride. Repeated station
segments use native station conversion. Each subsequent A builds one native
track block at the forward endpoint and charges its queried price once. A
multi-tile turn remains one native block/action. Nothing automatically closes
or connects the coaster. Native shape, clearance and connection validation apply.

Select opens the shared three-column/six-item paged picker. It filters against
loaded vehicle support, ride track groups, endpoint pitch, banking and diagonal
orientation. B cancels the picker without selecting/placing. Its final Return
to park entry safely leaves construction. Reopen ordinary ride inspection or
Dashboard Rides and choose Continue track... to reconstruct an unfinished course
from its real station/track elements. No Pocket construction state is serialized.
Closed completed circuits use the existing entrance/exit and Access recovery
entries. Opening/testing is still explicit and Cancel-first in Dashboard.

## Controls

| Control | Station placement | Sequential construction |
| --- | --- | --- |
| D-pad | Move native view/target | Pan native view; endpoint follows real track |
| A | Place one station block | Place one selected native block |
| B | Cancel empty construction | Remove preceding native block/refund |
| X | Bank selection where compatible | Cycle compatible outgoing bank |
| Y | Rotate station | Toggle native-supported chain lift |
| L1 / R1 | Compatible piece selection | Previous / next compatible piece |
| L2 / R2 | Lower / raise station target height | Previous / next compatible outgoing slope |
| Select | Track picker / Return to park | Track picker / Return to park |
| L3 | Toggle camera context | Toggle camera context |
| L1 / R1 in camera context | Native zoom | Native zoom |
| X in camera context | Native camera rotation | Native camera rotation |
| Start | Existing Dashboard/menu | Existing Dashboard/menu |

The contextual camera mode preserves camera rotation/zoom without assigning a
pointer or conflicting with piece, bank and slope controls. Trigger presses are
edge-triggered; held inputs must release after modal transitions. Existing modes
retain their original mappings and ignore the new slope/camera semantic actions.

## Native architecture

- `CustomTracks` owns transient selection, source block coordinates/type,
  target identity and endpoint. `RideGetStationStartTrackElement`,
  `trackBlockGetNext`, `findTrackGap`, `TrackIsConnectedByShape` and native TED
  pitch/roll/clearance tables determine recovery, compatibility and completion.
  No hard-coded track route, scenario site or Pocket geometry table exists.
- Native `RideCreateAction` creates the ride record. Cancelling an editor-created
  record with no real track uses native empty-ride cleanup, not a demolition UI.
- Registered/serializable `SequentialTrackPlaceAction` guards native ride/object,
  build date, closed state, research, source block, unchanged endpoint,
  pitch/bank, vehicle support, money mode and exact quote. It delegates Query/
  Execute to native `TrackPlaceAction` or `TrackRemoveAction`. Native action
  runner owns funds, pause, execution, accounting and refunds. Prospective
  connections are checked through native endpoint lookup and TED shape metadata.
- Shared `TrackPreview` now accepts a native track type and lift flags. It uses
  a borrowed unused native ride record, native ghost/no-spend actions and native
  removal. The real coaster's station data/measurements are not used as scratch
  preview storage. Existing flat-ride callers keep their shared preview path.
- Native provisional removal clears custom ghosts before simulation/save;
  nested suspension delays revalidation until fully restored. Invalid placement,
  object/research loss, picker/Dashboard, focus loss, park changes and leaving
  construction clear previews. Successful placement/undo refreshes the native
  source, identity and viewport focus. Stale identities cannot resume tools.
- `WindowManager::Cleanup` ends handheld tools before objects/context unload,
  fixing active-ghost shutdown ordering. No simulation or save format changes.
- Existing `FlatRides` station entrance/exit recovery and `RideAccess` optional
  handoff are reused unchanged. Native Dashboard enumeration finds the ride.

## Supported vocabulary

Only loaded/researched Wooden Roller Coaster entries are offered for Custom build.
Supported native families, when the loaded vehicle and current endpoint permit:

- straight/cardinal and compatible diagonal pieces;
- station extension;
- moderate/steep ascending/descending slopes and transitions, including long bases;
- left/right turns, small/large curves, compatible sloped/banked curves and S-bends;
- straight/slope banking and bank transitions;
- native-supported chain lift;
- brakes and block brakes, including compatible diagonal variants.

Deferred: inversions, helixes, splash/on-ride-photo/boosters and unusual special
pieces needing another interaction model; backward endpoint selection, editing a
completed closed circuit, operating settings and brake-speed editing. Native default
brake speed is used. Other custom ride types are excluded. No auto-close or route
builder exists in production.

## Validation

Native Release GUI and CLI build successfully. Refreshed lightweight CTest:
3/3 input/launcher/save-preservation tests pass, including new virtual SDL trigger
edges, held-trigger handoff and L3 camera action. Both asset-backed and graphics-
disabled complete native handheld regression drivers pass. Saved-design and
original elevated-access real-scenario drivers pass independently. Python compile
and `git diff --check` pass.

`tools/handheld/render_custom_coaster.py` links the production GUI renderer and
`test/handheld/CustomCoasterCapture.cpp`. It uses the private Mines of Africa
scenario and a native permitted loan; no assets, park files or designs enter git.
The integration driver covers:

- actual Build/picker/Custom entry and loaded/research filtering;
- native station ghost, target height/orientation and station extension;
- native action factory/serialization and changed quote/money/research rejection;
- native funds/pause restrictions and no-money placement/undo without cash changes;
- unchanged/invalid ghosts, nested provisional handling and random/real identity;
- actual Dashboard transition and native SDL window focus loss;
- semantic straight/turn/slope/bank/chain selection and construction;
- shared picker paging, selection/cancel and mapped held-A handoff;
- duplicate-A frames, exact each-block costs, two-block and multi-tile-turn undo/refunds and rebuilding;
- unfinished save/reload with ghost active, no persisted ghost and inspection resume;
- a 54-block closed circuit constructed by semantic controller frames;
- native entrance/exit placement, elevated sloped queue/normal paths, exact costs,
  cancellation/later inspection resume, native queue association and park connectivity;
- Dashboard roster, Cancel-first Test/Open, cancellation and native status actions;
- natural guest queue → boarding/onRide → travelling car → walking/hasRidden exit;
- completed open ride/connectivity/cash save/reload, stale access, removed/reused
  ride identity, park changes and context shutdown with an active native track ghost.

Restriction checks temporarily alter cash/no-money/research/pause as explicitly
labelled reversible test fixtures and restore them before the playable captures.
Actual scenario construction, loan, tracks, entrances, paths and status changes use
native actions. Site/route search exists only in the test harness; it selects
native-query-valid owned terrain and uses native metadata. Guests are not
teleported, assigned destinations, mutated or forced onto the coaster.

Reproduce (supply local build/data/private assets/scenario and output paths):

```sh
python3 tools/handheld/render_custom_coaster.py --build BUILD --data ENGINE_DATA \
  --assets PRIVATE_RCT --scenario SCENARIO --output OUTPUT --ninja NINJA
```

## Captures and limits

All captures are exactly 512×384, from the real scenario and native viewport.
The set includes Custom choice, station ghost, sequential track, chain/slope,
banked turn, paged track picker, closed coaster, entrance/exit, connected elevated
paths/access review, Dashboard roster/Test/Open and genuine queue/ride/exit.
Private scenario/save/assets remain outside git. Native financial-effect text can
appear immediately after actions; it is ordinary world rendering, not a Pocket
status overlay.

Solid shared palette is unchanged: background 118, border/separator 119,
selection 53, primary text 21 and muted text 18. No dithering. The 164-pixel
construction panel leaves the park dominant. Picker tile captions may truncate;
its detail line shows the selected piece name. Track pieces lacking native object
thumbnails use compact footprints derived from native TED sequence data and
existing palette-safe path icons, not new artwork. Ordinary pieces lacking native
localized description strings have concise English metadata-derived labels.

Ghosts need an unused native ride record. Native full-capacity construction limits
remain; no extra object/ride capacity is invented. Recovery follows the forward
station course; arbitrary disconnected/hacked layouts and rare pieces are outside
this first editor. Station height controls are bounded to ground plus 256 world
units and all heights still require native validation. Physical Brick/ARM runtime
and performance remain to be tested; these are native host builds/captures.

## Next milestone

Pocket-wide persistent top HUD plus a cohesive 512×384 UX/visual pass. Reuse native
park date/cash/guests/rating/pause state and shared formatting. Preserve these
completed construction and management workflows; do not add a gameplay subsystem.
