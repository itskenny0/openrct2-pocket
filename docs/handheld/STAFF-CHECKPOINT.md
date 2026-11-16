# Controller Staff workflow — 2026-10-07

Continues `handheld/modern-ui-paths` from GitHub checkpoint
`7a0236464cd59f5f7957ff3746276309c842acee`. Completed finances, construction,
shop management, pickers, ghosts and solid palette presentation are retained.
This milestone implements handymen and mechanics only.

## Complete player workflow

Dashboard -> Staff -> hire a handyman/mechanic with native automatic placement ->
inspect the employee -> edit orders or review statistics -> locate in the park
or explicitly confirm dismissal -> return to park and save.

| Page | Controller behavior |
| --- | --- |
| Staff roster | Up/down navigates every entry across pages; A selects; B returns to Dashboard |
| Hire confirmation | Up/down chooses Hire/Cancel; A activates; B cancels; initial focus is Cancel |
| Employee | Select Locate, Orders, Statistics, Dismiss or Back; B returns to roster |
| Orders draft | Up/down selects a role order; A toggles only its draft; Apply orders submits; B/Cancel discards |
| Statistics | Native counters and employment duration; A/B returns to employee |
| Dismissal | A on Dismiss submits once; B/Cancel leaves employee untouched; initial focus is Cancel |
| Every Staff page | Start discards unsubmitted work and opens existing pause menu |

Locate closes the panel, then calls native `WindowScrollToLocation` on the main
viewport. The employee is visible without an opaque menu covering them. Camera
rotation and zoom remain the existing native/controller behavior.

The roster includes the entire supported set, sorted by role and native staff
number. Shared `ManagementPanel.h` displays the selected page (up to seven rows
when no stats are present) and its page count. Staff roster focus is retained on
return and refreshes when native roster changes occur. Security and entertainers
are not included or offered for hiring.

## Native actions and accounting

`StaffManagement.cpp/.h` is the small section adapter. It queries/executes native
`StaffHireNewAction`, `StaffSetOrdersAction` and `StaffFireAction`, without changing
their logic or adding simulation actions. Hire requests native automatic
placement, with already loaded role animations and desktop default orders;
handymenMowByDefault remains respected. No object loading happens in production.

Hiring has **zero upfront charge**. The panel shows native monthly wages via the
shared native currency formatter. Existing `FinancePayWages` pays one quarter of
the monthly wage each native weekly payment. Dismissal has no invented refund.
No-money parks permit the same native staff operations, show wages/refund as Not
applicable, and incur no wage cash payment. Native queries enforce entity limits
and disallow firing a mechanic actively fixing or inspecting a ride. All three
actions retain their native allow-while-paused behavior; menu pause ownership is
unchanged.

Automatic placement uses native walking guests/path locations or park entrances.
The adapter rejects hiring when neither exists, rather than leaving an employee
picked up when this milestone has no manual pickup/placement tool. Missing loaded
role animations are reported honestly. Native placement rules are not bypassed.

Orders editing preserves unsupported bits, toggles only the role's native mask,
and validates the original orders before submission. Queries/drafts never change
orders. Handymen expose sweeping, watering, bins and mowing; mechanics expose
inspection and repair. Native employee names, action descriptions, monthly wages,
employment duration and role-specific lifetime counters supply the display.
New hires legitimately have zero counters. No fictional productivity statistic
or arbitrary upfront fee is introduced.

## Lifecycle and input safety

Targets contain native entity ID, role, staff number, hire date and a presentation
generation. Every operation resolves the target again. The existing UI peep-close
notification in `WindowManager::CloseByNumber` invalidates handheld staff identities
when a native employee disappears. This prevents a reused native entity slot from
reviving an old dismissal confirmation. The adapter's dismissal callback also
invalidates its identity in UI-free integration contexts. No serialized staff or
entity layout changes are needed. Native queued execution remains authoritative.

The hire callback uses a weak menu-lifetime token and request generation. An
already authorized hire may complete after the user leaves its page, but cannot
reopen an exited menu or modify a newer menu request. The existing controller
suspension consumes only one action per frame and requires release/fresh press
across transitions. Selecting A cannot hire, dismiss, locate or place through a
newly entered page. Safe confirmations start on Cancel. Loading closes the menu
through the existing lifecycle and drops unsubmitted drafts.

Menu entry continues to clear tree/path/shop ghosts; provisional save hooks and
all existing tools are untouched. The native simulation owns staff placement,
orders, dismissal, patrol cleanup and financial effects. No patrol UI, manual
pickup, naming, new staff categories, HUD or ride construction is added.

## Presentation and captures

Dashboard adds Staff to its existing section table. All pages reuse the existing
244-pixel management rail, solid fills and thin controller strip. Palette entries
remain panel 118, border/separator 119, gold selection 53, primary text 21 and
secondary text 18. Native currency/localized text retains the established native
font fallback. No dithering or new artwork is added. Existing finance layouts fit
the shared page capacity and remain unchanged.

`tools/handheld/render_staff.py` links `test/handheld/StaffCapture.cpp` against the
native GUI build. It loads private **Mines of Africa**, hires ten employees using
production native automatic-placement actions to demonstrate roster paging, and
renders these exactly 512x384 captures:

- staff-roster.png / staff-roster-last.png
- staff-inspection.png
- staff-orders.png (unapplied mowing draft)
- staff-hire.png (Cancel selected)
- staff-dismissal.png (Cancel selected)
- staff-statistics.png

These are native scenario captures after actual hires, not fixture entities or
physical Brick screenshots. Rendering/drafting leaves cash, loan and admission
unchanged; native hires incur no upfront charge. The original scenario is never
overwritten. Private assets and capture outputs stay outside git.

```sh
python3 tools/handheld/render_staff.py --build ../build-ui \
  --data ../recovered/proof1/.ports/openrct2-handheld/engine/share/openrct2 \
  --ninja ../host-tools/usr/bin/ninja \
  --assets /workspace/scratch/486484e47585/rct-assets \
  --scenario '/workspace/scratch/486484e47585/rct-assets/Scenarios/Africa - African Diamond Mine.SC6' \
  --output /workspace/scratch/486484e47585/staff-captures
```

## Validation

- Native GUI/CLI build succeeds; existing CTest suites pass 3/3.
- Full integration passes with private assets, real UI and virtual SDL controller.
- Engine-only integration passes without private assets.
- New checks cover supported roles, missing auto-placement locations, query-only
  behavior, native automatic hiring and queued execution, exact zero upfront cash
  and unchanged ledger, exact weekly wages/no-money behavior, role orders and stale
  drafts, full roster/later-page mechanic selection, B/default-Cancel safety,
  duplicate-frame and real held-A/release/fresh-A handoff, native mechanic dismissal
  restrictions, native removal and entity-slot reuse, native locate/return, callback
  completion after menu exit, pause ownership, exact zero dismissal refund and
  staff/orders/cash save/reload. Loading with Staff open closes it.
- All existing finances, shop/common-price/demolition, Terrain, Water, surface,
  scenery removal, Paths/options/ghosts and Trees/picker/ghost tests still pass.

Run `tools/handheld/run_native_test.py` with the same build/data/ninja arguments;
add `--assets PRIVATE_ROOT` for real UI coverage. Use the established host-library
LD_LIBRARY_PATH and CMake-tool PATH. Expected negative logs remain native invalid
surface/price validation messages. `git diff --check` passes.

## Limits and next workflow

Verification uses the native host and virtual controller, not physical Brick
hardware. No new ARM package is claimed. Network/replay races are not separately
tested by this network-disabled host build. Automatic hiring requires an existing
native placement location. Unsupported staff and patrol/name controls remain later
work, and native wage/counter/statistics semantics remain unchanged.

Recommended next complete workflow: controller-native Research management — review
the native project/progress, draft funding and category priorities, explicitly
confirm/cancel, see native research unlocks reflected in existing eligible Build
pickers, and save/reload. Do not redo the completed finances or staff workflow.
