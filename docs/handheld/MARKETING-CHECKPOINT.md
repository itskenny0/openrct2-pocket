# Marketing workflow checkpoint

Continues remote `31e9fd90fd2e0672fffc0966c894c0d0360cd257` on
`handheld/modern-ui-paths`. This milestone enables Marketing in the existing
two-pane Dashboard; no new navigation shell or construction features were added.

## Player workflow

Start → Dashboard → Marketing → New campaign → choose an eligible campaign →
choose its required target → set 2–12 weeks → Review → explicitly select Start
campaign or Cancel → return to the park. Active campaigns have a read-only detail
page showing their native target and remaining weeks. There is no invented
campaign-cancellation action.

The rail/content focus, Select switching and section resume use the existing
Dashboard implementation. D-pad navigates rows and paged target lists. On the
duration row, Left/Right adjusts the bounded duration and A cycles it. A selects
or reviews; B backs out one level. B from confirmation retains the draft; leaving
the editor discards it. Start retains existing pause behavior and discards drafts.
Confirmations default to Cancel. Draft, target and confirmation pages hold modal
focus, preventing Select from carrying a draft into another section. Input-frame
consumption and a pending-action guard prevent selection/confirmation spillover
and duplicate queued purchases.

## Architecture and native rules

- `MarketingManagement.h/.cpp` owns campaign/target adapters, bounded drafts,
  fresh validation, native query/execute and shared management presentation.
- `Menu.cpp` owns section pages, cached navigation lists, drafts, modal guards,
  resume and callback lifetime handling. `Dashboard.h` enables the completed rail
  section. Existing shared Dashboard drawing provides the same solid palette,
  persistent rail, right pane and thin action strip.
- Native `ParkMarketingAction` remains responsible for query/action cost,
  funds, pause permission, payment and marketing ledger entries. Native
  `MarketingUpdate`, campaign vouchers, guest-generation effects and persistence
  are unchanged.
- All six native campaign types are supported when eligible: free park entry,
  free ride vouchers, half-price park entry, free food/drink, park advertising and
  ride advertising. Native per-week prices are respectively £50, £50, £50, £50,
  £350 and £200. Currency display uses the existing native formatter.
- The adapter mirrors native desktop eligibility/target filtering because the
  underlying marketing action does not validate every target or applicability
  restriction itself. It rejects active types, authority restrictions, locked
  admission/ride-price cases, missing targets and stale targets before query or
  confirmation. It does not load objects or unlock research.
- Ride targets include the full eligible open-ride set, with scrolling/paging;
  shop/facility targets are excluded. Food/drink targets are deduplicated native
  products sold by currently open loaded providers, using native product names.
  The reusable management row paginator is appropriate for these semantic
  targets; no parallel object-preview asset pipeline was introduced.
- Ride target identity includes ID, type, object, build month, location and a
  deletion generation. The native ride-window close/removal notification updates
  that generation, preventing a reused ride ID from silently retaining an old
  draft. Re-selecting an available target is required after invalidation.
- No-money mode follows native actions: campaigns can be started when eligible,
  without a cash/ledger charge; monetary presentation says Not applicable.
  Marketing actions are allowed while paused by the native action flag.
- Weekly progression retains the native first-partial-week rule and native
  never-ending-marketing cheat. No weekly recurring campaign charge is invented.

## Verification

The native GUI/CLI host build succeeded. Full native UI-linked integration tests
and the engine-only integration variant passed, including existing construction,
ghost, shop, Staff, Finances, Research and save/reload coverage. CTest passed all
three suites: handheld input, launcher and save preservation. Python capture
script compilation and `git diff --check` passed.

New integration coverage exercises all six types, admission/ride-price locks,
authority restrictions, 18 ride targets and paged navigation, native product
filtering, bounds including extreme adjustments, missing/closed/reused targets,
draft/target cancellation, rail/content resume, modal input guards, Cancel-first
confirmation, duplicate input/queued purchase protection, exact native cash and
ledger changes, insufficient funds, pause/no-money behavior, native vouchers and
guest-generation probability, weekly progression/expiration and save/reload.
Fixture object loading/ride allocation is confined to tests; production never
loads additional objects. Original scenario state is restored after tests.

## Native 512×384 captures

`test/handheld/MarketingCapture.cpp` and `tools/handheld/render_marketing.py` render
production Dashboard presentation over the native Mines of Africa scenario from
the private RCT installation. These are native scenario captures, not fixture art.
Private assets remain outside git.

The capture setup opens an existing valid scenario ride through a queried native
status action, then starts a real two-week park-advertising campaign for £700.
The overview shows one active campaign and actual cash £9,300. The unsubmitted
three-week Lift 1 advertising draft queries £600 and the confirmation shows cash
after £8,700 with Cancel selected. Rendering is checked not to change cash, loan,
admission or campaigns. The source scenario is not overwritten.

Outputs: `marketing-overview-rail.png`, `marketing-overview-content.png`,
`marketing-draft.png`, `marketing-confirmation.png`, all exactly 512×384.

## Limits and next milestone

Native sprite-font text and the shared Dashboard pane constrain long names;
lists use the existing pagination rather than wider panels. Campaign effects
are not presented as fabricated forecasts. Expired/unavailable active entries
are shown honestly; starting a new campaign always revalidates native state.
Conservative ride-window invalidation can require target re-selection after a
desktop ride-construction transition. Multiplayer/replay races were not tested.
This checkpoint was built/tested on the host, not packaged or tested on a physical
Brick. Existing renderer/palette constraints remain unchanged; no dithering.

The next recommended complete workflow is Guests within the existing Dashboard:
native summary → paged roster → inspect native guest information/thoughts →
locate → return to the park, with stale-entity/input/save safety. Do not redo
Marketing, Research, Staff, Finances or the Dashboard shell.
