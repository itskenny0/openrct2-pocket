# Staff Management Completion — incremental checkpoint

Goal: 100% ordinary OpenRCT2 gameplay in a controller-native 512×384 interface.
Continues published `9b7ec7ab41bf9bd2501a4caee8fb483cbfad37e3`. Local safety
history is preserved; the equivalent source tree was verified against remote.
Esperanto edits remain untouched and excluded from publication.

## Phase 1 — all roles and costumes

The full native roster now includes handyman, mechanic, security guard and
entertainer. Existing two hire shortcuts/employee offsets are preserved;
Hire other role opens a four-role selector and the existing Cancel-first hire
review. Loaded role animation availability, native automatic placement, native
entity limits, zero upfront charge and actual monthly wages remain authoritative.
Security/entertainers have zero native order bits; they do not inherit handyman
orders. Inspection and statistics label all roles and use their native counters.

Entertainer Costume uses desktop getAvailableCostumeStrings, listing loaded native
entertainer animation objects by raw costume name. StaffSetCostumeAction alone
changes animation and native slow-walking behavior. Role, loaded availability,
baseline animation and stale/repeated selection are revalidated. No production
object loading/research changes occur. A missing-object guard fixes a native
StaffSetCostumeAction null dereference; bad selections return a native error.

Controls: Up/Down selects, A activates, B returns, Start retains the established
pause transition. Existing Dashboard focus and release-before-confirm behavior
remain. Costumes use the shared management panel; no desktop window is opened.

Validation: Release GUI/CLI build; new public-metadata StaffCompletionTests native
all-role hire/query, invalid roles/objects, costume query/execute/repeat guards,
exact weekly wages/no-money, dismissal and native .park save/reload. Expanded real
SDL window/controller lifecycle covers role picker, Cancel-first hire, duplicate-A
handoff and native costume action. Staff fixture operations remain graphics-disabled
because original animation sprites are absent; public fonts still render controls.
Native entity-pool exhaustion, asset-backed animation and physical Brick are not
claimed by these tests.

## Remaining phases

Patrol inspection/add/remove/clear and controller area selection; naming, role
filtering and remaining ordinary information/operations depth. Existing construction
and Ride Management remain regression contracts. Screenshots will use actual
production components and native fixtures after validated source publication.

## Phase 2 — patrol editing

Inspector → Patrol area enters a compact park tool. D-pad pans, A applies one
native rectangle, Y toggles Add/Remove, Select opens width/height (1–64 tiles),
mode, Clear review and Return to employee. X rotates the camera, L1/R1 zoom;
B exits to the employee. Options B preserves the editing context. Native
SetPatrolAreaToRender displays the selected employee's actual coverage; menu,
focus, replacement, staff removal, load and teardown hide/clear the owned overlay.
The shared native map-selection rectangle previews the exact brush footprint.

StaffSetPatrolAreaAction alone adds, removes and clears coverage and recomputes
consolidated patrol areas. Off-map/unaligned rectangles are rejected before native
clamping. This engine's PatrolArea::set stores TileCoordsXY individually; the older
4×4 comment in PatrolArea.h is historical. Pocket therefore selects native tiles,
not an independent block model. All roles use native patrol permissions and zero
cost; no ownership changes or entity/map mutation bypass native actions.

Clear is Cancel-first and snapshots the entire selected employee's native patrol
vector. Changed/departed/reused targets, stale coverage, empty/repeated clear and
pending action duplicates reject. Successful clear restores unrestricted native
patrol. Presentation context is transient and never serialized.

Validation: Release GUI/CLI; native 3×2 exact footprint, independent employees,
query-only behavior, invalid bounds/alignment, pending repeat guard, individual
removal, stale clear, clear/repeat and 2×2 save/reload. SDL inspector-to-park route,
duplicate-A brush adjustment, Cancel-first clear, native clear and options recovery
pass. Original-sprite patrol rendering/effectiveness and physical Brick remain
unverified. Naming, role filtering and uniforms are next; manual pickup remains
an explicit ordinary-gameplay gap until implemented.

Published Phase 1: `a6e4baa5782e00e5eaa3ffe092830982122592a1`.
