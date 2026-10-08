# Vehicles / Dispatch — 2026-10-08

Continues published d2a49501bf7af65b6923d124f0a481be2440583b.
The existing local starting commit cc5519c3277048473037c02ae5ebc3cdbea8c335
has the same source tree as that published checkpoint. Local safety history is
preserved; publication extends the canonical remote parent without force-push.
The pre-existing data/language/eo-ZZ.txt edit is untouched and excluded.

## Workflow and native authority

Dashboard → Rides → inspect → Vehicles or Dispatch → bounded draft → native
queried review → Cancel-first confirmation → section → existing native Test/Open
review → park/save/reload. No new physical controller mappings or navigation shell.

Vehicles mirrors desktop Ride.cpp visibility, omitting fixed/single-value counts,
no-vehicle types, unavailable objects and rides without a station. Train bounds
come from native Ride::updateMaxVehicles; cars use loaded entry min/max (including
native zero-car display convention). The native train-length cheat, if already
active, supplies native Limits bounds; Pocket never enables it.

A detached Ride containing only calculation inputs (type/object/mode/id, native
stations, block/count/request/limit fields) calls updateMaxVehicles. It does not
copy owned measurement storage, use a parallel geometry algorithm or mutate the
live ride. Native window invalidation may request a cosmetic redraw. Review shows
requested value and effective trains/cars. Station length/mass, block count and
mode can clamp requested counts; confirmation does not promise the request will
be the effective result. Execution uses RideSetVehicleAction, preserving closure,
breakdown, pause policy, vehicle resets, object rules and native serialization.
Counts do not select/load vehicle objects or bypass research.

Dispatch mirrors desktop descriptor checks:
- Wait for load and all five native load thresholds for hasLoadOptions.
- Leave when another arrives only for supported types with multiple actual trains
  outside block-sectioned modes.
- Synchronise adjacent stations only for supporting descriptors.
Each choice preserves all other departure bits and uses RideSetSettingAction.
Existing minimum/maximum waits and their toggles remain in Operations unchanged.

Both share the completed typed draft architecture. Confirmation rederives the
setting and legal values, checks identity/status/money rules and a snapshot of
operation and vehicle state. Vehicle reviews also recheck the native effective
counts. Unsupported, out-of-range, unchanged and stale drafts are rejected.
Native errors remain visible. WindowInitAll's no-exceptions UI reset invalidates
all retained ride identities, preventing pre-load drafts matching an unchanged
reloaded ride. Native ride removal still invalidates individual IDs.

## Controls

D-pad Up/Down chooses rows. Left/Right adjusts the bounded draft. A enters a
setting, adjusts the selected decrease/increase row, reviews or explicitly
confirms/cancels. B review → draft → section → ride → roster → rail/park.
Select retains rail/content switching outside drafts/reviews. Start retains
pause/menu behavior. Shared one-transition-per-frame, busy/callback lifetime and
Cancel-first guards prevent duplicate selecting/confirming input executing twice.
Vehicles/Dispatch resume through the existing section table. Operations,
Maintenance and demolition keep their existing order and behavior.

## Validation and limits

Public-data native fixtures use real Small/Flat/Track actions, not invented
operating tracks or guests. A genuine native Merry-go-round confirms fixed-count
filtering; a placed Dodgems validates multi-vehicle count bounds/action/accounting;
a native wooden station validates loaded car bounds, station clamping, paused
zero-cost native changes, draft cancellation, invalid/repeated/stale actions,
unrelated departure-bit preservation and native incomplete-track Test/Open
rejection. Configuration persists through native .park save/reload.

Real SDL/window fixtures exercise Dispatch route/resume, modal guards, duplicated and SDL-held
selecting input, Cancel-first cancellation, Vehicles unavailable/trackless state
and preserved Operations/Maintenance/demolition/HUD lifecycle. Native Release GUI/CLI build passes. The full handheld native regression driver
and real SDL lifecycle driver pass; lightweight CTest passes 3/3. The dummy
WindowManager omits reset notifications, so the native driver explicitly mirrors
that callback; the SDL driver tests the production reset hook directly.

Original private RCT graphics/scenarios are unavailable. No claim of a completed
live coaster Test/Open/guest run after configuration changes is made. The native
fixture tests reject incomplete coaster opening honestly; existing complete flat
ride/access tests continue to validate native opening. Asset-backed complete
coaster operation and physical Brick readability are follow-up validation.
Captures are 512×384 production-component fixtures, not native scenario captures.
The vehicle capture's station is placed by native TrackPlaceAction; the world is
not rendered. No fabricated ratings, guests or operation statistics.

## Changed files

- RideManagement.h/.cpp: vehicle/dispatch adapters, effective reviews and snapshots.
- Menu.cpp: new routes, section return/resume and confirmation callback parents.
- WindowManager.cpp: retained ride identities invalidated on complete UI reset.
- VehicleDispatchTests.h, RideDepthTests.h: native fixture integration.
- PresentationLifecycleTests.cpp: production SDL focus/confirmation/handoff.
- PanelPreview.cpp, render_shell.py: native component captures.
- FEATURE-DEPTH-MATRIX.md and this checkpoint.

Deferred: vehicle-object/reversed-train controls, mode-specific launch/time/laps/
rotations/circuits, music/appearance, graphs. No gameplay rules, save formats,
construction or Pocket palette/layout are changed.
