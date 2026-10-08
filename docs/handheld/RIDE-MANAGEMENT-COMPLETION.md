# Ride Management completion — delivered checkpoint

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

## Delivered milestone / parity

| Feature | State | Native references / dependency |
| --- | --- | --- |
| Appearance, track/vehicle schemes and entrance styles | Complete in Phase 4 (painting/randomization below) | Ride.cpp colour page; RideSetAppearanceAction / RideSetColourSchemeAction |
| Music enable/style | Complete in Phase 2 | Ride.cpp music page; MusicObject::SupportsRideType; RideSetSettingAction |
| Ride/shop rename and reusable keyboard | Complete in Phase 5 | RideSetNameAction; native UTF-8 constraints; reusable controller text entry |
| Refurbishment and mechanic locate | Complete in Phase 3 | RideDemolishAction renew; RideGetAssignedMechanic / RideFindClosestMechanic |
| Existing maintenance interval/information | Complete in prior scope | RIDE-DEPTH-CHECKPOINT.md |
| Customer/financial information depth | Complete in Phases 6 and 8 | Ride totals, rolling data, queue/stations, undefined-value guards |
| Ride graphs | Complete in Phase 7 | RideMeasurement samples and native desktop graph conversions |
| Track colour scheme painting / random vehicle colours | Complete in Phase 8 | Native actions, map targeting, per-car/train scheme semantics |
| No-vehicle maximum-rider Operations fallback | Complete in Phase 2 | Desktop operating mode fallback, RideSetSettingAction |

The eight phases below are delivered within their stated scope. This milestone
does not imply unrestricted desktop construction or hardware validation.

## Phase 2: Music and no-vehicle Operations

Music has its own inspector section using the shared bounded draft, native query,
Cancel-first confirmation, busy and held-input architecture. Enable/disable uses
RideSetSettingAction; style choices use loaded MusicObject entries and native
SupportsRideType with explicit-listing and existing unlockOperatingLimits policy.
Unavailable custom WAV styles are omitted as on desktop. Pocket loads no music
objects and changes no cheats. The operation snapshot now includes style and
playback, rejecting external changes and repeated drafts. Object availability is
rederived on review. No audio playback is claimed by metadata-only fixtures.

No-vehicle operating modes now use the native maximum-people caption and native
OperatingSettings bounds for the desktop default case (excluding shopStall).
Existing mode-specific parameters and circuit behavior are unchanged.

Validation: Release GUI/CLI build; full public-data native driver (toggle, loaded
style selection, query cancellation, repeated/invalid drafts, zero cash and
native park serialization); SDL lifecycle driver (Music route, rail and resume,
existing Dispatch/Vehicles/Operations/modal regressions). Target hardware music
playback and complete coaster assets remain follow-up validation.

## Phase 3: Maintenance actions

Maintenance now offers mechanic locate and refurbishment for types with native
breakdowns. Locate checks RideGetAssignedMechanic then RideFindClosestMechanic
for inspection and pans to the live entity; no assignment or teleport is made.
Unavailable mechanics report a visible message.

Refurbishment uses RideDemolishAction renew, native Query price and native
closed/empty/ever-opened/need/funds/pause policy. Native execution owns age,
reliability, downtime and cash. Review is Cancel-first and temporarily releases
only Dashboard's owned pause, restoring it on cancellation/return. It compares
fresh cost and the existing ride identity, status and money snapshot; repeated
renewal is rejected by native ever-opened policy. Successful owned renewal
refreshes the identity build date and returns to Maintenance. Ride, track,
entrances, vehicle configuration and music remain under native rules.

Validation includes native query/cancel, exact renewal cost, zero age, retained
name/music, repeat rejection and original demolition regression; real SDL review
handoff and Cancel-first pause/parent restoration. Complete operating scenarios
and physical Brick input/readability are still not claimed by public fixtures.

## Phase 4: Appearance configuration

Appearance exposes native supported main/additional/support colours for each
supported track scheme; maze styles; loaded entrance objects; permitted vehicle
body/trim/tertiary channels and same/per-train/per-car schemes. Descriptor,
RideEntry and CarEntry checks mirror desktop. Invisible cars are skipped but
native car indices are retained. Each setting uses the bounded typed draft and
RideSetAppearanceAction with its native index. A full appearance snapshot rejects
external colour changes; object availability and legal channels are rederived.
Native execution updates vehicle sprites and scheme resets, never direct mutation.
The existing native dropdown palette/name ordering is reused by a read-only name
accessor. Track painting and randomizing vehicle colours are delivered in Phase 8;
changing a secondary scheme alone does not assign track pieces to it.

Validation: native wooden station fixture exercises indexed colour channels,
query/cancel, repeat/invalid rejection, zero cost and native park serialization.
512x384 production-component appearance capture and SDL section route/back checks.
No original world sprites or complete operating coaster are fabricated.

## Phase 5: Ride and shop naming

A reusable eight-column controller keyboard offers upper/lowercase, digits,
common punctuation, space, UTF-8-safe erase, clear, review and cancel. Limit is
32 characters, matching desktop ride name entry. Existing Unicode names are
retained; new input is the labelled ASCII alphabet. D-pad selects keys, A types,
B cancels, Review enters the shared Cancel-first native name review. Rail switching
is blocked inside the keyboard. Empty input explicitly means reset to default.

RideSetNameAction owns uniqueness, native default names, cash and serialization.
The retained target includes a shop marker so shops share generation/reset/object
identity protection without appearing in the non-shop roster. External name
changes and repeated drafts are rejected. Successful review returns to the ride
or shop inspector; review cancel returns to the keyboard with typed text retained.

Validation: native ride and shop rename, duplicate shop rejection, zero cash,
Unicode persistence; keyboard code-point limit/erase/case/bounds; real SDL
keyboard route, duplicate-A boundary, modal rail guard, Cancel-first review and
B return. Production-component 512x384 keyboard capture.

## Phase 6: Information depth

Statistics now links Customers and Finances. Customers reports native lifetime
and hourly counts, riders now, total queue, maximum wait, popularity and
satisfaction using desktop scaling and undefined sentinels. Finances reports
admission, income, native upkeep times 16 per hour, hourly profit and total profit,
with no-money/undefined guards. These read-only pages return to Statistics and
retain rail/resume behavior. Native scenario data is displayed without estimating
missing values or inventing customers. Graphs and per-station drilldown are delivered below.


## Phase 7: Native ride graphs

Statistics opens a bounded graph page for speed, altitude and supported vertical/
lateral G force. Entering the page requests native Ride::getMeasurement; the native
engine owns sampling, eligibility and measurement eviction. Missing, waiting and
unsupported measurements remain visible states. Each window shows at most 240
native samples; Left/Right scrolls 120 samples with bounds derived from native
num_items. The displayed time range uses native 16 samples per second. Speed,
altitude and G values use desktop conversion and configured native unit strings.
Metric selection and Back return through Statistics without changing ride settings.

Validation: native registration/waiting and real SDL routing, bounded scrolling
and conversion assertions. The 512x384 graph capture uses an explicitly labelled
synthetic measurement fixture for rendering and conversion only. It does not prove
live sampling of a fully operating coaster with original vehicle/world assets.

## Phase 8: Painting, random colours and station detail

Appearance offers a scoped track colour tool for rides supporting multiple schemes.
It retains the inspected ride identity and targets an actual non-ghost track piece
at the viewport centre. Y cycles four native schemes, A paints one native piece,
B exits; camera/zoom controls retain the existing Explorer behavior. Native
RideSetColourSchemeAction owns validation, track mutation and serialization. Target
identity, ride, direction, sequence, track type and baseline scheme are rechecked;
no-op, stale and other-ride targets are rejected. There is no brush or whole-ride
painting claim.

Per-train/per-car vehicle schemes offer random colours using native permitted
palette entries and CarEntry channel support. Proposals are fixed before review,
shown individually and Cancel-first. All proposals are queried before the native
appearance actions are queued, using desktop's per-channel execution semantics.
Fresh appearance/operation snapshots and legal channels are revalidated. Normal
and existing special-colour cheat palettes are respected without enabling cheats.
Customers also opens a read-only per-station queue/wait and entrance/exit summary.

Validation: native painting and random-colour query/cancel/execute, repeat and
wrong-ride rejection, exact zero cash, native save/reload; shared SDL lifecycle,
shop keyboard focus/typing/cancel, station/graph routes and held-input protection.
Production-component 512x384 graph, station, painting and keyboard captures were
inspected. Release GUI/CLI build, full native driver, SDL driver and lightweight
CTest (3/3) passed. Physical Brick readability/input, live complete-coaster graphs
and target audio playback remain validation follow-ups, not claimed fixture results.
