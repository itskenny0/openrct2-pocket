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

Published Phase 2: `bb17348865870388e860c5ca7b0c158aff9a2a56`.

## Phase 3 — operations and information

Staff Name reuses the eight-column 32-codepoint keyboard, retaining existing UTF-8
and erasing complete codepoints. OK queries StaffSetNameAction and opens a Cancel-first
review; cancellation retains the draft. Identity and native baseline name are checked
again on apply. Clearing resets the native default name; an already-default empty draft
is rejected. No employee field is written directly.

Roster Filter cycles All, Handyman, Mechanic, Security, Entertainer. Native identity
selection is retained during roster updates, and the existing two hire shortcuts and
employee row offsets remain. Role counts / wages lists actual per-role counts and native
monthly wages plus total; no-money parks show Not applicable. Orders and all role-specific
work statistics use native data. Uniform colour changes apply to the entire native role
(handyman, mechanic, security) through StaffSetColourAction, with loaded target identity,
baseline and ordinary 72-colour validation. Entertainers use their costumes.

Controls: D-pad navigates, A types/toggles/reviews/applies, B returns. Left/Right adjusts
the selected uniform colour; keyboard D-pad uses native Pocket grid navigation. Hiring,
dismissal, name and patrol-clear confirmations initially focus Cancel. Held A cannot
cross a menu handoff or type/apply twice.

Focused native tests pass all-role hiring, costs/wages/no-money, costume availability,
patrol query/execute/add/remove/clear/stale/pending/bounds, Unicode naming, invalid lengths,
role filtering, invalid/non-entertainer uniform selection, zero-cost uniform changes and
native save/reload. SDL lifecycle exercises real controller/window routing for names,
Cancel-first review, drafts, roles/costumes/patrol and existing construction/ride tools.

## Ordinary functionality still incomplete

Manual staff pickup/reposition and hiring without an automatic placement source remain
missing. Native patrol effectiveness, staff animations, uniform sprite colours and work
statistics accruing in a live asset-backed park need validation. Native entity-pool
exhaustion has not been fixture-tested. No AArch64 build or physical TrimUI Brick test is
claimed. Staff remains PARTIAL against the 100% ordinary-gameplay goal; delivered patrol,
roles and operations do not erase these gaps. Existing construction gaps remain unchanged.

Screenshots and final broader regression results are recorded below after capture review.
