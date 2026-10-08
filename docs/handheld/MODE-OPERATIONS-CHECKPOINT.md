# Mode-specific Operations — 2026-10-08

Continues published 1970064d0f31bab5b23d8e320d2dd718e9c643cb. Local starting
9f77977dd2e03b7b2e7ed9a01384494238720700 has the same source tree. Incremental
local commits preserve work. Publication extends the canonical remote parent
with identical source-tree verification and a non-force guarded update. The
pre-existing Esperanto edit is preserved and excluded.

## Complete workflow

Dashboard → Rides → inspect → Operations → applicable named native parameter →
bounded draft → native queried review → Cancel-first confirmation → Operations →
existing ride Test/Open review → park/save/reload. No new route, shell, physical
mapping, simulation rule, native action or save format is introduced.

RideManagement::Settings mirrors desktop Ride.cpp operatingOnPrepareDrawMode:

| Current native mode | Pocket parameter / native formatting |
| --- | --- |
| Dodgems | Time limit, STR_RIDE_TIME_ENTRY and descriptor multiplier; native ten-unit spinner step |
| Race | Number of laps, native integer value |
| Swing | Number of swings, native descriptor multiplier |
| Rotation, forward rotation, backward rotation | Number of rotations, native descriptor multiplier |
| Powered launch passthrough, powered launch, upward launch, powered launch block-sectioned | Launch speed, native raw * 9 / 4 and STR_VELOCITY |
| Station to station | Speed, same native velocity conversion/formatting |
| Native canHaveMultipleCircuits | Circuit count 1–Limits::kMaxCircuitsPerRide, relevant modes/trains/stations only |

The actual loaded ride descriptor supplies operating min/max. Already active
unlockOperatingLimits uses native cheat bounds, never enabling cheats. The native
current mode must appear in getAvailableModes. Missing objects and singleton
operating ranges are not exposed as working adjustments. Cable-lift rides are
not offered multiple circuits; native queries still own validity. Ordinary
continuous-circuit/block rides do not receive an invented launch/laps control.
The desktop no-vehicle maximum-riders fallback is intentionally deferred.

These remain semantic typed adapters over RideSetSetting::operation and
numCircuits. No raw operationOption control is displayed. Existing mode/wait/lift,
Vehicles/Dispatch, Maintenance and demolition remain unchanged.

## Safety and controls

The existing shared setting draft gains a default step of one (Dodgems ten).
Values remain a full legal native interval, preserving non-round values from
existing saves while bounding increments to each endpoint. Current/draft labels
use the correct native units. Native FmtString::WithoutFormatTokens removes
desktop colour/font tokens from these new formatted values, and caption colons
are trimmed, so existing Pocket typography/focus remains in control. Native Action rederives the setting, availability
and legal values before query/execution. Snapshot additionally records the native
operation byte, rejecting changed settings even when identity/mode are unchanged.
Mode/status/vehicle/count/money and reset-generation guards remain in force.

D-pad Up/Down selects, Left/Right adjusts. A selects/adjusts/reviews or explicitly
confirms/cancels. B review → draft → Operations → ride/roster/rail/park. Select
pane switching stays blocked in drafts/reviews. Start retains existing pause/menu.
Cancel-first and one-transition-per-frame, held-input suspension and callback
lifetime guards are reused unchanged.

RideSetSettingAction retains native accounting, test-result invalidation,
serialization and its AllowWhilePaused flag. Native operation/circuit changes do
not acquire an invented closure requirement. Operating-mode changes still require
closed/simulating status and respect breakdown restrictions. Native Test/Open
remains authoritative for track, stations, entrances/exits and connectivity.

## Validation

Native Release GUI/CLI builds pass. Full public-data native handheld regressions,
real SDL/public-data lifecycle and lightweight CTest (3/3) pass. Source diff and
Python capture tooling checks pass.

ModeOperationsTests uses native RideCreate/SetSetting/SetStatus/Demolish actions
and public object metadata for Merry-go-round, Dodgems, Swinging Ship, Ferris
Wheel (both rotation directions), Go Karts, Chairlift and Corkscrew Coaster.
It checks type/mode labels and units, bounded drafts/steps, exact native query
results, paused/no-money behavior, zero cash/ledger changes, cancellation,
invalid/repeated/stale settings, disappearing laps after a native mode change,
circuit/launch availability and incomplete-track Test/Open rejection.
The genuinely placed connected Merry-go-round changes rotations, then closes/
reopens natively and persists its setting through native save/reload. Trackless
metadata fixtures are removed through native actions before saving; they are
not fabricated operating rides. VehicleDispatchTests also persists changed
Dodgems duration and wooden-station circuits alongside existing vehicle/flags.

SDL lifecycle tests navigate the actual Operations row, use a mapped virtual
controller to hold selecting A, cancel review, confirm once and return to the
correct section without cash mutation. All prior mode/lift/wait, Dispatch,
Maintenance, demolition, HUD and lifecycle assertions remain intact.

Private original RCT graphics/scenarios remain unavailable. No completed live
coaster/Chairlift/Go Karts guest operation is claimed. Their parameter tests use
native metadata/actions and native prerequisite failures honestly. Asset-backed
custom-coaster, saved-design and elevated-guest scenario drivers cannot be rerun;
production construction/access/ghost code is unchanged. Physical Brick usability
and networking remain unverified in this native host build.

Captures use production 512×384 Dashboard/draft/confirmation rendering, native
created ride metadata and real queried values, with no world backdrop or invented
ratings/guests. They are labelled component fixtures, not scenario captures.

## Files and next depth

RideManagement.cpp/.h: mode adapters, step and operation-state snapshot.
ModeOperationsTests.h, RideDepthTests.h, VehicleDispatchTests.h and
PresentationLifecycleTests.cpp: native/controller/save integration.
PanelPreview.cpp/render_shell.py: component capture states.
FEATURE-DEPTH-MATRIX.md and this checkpoint.

Next: a focused native Maintenance/refurbishment/mechanic-assistance workflow.
No naming, graphs, new ride types/categories, patrol editing, UI redesign or
operating construction changes in this milestone.
