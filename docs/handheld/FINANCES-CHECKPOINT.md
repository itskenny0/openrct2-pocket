# Park Finances checkpoint

Continues `handheld/modern-ui-paths` from GitHub checkpoint
`9db7a25b6dacd4dda3381c986a505625376f4f2b`. Construction, shop management,
pickers and ghosts are retained. This milestone adds only park finances and
resolves the previously documented handheld currency scaling error.

## Player workflow

Open Dashboard, choose Finances, review the native financial state, edit a loan
balance or permitted admission price, review the draft, explicitly confirm,
then return through Dashboard to the park.

| Page | Controls |
| --- | --- |
| Dashboard / Finances | D-pad up/down selects; A enters; B returns |
| Loan / admission editor | Left/right adjusts draft; up/down selects Review or Cancel; A activates; B discards |
| Confirmation | Up/down selects action or Cancel; A activates; B returns to draft |
| All finance pages | Start discards the draft and opens the existing pause menu |

Confirmation always initially focuses **Cancel**. Cancel never changes the
park. Existing input suspension prevents a held/selecting A press from also
confirming a change. Returning to Dashboard retains its selected section.

## Native authority and accounting

`Finances.cpp` queries and executes `ParkSetLoanAction` and
`ParkSetEntranceFeeAction`. Drafts do not alter simulation state. Loan steps
match the desktop control (GBP 1,000); admission steps match its GBP 1 step.
Displayed values use the configured native currency conversion.

Loan drafts are bounded to zero and the native limit. When an existing loan
already exceeds a changed limit, repayment remains possible without allowing
further borrowing. Native queries enforce the loan limit and affordability of
repayment. Borrowing while cash is negative retains native behavior. Loan
execution changes cash exactly once and does not fabricate ledger income.

Admission editing respects `Park::EntranceFeeUnlocked`, free-entry and
unlock-all-prices rules, no-money restrictions and the native maximum. Setting
a price does not immediately change cash. No-money parks display financial
values as Not applicable and disable edits. The loan presentation guard follows
the native desktop's no-money UI restriction; it does not change the action.

Both native actions allow execution while paused. Existing menu pause ownership
is preserved. Drafts record the original field value; stale values are rejected
before submission. Native execution remains authoritative. Closing/loading
discards the menu and draft.

Finances shows native cash, loan, effective admission, current profit and the
current month's ledger totals. Month income/spent are sums of positive/negative
**net category entries** in `expenditureTable[0]`, not gross turnover estimates.
Undefined values display Unavailable. A newly loaded scenario may legitimately
have zero current-month entries. No history chart or forecast is added.

## Reusable presentation

Dashboard sections use one navigation table in `Menu.cpp`.
`ManagementPanel.h` provides a presentation-only compact left panel with title,
stat rows, optional highlighted draft, actions, feedback and the existing thin
controller strip. Future sections can reuse the table and shell without
changing controller architecture. Only Finances uses the new editor workflow.

Solid palette styling is unchanged: panel 118, border/separator 119, selected
gold 53, primary text 21, muted text 18. No dithering or new world art. The park
remains visible beside the 244-pixel panel. Native currency glyphs use the
existing native-font fallback; modern ASCII labels keep the existing font.

`Currency.h` supplies one `FormatMoney(money64)` based on native
`STR_CURRENCY2DP`. It replaces the old hard-coded dollar labels and `/100`
scaling in tool, Dashboard and read-only finance/ride displays; shop management
and older capture drivers use the same helper. Native values are tenths of GBP,
so the previous conversion was incorrect. Gameplay prices/actions are unchanged.

## Verification

- Native GUI and CLI build passed.
- Existing CTest suite: 3/3 passed.
- Full native integration suite passed with private assets and virtual SDL
  controller, including every existing tool/shop/ghost test.
- Engine-only integration suite passed without private assets.
- New coverage: native ledger totals and undefined values; currency formatting
  in pounds/dollars/euros; loan bounds, debt, repayment affordability, exact
  cash and unchanged ledger; native admission bounds/restrictions; stale drafts;
  no-money behavior; cancellation and safe defaults; held-A handoff; pause
  ownership; save/reload and loading with a draft open.
- Older capture drivers with formatter changes passed syntax compilation;
  native shop scenario capture was rerun to verify the corrected cost display.

Reproduce native integration:

```sh
python3 tools/handheld/run_native_test.py --build ../build-ui \
  --data ../recovered/proof1/.ports/openrct2-handheld/engine/share/openrct2 \
  --ninja ../host-tools/usr/bin/ninja \
  --assets /workspace/scratch/486484e47585/rct-assets
```

Reproduce captures:

```sh
python3 tools/handheld/render_finances.py --build ../build-ui \
  --data ../recovered/proof1/.ports/openrct2-handheld/engine/share/openrct2 \
  --ninja ../host-tools/usr/bin/ninja \
  --assets /workspace/scratch/486484e47585/rct-assets \
  --scenario '/workspace/scratch/486484e47585/rct-assets/Scenarios/Africa - African Diamond Mine.SC6' \
  --output /workspace/scratch/486484e47585/finance-captures
```

The host requires the established build-tool PATH and host-library
LD_LIBRARY_PATH. Captures are exactly 512x384, from native Mines of Africa:
Dashboard, Finances, loan draft, Cancel-focused confirmation and admission
draft. They use the production presentation functions and native viewport.
No fixture financial values or proprietary assets are committed. The capture
harness checks that cash, loan and admission are unchanged by previewing.

## Limits and continuation

Verification is native host/virtual-controller testing, not a physical Brick
test or a new ARM package. Network/replay behavior was not separately exercised.
Monthly totals have the native net-category limitation noted above. No staff,
rides, naming keyboard, HUD or broad visual changes are included.

Recommended next complete workflow: Dashboard -> Staff roster -> hire a native
handyman/mechanic -> inspect/locate and adjust role orders -> confirmed dismissal
-> return to park. Reuse the management navigation/shell and preserve all
confirmed finances, construction and ghost behavior. Patrol painting and naming
remain later milestones.
