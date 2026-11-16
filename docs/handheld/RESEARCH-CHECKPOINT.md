# Research Dashboard workflow

Continues `handheld/modern-ui-paths` from remote checkpoint
`cc11b236a8d4b40aae14cf6c0983e490d1ce9b31`. Only Research is added to the existing
two-pane Dashboard. The shell, Staff, Finances, construction, pickers, ghosts,
world rendering and native simulation/action implementations are preserved.

## Player workflow

Start opens Dashboard. Select Research in the rail and A enters its right pane.
The overview reads the native current project, development stage, available
expected date, funding and monthly research cost. Unknown initial projects and
dates stay Unknown; completed research reports All research completed and no
remaining projects. Project/category names and stage/funding/date strings come
from native localisation. No invented forecast or cross-stage percentage is used.

Funding / priorities opens one draft containing all seven native category bits.
Up/down navigates the paged list. On Funding, left/right adjusts within the four
native levels; A cycles those levels. A on an available category toggles its draft
priority. Categories with no remaining projects cannot be toggled. When neither funding nor
priorities is available, overview reports Settings unavailable and cannot open
an empty editor. Unknown bits
are preserved. Review settings queries the native action and opens a paged review
showing the new funding, monthly cost and every category state. Focus initially
rests on **Cancel**. Only A on Apply settings submits the action.

B from review retains the draft and returns to its editor. Cancel discards it.
B from editor discards to overview; B from overview returns to rail; B from rail
returns to park. Select switches rail/content on overview and retains its row;
Select is consumed inside drafts/review. Start drops unsubmitted work and opens
the existing pause/build/save/load menu. Existing per-frame suspension and release
handoff prevent selecting A from editing or submitting through the next page.

## Native authority and accounting

`ResearchManagement.h/.cpp` is the section adapter: read-only presentation,
bounded draft helpers and query/execute of `ParkSetResearchFundingAction`.
`Menu.cpp` owns routing, focus, the optional draft and modal transitions.
`Dashboard.h` enables Research and retains the existing category registry,
section resume and rail renderer. `ManagementPresentation::primaryAction` permits
short contextual A prompts without changing existing section defaults.

The draft stores the original funding/priorities. Both review and submission
reject settings changed externally, unavailable funding edits, or category
changes that ceased to be available. The native action remains authoritative;
no action/simulation code or serialization format changes are included.

Settings have **zero upfront charge**. The native monthly costs are GBP 0, 100,
200 and 400, formatted through the existing shared native formatter. Native
`FinancePayResearch` charges one quarter weekly; no custom charge, cash mutation
or funding ledger entry is invented by the handheld adapter. Funding controls
are unavailable in no-money parks or after all research finishes, matching the
desktop UI. Pending category priorities remain editable in no-money parks.
Native no-money progress and zero payments are preserved. Native all-off priority
fallback still chooses a remaining project; turning priorities off is not a
research lock. Production never loads objects or manipulates invention flags.
Native progression naturally updates eligibility used by existing Build pickers.

## Layout and captures

The 138-pixel rail, 352-pixel right pane, paged management content, 28-pixel bottom
strip and solid palette are unchanged: panel 118, border 119, gold 53, primary 21,
secondary 18. No dithering, placeholder sprites or new management shell.

`test/handheld/ResearchCapture.cpp` and `tools/handheld/render_research.py` render
the production presentation/shell and native viewport over private **Mines of
Africa**, at exactly 512×384. Overview rail/content, an unsubmitted funding/priority
draft, and Cancel-focused review are captured. The scenario starts in initial
research, so its actual project/date are Unknown. No fixture research state is
substituted. Rendering asserts cash, loan, admission, funding and priorities are
unchanged; original scenario/assets remain outside git.

```sh
python3 tools/handheld/render_research.py --build ../build-ui \
  --data ../recovered/proof1/.ports/openrct2-handheld/engine/share/openrct2 \
  --ninja ../host-tools/usr/bin/ninja \
  --assets /workspace/scratch/486484e47585/rct-assets \
  --scenario '/workspace/scratch/486484e47585/rct-assets/Scenarios/Africa - African Diamond Mine.SC6' \
  --output /workspace/scratch/486484e47585/research-captures
```

Use the established host-library LD_LIBRARY_PATH and CMake PATH. Run
`tools/handheld/run_native_test.py` with the same build/data/ninja flags, with
`--assets` for real UI/virtual-controller integration or without for engine-only.

## Verification

- Native GUI/CLI build passes; CTest input/launcher/save-preservation passes 3/3.
- Full native integration and engine-only suites pass, including all previous
  construction, eligibility, shop, Staff, Finances, ghost and save/reload tests.
- New coverage: funding bounds/native invalid-level query; priority availability
  and unsupported-bit preservation; stale funding/priorities/completed category;
  no-money priority editing, automatic progress and no research payment;
  zero upfront cash/ledger changes and exact native weekly payment; rail/content
  switching; editor/review modal locks; duplicate-A input handoff; cancellation,
  confirmation and Start ownership; native progression with all-off priority
  fallback; shop picker eligibility after native unlock; completed-state display;
  settings/unlock/cash save/reload and menu teardown on load.
- Integration fixtures isolate a loaded shop as a pending research item; only
  tests set up that fixture. Unlock is performed by repeated native ResearchUpdate,
  not ResearchFinishItem or manual invention shortcuts.
- `git diff --check` and Python capture-tool compilation pass. PNGs are 512×384.

## Limits and continuation

Long native project names ellipsize to fit 512×384. Editor and review page through
all categories rather than compressing them. Stage descriptions follow desktop
Research rather than presenting a misleading total percentage. Expected dates
use the native stored estimate; native funding actions do not immediately
recalculate it. It can lag until native progression refreshes the estimate.
Initial/zero-funded/finished states do not invent dates. Tests/captures are host
native/virtual-controller work, not a fresh ARM package or physical Brick test.
Network/replay is not separately validated. Other unavailable categories remain
unavailable. No additional gameplay or HUD/keyboard work is included.

Next substantial workflow: Marketing in this Dashboard shell — review active
campaigns, choose a native eligible campaign/target/duration, review native cost,
explicitly confirm or cancel, return to park and save/reload. Use ParkMarketingAction
and native campaign effects/accounting, without redoing Research or the shell.
