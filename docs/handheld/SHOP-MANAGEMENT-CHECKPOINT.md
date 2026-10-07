# Controller shop / facility management — 2026-10-07

Continues source checkpoint `cbd1ef96a5d20d4604d8fac6076d39bc9815a4fd`
on `handheld/modern-ui-paths`. This completes placed-shop inspection, native
opening/closing, product/admission pricing and confirmed demolition. Construction,
pickers, ghosts, existing tools and simulation actions are not rewritten.

## Player workflow

Leave placement with B. Move the park/crosshair to a real shop or facility and
press A. Native viewport hit testing supplies the target; ghost tracks and
unsupported/stale identities cannot become a shop management target. Other rides
keep the existing inspection interface.

The compact 224px panel shows the native name and formatted operating status,
total customers, native customers/hour estimate, income/hour, profit/hour,
product prices and each available item's native sales counter. Undefined income
or profit reads **Unavailable**, not zero. Facilities without products/admission
have no editable price. Toilets expose native admission; information kiosks
expose both loaded product slots. No phantom on-ride-photo price is introduced.

D-pad up/down chooses Status, Prices or Demolish; A acts; B exits. On the price
page, up/down selects a product and left/right changes its draft by one native
money unit (0.10 GBP, exactly the desktop spinner increment). Native minimum and
maximum bounds apply. A submits the selected draft through RideSetPriceAction;
B discards it. Changing the focused item also discards an unapplied draft.
Existing common-price flags are displayed and preserved: the native action
propagates a shared item price across the park, including secondary products and
toilet admission. This workflow does not toggle those flags or load objects.
No-money parks show Free and disable price editing, like the native ride window.

Demolition has a separate confirmation page, initially focused on **Cancel**.
It shows a native queried refund. B and Cancel leave the building/cash untouched.
A on Demolish submits one RideDemolishAction and closes the menu. Native authority
restrictions, refunds, path disconnection, guest/queue cleanup and ride deletion
remain in the action. No-money parks honestly show that no refund is paid.

## Native validation and input safety

ShopManagement stores a map identity rather than an element pointer: ride ID,
ride type, loaded subtype, build date, tile, base height and direction. Every
operation rechecks the loaded ride and a matching real non-ghost track. A stale
identity cannot submit status, price or demolition actions. A vanished target
invalidates the menu snapshot; park loading closes the menu through existing UI
lifecycle. Native queries/execution remain authoritative.

RideDemolishAction::Query checks permission/pause but does not populate a refund.
The adapter first runs that query, then obtains RideGetRefundPrice, the exact
TrackRemoveAction-query calculation used by the native desktop demolition prompt.
No custom refund formula or simulation/action changes are added. Execution is
revalidated by the queued native action. Negative native refund cost is shown as
a positive receipt; tests compare the executed cash delta with that queried cost.

Shop inspection stays live rather than retaining the pause introduced by the
menu shell: this avoids bypassing native demolition's pause rule. An already
paused park stays paused. Start opens the existing pause page and discards any
price draft; leaving pause restores only a pause introduced here. Status and
pricing keep their native allow-while-paused rules. All original placement and
camera mappings are unchanged.

Existing controller suspension consumes the first semantic action per frame;
held A and its release cannot confirm a newly entered page or spill into park
placement. The demolition confirmation requires a fresh press and intentional
focus change away from Cancel. Menu opening still clears path/tree/shop ghosts;
save/autosave/provisional handling remains unchanged.

## Rendering and captures

The production renderer is shared by Menu and the scenario capture harness:
`src/openrct2-ui/handheld/ShopManagement.cpp/.h`. It uses existing Concept::Box,
modern/native text, solid palette and 28px action strip. No dithering, new
artwork, object loading, placeholder sprites or desktop frame are added.

Palette entries remain panel 118, border/separator 119, selection 53, primary
text 21 and secondary text 18. Native formatted currencies/localised names use
the renderer's existing native-font fallback where needed.

`tools/handheld/render_shop_management.py` links
`test/handheld/ShopManagementCapture.cpp` against the native GUI build. The driver
loads the user's private Mines of Africa scenario, finds a native eligible
path-adjacent tile, places/opens a Burger Bar using the production native action,
checks the exact purchase charge and native viewport inspection hit, then renders:

- shop-inspection.png
- shop-prices.png (unapplied 0.10 GBP draft)
- shop-demolition.png (Cancel focused, native queried refund)

These are exactly 512×384 automated native scenario snapshots, not fixture art
or physical Brick captures. The newly built shop has no sales yet; unavailable
hourly values are shown honestly. The private RCT installation stays at
`/workspace/scratch/486484e47585/rct-assets`, outside git.

## Validation

- Native GUI build links successfully.
- Existing CTest handheld-input, handheld-launcher and handheld-save-preservation:
  3/3 passed.
- Full native integration suite passes with private assets and real SDL UI;
  native engine-only integration also passes.
- New integration covers real/non-ride/ghost/invalid/stale inspection identities;
  open/close and unchanged cash; one/two/zero product slots; toilet admission;
  local primary pricing and shared primary, secondary and admission pricing;
  native invalid-price rejection and controller min/max bounds; B/default-Cancel
  safety; duplicate-frame and virtual-SDL held-A/release/fresh-press handoff;
  protected and paused demolition; externally paused inspection and no-money
  price editing; exact targeted demolition/refund and neighbour preservation;
  status/price/common flags/cash persistence; load while inspecting; and
  removal/refund persistence after save/reload.
- Existing Terrain, Water, surface paint, path options/ghosts, scenery removal,
  Trees/picker/ghosts, shops/picker/rotation/ghost/save/research tests still pass.
- Expected negative-test logs include invalid surface and out-of-bounds prices.
- git diff --check passes; all private assets and build/capture outputs are outside git.

## Limits and next workflow

Physical Brick validation and an ARM release package are still pending. The
menus do not add renaming, common-price toggles, staff, flat rides, coasters,
keyboard or a persistent HUD. Native hourly statistics may need simulation time
before becoming available. Existing native-font fallback is retained.

A carried-forward display issue exists in older tool cost badges and legacy
read-only dashboard pages: their dollar formatter divides native money by 100
rather than 10. This milestone uses native currency formatting for all new shop
prices/refunds; it does not change those older tool renderers. Native cash/query
accounting is correct and covered independently of labels.

Recommended next complete workflow: controller-native park finances — readable
native cash/income/expenses, loan borrowing/repayment and permitted park admission
pricing, with native actions, bounded drafts, confirmations and save/reload.
Use the native money formatter consistently as part of that financial workflow;
do not redo finished construction or UI polish.
