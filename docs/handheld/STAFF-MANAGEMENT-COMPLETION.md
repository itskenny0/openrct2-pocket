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
