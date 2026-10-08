# Ride Management completion — incremental checkpoint

Continues published 02e40f70382555577799890d5541a631c27a13ca. The existing
local starting aab02e0 has the same source tree; both histories are preserved.
The unrelated Esperanto edit remains untouched and excluded from commits.
This document tracks delivered phases honestly, not a claim of desktop parity.

## Phase 1: Vehicles — complete within the implemented configuration interface

Vehicles now offers loaded, researched alternative vehicle objects using native
GetAllRideEntries and RideEntryIsInvented. Native existing cross-track/research
cheat policy is mirrored without enabling cheats. Flat/maze/mini-golf exclusions
match desktop Ride.cpp and RideSetVehicleAction. No objects are loaded by Pocket.
A single available object remains read-only rather than an empty choice.
Reversed trains follows allowReversedTrains and noReverseOption, including the
native already-enabled train-length cheat visibility policy.

The setting/draft value path is widened to uint16_t for native ObjectEntryIndex;
no truncation to a byte. Object switching previews native updateMaxVehicles with
native proposed-car clamping on a detached calculation ride. Review shows named
objects and effective counts. RideSetVehicleAction owns closure/breakdown/pause,
vehicle resets, presets, test invalidation, cash and serialization. Both use the
existing bounded draft, native query, Cancel-first confirmation and held-input
architecture. Reversal is in the baseline snapshot. Research/loaded-object choices
are rederived at confirmation. Only a successful owned object-change callback
refreshes inspector identity; external object changes still invalidate targets.

Controls are unchanged: Up/Down rows; A enters; Left/Right drafts; A Review;
Cancel first; B returns through hierarchy; Select rail switching outside modals.
Native vehicle-object selection uses named bounded choices, not a new window.
The pre-existing Vehicles page requires a station; trackless unfinished rides
continue to show unavailable configuration rather than claiming an effective train.

Tests: Release GUI/CLI build; full public-data native handheld driver exercises
research disappearance, loaded alternatives, query/cancel, exact zero cash,
native object switch/revert, stale identity/repeat rejection, reverse while
paused, native limits and native .park persistence. Existing SDL lifecycle and
lightweight CTest protect confirmation, held input, routing and unrelated tools.
Private scenario assets remain unavailable; real complete coaster operation and
physical Brick readability are not claimed by metadata/component fixtures.

## Remaining phases / parity

| Feature | State | Native references / dependency |
| --- | --- | --- |
| Appearance, track/vehicle schemes and entrance styles | Missing | Ride.cpp colour page; RideSetAppearanceAction / RideSetColourSchemeAction |
| Music enable/style | Missing | Ride.cpp music page; MusicObject::SupportsRideType; RideSetSettingAction |
| Ride/shop rename and reusable keyboard | Missing | RideSetNameAction; native UTF-8 constraints; reusable controller text entry |
| Refurbishment and mechanic locate | Missing | RideDemolishAction renew; RideGetAssignedMechanic / RideFindClosestMechanic |
| Existing maintenance interval/information | Complete in prior scope | RIDE-DEPTH-CHECKPOINT.md |
| Customer/financial information depth | Partial | Ride totals, rolling data, queue/stations, undefined-value guards |
| Ride graphs | Missing | RideMeasurement samples and native desktop graph conversions |
| Track colour scheme painting / random vehicle colours / other small parity | Missing | Native actions, map targeting, per-car/train scheme semantics |
| No-vehicle maximum-rider Operations fallback | Missing | Desktop operating mode fallback, RideSetSettingAction |

No naming/appearance/graph/maintenance action is implied working by this Phase 1
checkpoint. Follow subsequent phase entries in this document for current state.
