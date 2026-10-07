# Two-pane Dashboard shell — 2026-10-07

Continues `handheld/modern-ui-paths` from GitHub checkpoint
`c2e8073958788a4f294e17c4e6cd682311ab1a20`. This is a management navigation and
presentation migration, not new financial or staff gameplay. Native action code,
Staff/Finances adapters, construction tools and provisional-save logic are unchanged.

## Player controls

Start in the park now opens Dashboard, including when a handheld construction
tool is active. The tool remains active and its ghost is removed during menu
entry; returning to the park revalidates/restores it normally. Existing native
desktop-tool guards remain unchanged. Start in construction pickers and other
existing menus retains their original pause behavior.

| Context | Controls |
| --- | --- |
| Navigation rail | Up/down selects category; A enters its available content; B closes Dashboard directly to park |
| Content | Up/down selects actions; A activates; B backs out one level |
| Section overview, employee inspection/statistics | Select switches rail/content, retaining section view and row |
| Financial draft | Left/right adjusts; A reviews; B discards to overview |
| Financial confirmation | Initial focus Cancel; B returns to draft; A activates focused choice |
| Staff orders | A toggles draft; Apply submits; Cancel/B discards |
| Staff hiring/dismissal | Initial focus Cancel; A activates; B cancels |
| Drafts / confirmations | Select is consumed without switching panes; B must resolve the pending state first |
| Inside Dashboard | Start opens the unchanged pause/build/save/load menu and discards unsubmitted drafts |

The existing pause menu's B behavior still returns to the park. It is a separate
menu, not another Dashboard category. Save/load/exit and Build remain reachable
through that menu. No physical mappings or fake-mouse controls are introduced:
Select uses the existing semantic `openBuild` action only while in Dashboard.
In park/construction contexts Select keeps its original meaning. Camera buttons
and every other tool mapping are unchanged.

## Architecture

`Dashboard.h` contains the permanent category registry, navigation-only
`Dashboard::State`, availability checks and shared two-pane renderer. The rail
order is Overview, Rides, Guests, Staff, Finances, Research, Marketing. Only Staff
and Finances are enabled. Unavailable entries are explicitly labelled and cannot
acquire content focus or submit actions. Their pane reports Not implemented;
no fake values, charts or working screens are supplied.

`Menu.cpp` keeps section-specific drafts, targets and native action dispatch. Its
Dashboard navigation state is separate from the existing section/modal page state.
Per-section resume records retain only a nonmodal page and row. Returning to a
section can resume employee inspection/statistics; pending drafts/confirmations
never become a resumable rail preview. Native targets are revalidated as before.
Future categories add their section adapter/routing, availability and content
presentation to this shell, rather than inventing rail, pane or controller logic.

`ManagementPanel.h` extracts `DrawManagementContent` with an explicit origin,
size and focus flag. The Dashboard renderer owns the rail and single bottom
strip; the content renderer owns titles, native stat rows, action focus, paging
and feedback. Existing legacy capture wrapper remains compatible. Staff and
Finances presentation/data models are reused without changing gameplay actions.

`Explorer.cpp/.h` routes Dashboard navigation before individual section handlers,
handles Select focus switching and changes the park's existing `openMenu` route
to `HandheldDashboardOpen`. Existing input suspension consumes one semantic action
per frame and requires release/fresh press across rail/content/modal transitions.

Legacy minimal Overview/Guests/Research page code remains in the source but is
not exposed as a migrated Dashboard workflow. Research eligibility filtering and
native research actions/tests are preserved. Those sections require subsequent
complete management milestones before being enabled in the rail.

## Native behavior retained

Finances retains native cash/loan/current-month net ledger totals/current profit,
effective admission, bounded drafts, query/review/Cancel-first confirmation,
loan/funds/admission/no-money restrictions, exact cash accounting and native
currency formatting. Staff retains full roster paging, automatic handyman/mechanic
hiring, native wages/status/counters, locate, cancellable role orders, Cancel-first
dismissal, mechanic restrictions, stale-generation/slot-reuse protection and
callback lifetime guards. Locate still closes management and scrolls the native
viewport so the employee is unobstructed.

Menu pause ownership, native queued execution, ghost cleanup/revalidation,
save/autosave and park-load menu teardown remain the established implementations.
No simulation, financial action, staff action, object loading, research or world
rendering logic changes are included.

## Layout at 512x384

- Usable width: 496 pixels, with eight-pixel outer margins.
- Rail: 138 pixels (~28%); content: 352 pixels (~71%); six-pixel gutter.
- Both begin at y=8; rail is 338 pixels high. Content height follows its actual
  data/actions up to 338 pixels, exposing the park below shorter pages.
- Category rows use 38-pixel pitch. Disabled categories show an Unavailable
  subtitle. A gold fill marks rail focus; a slim gold marker retains the active
  category while content has focus. Only focused content actions/drafts get gold.
- Content keeps native stat rows and paging; long names/feedback ellipsize rather
  than overlap. No information is invented to fill empty space.
- Bottom controller strip remains 28 pixels with four short, contextual prompts.
- Solid palette entries remain panel 118, border/separator 119, selection 53,
  primary 21 and secondary 18. Native currency/localized glyph fallback remains.
  No dithering, gradients, new sprites or overlapping desktop frames are added.

The Finances overview and confirmation occupy almost all management space, so
the park is mainly visible in margins/gutters on those pages. This is intentional
management readability, not a construction-mode redesign. The layout targets
512x384; it does not implement an adaptive desktop Dashboard design.

## Verification and captures

- Native GUI/CLI build succeeds.
- Existing CTest input/launcher/save-preservation suites pass 3/3.
- Full native integration with private assets, real UI and virtual SDL controller
  passes all previous tool, shop, Staff and Finances tests after route migration.
- Engine-only suite without private assets passes, including pure navigation-state
  availability/wrap checks; interactive Dashboard checks run in the real UI suite.
- New coverage: opening/closing and pause ownership; unavailable categories; rail
  and content switching; direct employee-detail resume; drafts/confirmation focus
  guards; B through Finances confirmation/draft/overview and Staff orders/detail/
  roster; duplicate-frame and mapped SDL held Select/A release/fresh-press handoff;
  stale staff confirmation after native removal; path ghost cleanup/restoration;
  and save/reload while a finance draft is open.
- Native simulation/action adapters are unchanged. Existing exact finance, wages,
  no-money, common-price, refund, slot-reuse, research, construction and save tests
  remain enabled and pass.
- `git diff --check` and Python capture-tool compilation pass.

`test/handheld/DashboardCapture.cpp` and `tools/handheld/render_dashboard.py` render
the production shell/content components over private **Mines of Africa**, at
exactly 512x384. One handyman is hired through native automatic placement; it
incurs zero upfront cost. Financial values and employee statistics come from the
native park. Rendering, previews and unsubmitted drafts leave cash, loan and
admission unchanged, asserted by the harness. These are native scenario captures,
not fixture images or physical Brick screenshots. The original scenario and
private assets stay outside git.

Captures: dashboard-finances-rail.png, dashboard-finances-content.png,
dashboard-staff-rail.png, dashboard-staff-content.png, dashboard-staff-detail.png,
dashboard-loan-confirmation.png and dashboard-unavailable.png. Existing finance
and staff capture drivers also render the new production shell.

```sh
python3 tools/handheld/render_dashboard.py --build ../build-ui \
  --data ../recovered/proof1/.ports/openrct2-handheld/engine/share/openrct2 \
  --ninja ../host-tools/usr/bin/ninja \
  --assets /workspace/scratch/486484e47585/rct-assets \
  --scenario '/workspace/scratch/486484e47585/rct-assets/Scenarios/Africa - African Diamond Mine.SC6' \
  --output /workspace/scratch/486484e47585/dashboard-captures
```

Run `tools/handheld/run_native_test.py` with the same build/data/ninja arguments;
add `--assets PRIVATE_ROOT` for real UI coverage. Use the established host-library
LD_LIBRARY_PATH and CMake-tool PATH. Expected native negative-test validation logs
remain unchanged.

## Remaining UX limits and continuation

New Dashboard openings default to Finances with rail focus. Nonmodal section view
and row are retained while the menu stays open, not across closing/relaunching.
Select does not leave a pending draft or confirmation; this is an explicit safety
boundary. Five categories remain unavailable. Pause/save/build remains a separate
existing menu accessible with Start. Naming, patrol painting, persistent HUD and
other requested later systems are not added. Physical Brick/ARM-package and
network/replay validation remain pending.

Next complete workflow: native Research management inside the existing right
pane — current project/progress/available date, cancellable funding/category
priority drafts, review and safe confirmation, native unlock visibility through
existing eligible Build pickers, and save/reload. Enable Research only when that
complete workflow is implemented; preserve this navigation/focus shell.
