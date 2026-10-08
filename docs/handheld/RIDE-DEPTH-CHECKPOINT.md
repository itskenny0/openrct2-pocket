# Ride Details / Operations depth — 2026-10-08

Continues published Pocket `b21dd2daa71779122cbcadd1fa4c322725888abb`.
Read FEATURE-DEPTH-MATRIX.md for the source-backed roadmap. No gameplay system,
HUD, world graphics, save format or physical controller mapping is redesigned.

## Complete flow

Dashboard → Rides → existing inspection → Operations or Maintenance → one
bounded setting draft → native query/review → Cancel-first confirmation → updated
section → inspection/roster/park. Existing inspection, statistics, admission,
status, entrance/exit, Access and Continue track entries keep their prior order.
New depth entries follow them and use existing row paging. Ordinary compact ride
inspection already links into this same Dashboard workflow.

Operations offers only descriptor-relevant settings:

- Operating mode when Ride::getAvailableModes returns multiple alternatives.
- Minimum/maximum wait seconds, bounded by Limits::kMaxWaitingTime, only for
  types with hasLoadOptions. Separate On/Off entries retain every other departure
  flag. Native actions retain the coupled minimum/maximum adjustment behavior.
- Lift speed only when regular or inverted native paint metadata supports the
  liftHill track group. Native LiftData bounds and STR_VELOCITY formatting apply.
- Current native mode and actual trains/cars are read-only information. This
  checkpoint does not pretend to provide vehicle configuration.

Maintenance shows reliability, downtime, last inspection, due-inspection state
and native age in months. Inspection interval uses the complete native interval
names/list, including Never. Rides without native breakdowns show maintenance
Not applicable. Summary adds the native total queue length; existing ratings,
customer and financial statistics remain unchanged.

Drafts revalidate ride identity, build date/object/type/location/generation,
status, price/park money rules and a snapshot of mode, waits, departure flags,
lift speed and inspection interval. The available setting/values are rederived
before query/execute. Unsupported/out-of-bound/unchanged/stale values are rejected.
Native RideSetSettingAction retains mode closure/breakdown restrictions, test
result invalidation, vehicle reset, coupled waits, pause policy and serialization.
Research is not invented as a restriction on managing an already existing ride.

## Demolition

Inspection → Demolish → named destructive review, queried native refund,
Cancel-first → explicit Confirm → native removal → refreshed Rides roster.
RideDemolishAction is the only executor. Its Query checks permission but does not
populate demolition cost, so the review mirrors DemolishRidePrompt.cpp and shop
management using native RideGetRefundPrice (native TrackRemove query accounting).
A changed refund invalidates review. No-money parks show Not applicable; query
cost is zero and native execution owns all accounting. Invalid queries display
Unavailable/native feedback. Indestructibility and actual pause rules remain
native. Existing WindowManager ride-generation invalidation rejects reused IDs.

Dashboard normally owns an automatic pause. Demolition review temporarily
releases **only that owned normal pause**, keeping the park live for native
validation/execution. Leaving review or completing the callback restores the
Dashboard pause. Closing review directly returns to the original running state.
Pre-existing player pauses and other native pause reasons are never cleared.
An already-paused park therefore honestly rejects demolition until the player
returns to the park and unpauses. Ghosts were cleared by existing menu entry;
no construction/access state is injected or persisted.

## Controls and hierarchy

D-pad Up/Down chooses rows; Left/Right adjusts a setting draft. A adjusts the
selected decrease/increase row, enters Review, or explicitly confirms/cancels.
B from review returns to its draft (demolition returns to inspection); B from
setting draft returns to Operations/Maintenance; B from section returns to ride.
Select switches rail/content only outside drafts/reviews. Section page/row resume
uses the existing Dashboard table. Start retains existing pause/menu behavior.
The existing semantic handler admits only one action per transition; busy/request
and lifetime guards protect callbacks, and destructive review defaults to Cancel.

## Changed files

- RideManagement.h/.cpp: native typed setting adapters, bounded drafts, stale
  validation, maintenance/queue information and demolition query/executor.
- Menu.cpp: two section routes, return hierarchy, resume and owned-pause lifecycle.
- test/handheld/RideDepthTests.h and FlatRideWorkflowTests.h: real native flat
  placement/settings/removal/accounting/save fixtures.
- PresentationLifecycleTests.cpp: real SDL management focus/resume/confirmation
  and native wooden metadata setting tests without original world graphics.
- PanelPreview.cpp, tools/handheld/render_shell.py: six production-component
  ride-depth fixture states, each exactly 512×384.
- FEATURE-DEPTH-MATRIX.md and this checkpoint.

## Validation and honest limits

Native Release GUI/CLI builds pass. Existing lightweight CTest passes 3/3.
The full graphics-disabled handheld regression suite passes all prior tools,
pickers, native ghosts, research/progression, finances, staff, guests, shops,
Dashboard, rides, Overview, flat rides and RideAccess/save accounting.
New native ride-depth coverage uses the flat ride built through native actions:
inspection/settings bounds, stale/duplicate rejection, native query equivalence,
zero-cost operations, exact demolition refund, indestructible/pause rejection,
neighbouring path preservation, no-money placement/removal and save/reload.

The real SDL/public-data lifecycle driver passes wooden mode availability/action,
lift bounds, coupled waits, paused/no-money setting behavior, Operations rail
resume, modal focus guards, duplicated selecting input, draft/review cancellation,
confirmed maintenance, Cancel-first demolition, confirmed demolition returning to
a paused roster, pre-existing player pause rejection, stale generations, native
save/reload, HUD yielding/return and shutdown. SDL dummy renderer warnings and
missing PTCT1 sprite warnings are expected: public object metadata exists, the
private original sprites do not. Native action controls operate on real state;
metadata-only fixtures are not described as functioning coasters.

Capture driver compiles against the native GUI. Python tooling and committed
source diff checks pass. Public component captures use production presentation
and native wooden object metadata, with a clearly named trackless fixture;
ratings remain unavailable, queues are zero and its demolition refund is zero.
No park backdrop, guest use or physical Brick validation is claimed.

Private scenario assets remain unavailable. Asset-backed custom-coaster,
saved-design and elevated natural-guest scenario drivers were not rerun. Their
production actions/geometry/ghost code is unchanged, but this is a regression
coverage limitation, not a claimed scenario pass. Physical Brick performance and
readability still need testing. Networking is disabled in this host build; native
GameActions/serialization are reused but multiplayer was not tested.

## Deferred depth / next

Vehicle/train/car configuration; load-percentage and synchronization rules;
mode-specific launch/time/laps/swings/rotations/circuits; music/appearance/naming;
refurbishment/locate mechanic; richer customer/finance pages; ride graphs. Graphs
have four native RideMeasurement traces and scrolling/scale interactions; defer
until a readable controller-native design warrants their cost.

Next recommended package: **Rides → Vehicles / Dispatch → bounded native
configuration → review/confirm → native Test/Open → save/reload**, using
RideSetVehicleAction and remaining native departure flags. Reuse these typed
setting drafts rather than building another shell. Do not automatically begin it.

## Workspace/publication safety

Original checkout and unrelated Esperanto edit remain untouched. Implementation
uses the existing openrct2-pocket-hud working copy and incremental safety commits.
Its local ancestor differs from published history as documented in the HUD
checkpoint; publish an identical validated source tree with canonical b21dd2d as
parent using a guarded non-force branch update. Exclude the Esperanto edit.
