# Existing ride management checkpoint

Continues GitHub checkpoint `8eb729a70cd2e5c1e252b624aaa73e17742731c5` on
`handheld/modern-ui-paths`. Rides is now enabled in the existing Dashboard shell.
Only existing non-shop rides are managed; construction, shop/facility management,
all tools/pickers/ghosts and all prior management workflows are preserved.

## Complete player flow

Start → Dashboard → Rides → full native paged roster → inspection → statistics,
Locate, Open/Close/Test review, or admission-price draft/review → explicitly
Confirm or Cancel → return through inspection/roster/rail or to park → save/reload.

D-pad navigates, A selects, B backs out one level, Select switches rail/content
with existing per-section page/row resume, and Start retains the pause/build/save
menu transition. Drafts and confirmations cannot switch panes with Select.
No physical mapping, camera control or fake mouse behavior is added. Selecting a
ride does not also Locate or execute an action. Confirmations start on Cancel.

The roster includes every native non-shop ride record, including unfinished
rides, without an artificial limit. Shared pagination shows seven roster rows
per page at 512×384. Roster updates preserve a still-present selected identity;
returning from inspection restores its roster row. Names/status previews use the
existing ellipsis behavior. Inspection has two native status/admission values
and seven actions, with the final Back row on a second shared page.

Statistics show native excitement/intensity/nausea when tested and rated,
reliability percentage, lifetime customers, hourly income and hourly profit.
Untested/unrated or undefined financial statistics explicitly say Unavailable.
No-money financial information says Not applicable. Values use the existing
shared native currency formatter; no simulation values are invented.

## Native actions and validation

`RideManagement.cpp/.h` owns enumeration, identity resolution, native information,
price permissions, bounded drafts, native action adapters and presentation.
`Menu.cpp` owns pages, roster/target/draft state, focus and callback lifetime.

- Status changes call `RideSetStatusAction` Query/Execute. Open/test station,
  entrance/exit, track, vehicle and other prerequisites remain native. Unsupported
  testing is not selectable as a working action. Simulating is not offered.
- Requests for the current status are rejected to avoid repeated native Close
  clearing a ride for construction. There is no invented immediate status charge;
  native recurring operating costs remain simulation-controlled.
- Admission editing mirrors desktop permissions: a loaded primary-admission ride
  entry, native unlocked ride pricing, and a money park. Product/photo/common-shop
  prices remain in the completed shop workflow. `RideSetPriceAction` owns execution.
- Native 0–20 GBP limits bound the draft; D-pad Left/Right or Decrease/Increase
  adjusts by one native monetary unit (0.10 GBP). A reviews; B from review restores
  the draft, while explicit Cancel discards it. Native Query/Execute charge no
  immediate cash for changing admission; guests subsequently pay native prices.
- Drafts validate ride identity, baseline status/price, native ride-pricing
  availability and no-money settings before querying/executing. Native actions
  revalidate their own prerequisites. Invalid/stale reviews remain unapplied.
- Queued confirmation has a busy guard and a weak window lifetime/request token.
  Completion returns to inspection; destroyed/superseded menus cannot receive an
  old callback. Existing semantic suspension consumes one action per frame and
  requires a fresh button press across transitions.

Targets include ID, type, loaded-object index, build date, overall-view location
and removal generation. The existing ride-window close/removal notification in
`WindowManager.cpp` invalidates generations alongside Marketing's existing hook,
including identical reused IDs. Changed identities and missing rides fail closed.
Loading tears down the menu through the existing lifecycle; no menu/draft state
is written to a park file. Native saves omit trackless construction records as
before; the test's save-only fixture has a track element rather than changing
that rule.

Locate resolves again and uses the ride's native overall-view tile centre and
terrain height with `WindowScrollToLocation`, leaving the park unobstructed.
Missing/invalid coordinates report Location unavailable. It does not start follow,
change zoom/rotation, create a desktop Ride window or mutate the ride.

Two native Ride.cpp UI error helpers now skip their automatic desktop
construction/track-error handoff while a handheld menu exists. Without this guard,
a failed status query could run `RideModify`, which can queue a repeated Close and
start desktop construction. Native validation and returned errors remain intact;
desktop behavior without a handheld menu is unchanged. There are no simulation
or action-rule changes.

## Presentation and scope

The established Dashboard rail/content pane, compact hierarchy and thin strip
are reused unchanged. Solid palette entries remain panel 118, border/separator
119, selection 53, primary 21 and secondary 18. No dithering or redesign.

No ride placement, construction, demolition, naming, HUD, other management
sections or new native object loading is added. Overview remains unavailable.

## Verification

Native GUI and CLI build successfully. All three CTest suites pass. Full
UI-linked integration with private RCT assets and engine-only integration pass,
including every earlier tool/ghost/shop/management suite. Python capture-script
compilation and `git diff --check` pass.

New coverage includes:

- 20-record full roster, later-page selection, shop exclusion and unavailable
  native statistics; correct rating/currency scales;
- locate validity, native main-viewport locating, rail/content section resume,
  roster return focus, B hierarchy and pause ownership;
- actual mapped SDL held-A/release/fresh-A selection/Locate handoff and duplicate
  semantic input at draft/review/confirmation transitions;
- native open/test prerequisite rejection, unsupported testing, successful native
  close, no-money status behavior and prevention of repeated Close;
- native admission permissions, bounded prices, queried zero action cost, exact
  unchanged cash/loan/expenditure ledger, cancellation and one confirmed change;
- stale status/price/park settings, missing targets and identical reused ride IDs;
- a missing-entrance query while management is active cannot move the viewport,
  start desktop construction or mutate ride status;
- native price/status/cash/ledger save/reload and menu teardown.

`test/handheld/RidesCapture.cpp` additionally loads the private Mines of Africa
scenario and uses real native rides, sprites/world rendering and production
Dashboard presentation. Captures are exactly 512×384: roster, inspection,
statistics and Cancel-first status confirmation. No fixture rides or fabricated
statistics are shown. Scenario admission prices are locked, so inspection
honestly displays Price unavailable; a fake price editor is not captured.
Permitted price editing is covered by the integration fixtures above.

After captures, the same driver successfully opens, closes, tests and closes all
five built native scenario rides through the adapter. Cash and expenditure ledger
stay exact, then a native save/reload preserves their closed statuses and roster.
Private assets, generated captures and saved parks remain outside git.

Reproduce with the established `tools/handheld/run_native_test.py` and
`tools/handheld/render_rides.py` (same arguments as other native capture drivers).
Host build uses disabled networking/scripting/TTF/OpenGL; physical Brick/ARM and
multiplayer testing are still outstanding. Long names are ellipsised, stats are
split from inspection, and admission remains unavailable under scenario rules.

## Files

- `src/openrct2-ui/handheld/RideManagement.cpp/.h`: section adapter/presentation.
- `src/openrct2-ui/handheld/Menu.cpp`: Dashboard routing/state/input/callbacks.
- `src/openrct2-ui/handheld/Dashboard.h`: enable Rides category.
- `src/openrct2-ui/handheld/Explorer.h`: section presentation/introspection entry points.
- `src/openrct2-ui/WindowManager.cpp`: ride identity invalidation hook.
- `src/openrct2/ride/Ride.cpp`: handheld-only native error UI handoff guard.
- `test/handheld/PathActionsTests.cpp`: integration coverage; unavailable-rail
  expectations now use Overview rather than completed Rides.
- `test/handheld/RidesCapture.cpp`, `tools/handheld/render_rides.py`: real scenario
  captures and native status/save integration.
- This checkpoint document.

## Recommended next milestone

Complete native Park Overview and park open/close operations in the existing
Dashboard shell: honest park/objective summary → native open/close review →
Cancel-first confirmation → park → save/reload. Link permitted admission editing
to the existing Finances workflow rather than duplicating pricing. Preserve all
completed sections/tools, and leave naming, construction expansion and HUD for
later milestones.
