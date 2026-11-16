# Park Overview checkpoint

Continues GitHub checkpoint `21c4aba5c2f92373b5e3130536f499fc49a32890` on
`handheld/modern-ui-paths`. Overview is enabled and is the default selected rail
section on a fresh Dashboard opening. All seven existing rail categories now
have completed native workflows. No completed section, tool, picker or ghost is
redesigned in this milestone.

## Concise player workflow

Start → Dashboard with Overview selected → A enters the summary → review a
permitted Open/Close park change → explicitly Confirm or Cancel → summary →
rail/park → save/reload.

The right pane shows native park name, open/closed status, in-park guest count,
rating out of the native maximum 999, park value, the complete formatted native
objective and its recorded outcome. There are only two actions: Open/Close park
review and permitted Admission / Finances access. B returns to the rail; B on the
rail returns to the park. There is no additional summary/detail drill-down.

Objective text uses the same `Ui::formatObjective` and `kObjectiveNames` as the
native desktop Park objective page, including target quantities, dates,
currency, excitement and other native arguments. The native format parser removes
presentation tokens before native UTF-8 wrapping, so desktop black text does not
become invisible on the dark handheld panel. Recorded completion/failure uses
`scenarioCompletedCompanyValue` and its native sentinels. Undefined completion
means In progress; none/have-fun objectives say No completion target. Invalid
objective types and undefined park value say Unavailable. No-money park value
says Not applicable. This does not calculate invented percentages, predict
completion dates, call objective progression checks or change scenario outcomes.

Admission access uses `Finances::Available(admission)`. When permitted, it enters
the existing Finances overview with the admission row selected; a fresh A press
opens the completed native bounded editor. Existing financial review/cancellation
and B hierarchy are unchanged. Locked/free-entry/no-money pricing is shown as
unavailable and does not enter an editor. No duplicate price action or editor is
added. Returning to Overview through the rail retains its saved row.

## Native authority and safety

`ParkOverview.cpp/.h` owns read-only summary/goal presentation, status availability,
draft snapshots and native action adapters. `Menu.cpp` owns page/draft/focus and
callback lifetime state inside the existing shell.

- Status change Query/Execute uses only native `ParkSetParameterAction` open/close.
  Native pause/command permission rules remain authoritative. Both operations
  allow paused execution and have zero immediate cost; no upfront fee, refund or
  expenditure entry is invented. Simulation controls subsequent park effects.
- The native desktop Park entrance UI disables Open/Close for guests-and-rating
  objectives. The generic native parameter action does not enforce that UI rule,
  so the handheld availability/action guard explicitly mirrors it.
- Reviews snapshot the baseline park-open flag, requested inverse status,
  no-money setting, complete objective parameters and recorded outcome. Changed
  status/settings/objectives/outcomes reject the stale review. Repeated/unchanged
  requests are rejected. Native Query/Execute still validate the action itself.
- Confirmation starts on Cancel. Explicit Cancel and B discard the status review
  without changes. Select cannot switch panes while the review is unresolved.
- Native queued confirmation uses a busy guard and weak window lifetime/request
  token. Old callbacks cannot access destroyed or superseded menu pages.
- Existing semantic input suspension consumes one action per frame and requires
  release/fresh press across rail/content/review/return transitions. Selecting
  Overview cannot also open a review; review cannot also confirm; admission access
  cannot also edit or apply a price.
- Start keeps the existing pause/build/save menu transition and discards an
  unsubmitted review. Loading closes the menu through its existing lifecycle.
  No section or draft state is serialized. Existing path/tree/shop ghost cleanup,
  save/autosave suspension and return-to-tool revalidation remain unchanged.

Fresh Dashboard windows select Overview with rail focus and immediately display
its summary. Select switches rail/content with existing nonmodal section/row
resume. Returning to the rail retains the active section rather than resetting
ongoing navigation. Other sections and camera/controller mappings are unchanged.

## Layout and reusable shell

The production shell remains the 138-pixel left rail and 352-pixel right pane,
with eight-pixel outer margins, native wrapped body lines, quiet separators,
gold focus and the existing 28-pixel action strip. The Mines of Africa summary
and both actions fit on one page at 512×384. Its native objective includes its
900-guest target, October Year 2 deadline and 600-rating requirement.

Longer translated objectives may reduce the number of visible action rows under
the existing shared pagination; no objective information is deliberately shortened.
There is no new adaptive desktop layout. The existing unavailable-section fallback
now bounds-checks its title lookup; enabling every category exposed an optimized
build warning for out-of-range enum values in that fallback.

Solid palette entries remain panel/strip 118, border/separator 119, gold 53,
primary 21 and secondary 18. No dithering, replacement world sprites, simulation
changes, native action changes, naming, HUD or construction categories are added.

## Verification

- Native GUI and CLI builds pass.
- CTest input, launcher and save-preservation suites pass 3/3.
- Full UI-linked private-asset integration and engine-only integration pass,
  including every prior construction/ghost/shop/management suite.
- Python capture-script compilation and `git diff --check` pass.

New meaningful coverage includes native summary/currency/rating fields, native
formatted objective arguments, no-target/in-progress/completed/failed/unavailable
outcomes, undefined/no-money value presentation, forced-open objective guards,
native query agreement, successful paused/no-money open/close and exact unchanged
cash/loan/fees/expenditure/common-price settings; stale objectives/outcomes/status/
no-money settings; Cancel-first and B cancellation; duplicate semantic input and
actual mapped SDL held-A/release/fresh-A handoff; default landing, Select modal
guards and section resume; permitted/unavailable Finances admission link and
linked-editor cancellation; Start/pause ownership; menu teardown and native park
status/objective/accounting save/reload.

Older completed-section tests now use one explicit Finances-start helper rather
than assuming that Dashboard defaults to Finances. It reads the current section
index and navigates through the real rail. Existing unavailable-Overview test
expectations are updated to require working Overview entry/return. No coverage
for completed gameplay is removed.

`test/handheld/OverviewCapture.cpp` and `tools/handheld/render_overview.py` load the
private Mines of Africa scenario and render the production shell/Overview/Finances
components over the native world at exactly 512×384. Captures:

- `overview-rail.png`: default selected rail and native summary.
- `overview-summary.png`: right-content focus and both concise actions.
- `overview-confirmation.png`: native queried zero-cost, Cancel-first status review.
- `overview-finances.png`: existing Finances admission row selected.
- `overview-updated.png`: park status after one real native confirmation.

These are native scenario renders, not fixture images, AI art or physical Brick
screenshots. The driver asserts rendering/reviews leave cash, loan, admission,
ledger, objective and outcome unchanged. It then executes native Open/Close,
asserts exact accounting, saves/reloads the native objective and changed status,
and restores the original status using the native inverse action. Private
assets, captured images and saved parks remain outside git.

Reproduce with the established build/data/ninja/assets arguments to
`tools/handheld/run_native_test.py` and `tools/handheld/render_overview.py`, adding
`--scenario PRIVATE_SC6 --output CAPTURE_DIRECTORY` for captures. The existing
host-tool PATH and host-library LD_LIBRARY_PATH are required.

Host verification uses disabled networking/scripting/TTF/OpenGL. Physical Brick,
ARM packaging and multiplayer/replay validation remain outstanding. Objective
progress is native recorded outcome plus the actual displayed park statistics,
not a new goal-progress estimator. Admission remains subject to native rules.

## Files and next complete workflow

- `src/openrct2-ui/handheld/ParkOverview.cpp/.h`: native section adapter/presentation.
- `src/openrct2-ui/handheld/Menu.cpp`: Overview pages/routing/state/callbacks and
  admission link to existing Finances.
- `src/openrct2-ui/handheld/Dashboard.h`: enable/default Overview and safe fallback.
- `src/openrct2-ui/handheld/Explorer.h`: section introspection/presentation API.
- `test/handheld/PathActionsTests.cpp`: Overview integration and explicit existing
  section navigation with the new default.
- `test/handheld/OverviewCapture.cpp`, `tools/handheld/render_overview.py`: real
  scenario captures and native status/save integration.
- This checkpoint document.

Recommended next substantial workflow: existing handyman/mechanic patrol-area
editing from Staff inspection, using native patrol cells/overlays/actions,
bounded cancellable drafts and explicit safe confirmation, returning to the
same employee or park and preserving native patrol behavior through save/reload.
Do not add other staff roles, naming, HUD or construction categories with it.
