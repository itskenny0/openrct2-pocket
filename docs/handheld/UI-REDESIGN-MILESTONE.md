# Handheld UI redesign: dashboard, paths, rides

## Baseline audit and implementation plan

Canonical baseline: 30d55f8f782c7bb16fbfb817ab9ec7d078a98651, tree
653d3473e7316459050fb467aea898603d6e0c1d. The retained local commit
0bf53c9 has exactly that tree; this session uses an isolated clean checkout.
The unrelated Esperanto edit in the retained checkout is excluded.

Rendering is not a half-size menu hack. UiContext::Resize divides SDL output by
windowScale; both NextUI profiles request 1024x768, scale 2, DPI inference off.
UiContext::CreateRenderer selects nearest-neighbour at integral scale. Thus the
intended logical canvas is 512x384. Actual fullscreen dimensions still depend on
the device; this session cannot establish its physical display mode.

Architecture inspected:
- Panel.h supplies overlay lists, rows, command badges and path options.
- ConceptStyle.h supplies palette boxes and a fixed 164px construction rail.
- ManagementPanel.h serves dashboard sections, ride/staff/guest/finance/research
  adapters; Dashboard.h adds a 138px category rail. Menu.cpp owns focus/actions.
- ObjectPicker.h / PathSurfacePicker.h already have native visual browsing; keep
  these rather than replacing them with text lists. Landscaping and scenery use
  contextual overlays; information/graphs and safe reviews require separate layouts.
- The old management layout adds every stat and body line above the actions,
  leaving very little capacity on ride detail. It flattens status into roster names,
  hides page count whenever feedback exists, and has no visible overflow rail.
- Free-path overlay spends 278px on passive rows that look selectable. Connected
  paths use a different generic construction summary. Options need a wider rail
  than live placement, and must preserve centre reticle visibility.
- D-pad pans in park, navigates in menus; A activates, B backs out. Select changes
  dashboard pane or opens construction options/surface; Start returns to park.
  L/R emit previousOption/nextOption: used by park map, otherwise ignored by the
  dashboard. Triggers retain tool slope bindings. Select and Back preserve section
  resume state; returning to ride inspection currently resets its action focus.
- Native eligibility, drafts, query/execute accounting, Cancel-first reviews and
  action index dispatch must stay intact. Do not reorder the underlying ride rows.

Implementation sequence:
1. Shared hierarchy, metric tiles, action rows/tiles, explicit disabled metadata,
   visible pagination and bounded content layout. Preserve existing data adapters.
2. Dashboard: slimmer illustrated category rail and readable summary hierarchy;
   ride content expands to the available width with a two-column action layout.
3. Paths: one compact live tool summary, separated active mode/surface/geometry,
   settings grouped by task with controller prompts and native surface browsing.
4. Controller: grid-aware movement, shoulder paging without crossing action bounds,
   right-to-enter category navigation, and retained ride inspection focus.
5. Compile ARM GUI/CLI, run production controller/native action drivers, render
   actual production components at 512x384, inspect and iterate. Document evidence
   and remaining screen migrations below before publication.

## Implemented foundations and workflows

`Panel.h` now separates passive metrics, section labels, focusable actions and
page position. Focus has a filled surface plus a bright leading edge; unavailable
actions retain their text and a dash marker. `ModernText.h` adds a licensed 17px
heading atlas alongside the existing 12px text/strong atlases. The font generator
accepts size/height parameters while keeping existing defaults unchanged.

`ManagementPanel.h` measures layout in one place for rendering and page navigation.
Adapters opt into summary metrics, roster subtitles and action grids; native action
indices remain unchanged. Feedback and page position occupy different lines, with
an overflow indicator. Existing graphs, editable stat highlighting, text keyboards,
read-only bodies and Cancel-first confirmations remain supported.

The dashboard uses a 112px section rail with original native tab artwork clipped
to its own slot, and semantic monochrome fallbacks where original art is absent.
The representative park overview shows all six ordinary controls in a two-column
grid alongside actual park statistics and its native objective. Longer translated
objectives can still require paging. Right enters a section; A and Select retain
their existing entry semantics. L/R switch sections while the rail has focus.

Ride content expands to the full 496px workspace while active. The roster separates
ride name from status/queue. Inspection uses three summary metrics and a two-column
action grid. D-pad movement respects columns and edges; L/R page read-only ride
views. Select returns to sections; coming back or returning from a ride subpage
retains the selected action. A new ride starts at the first action. Edits/reviews
cannot use page navigation to bypass draft or Cancel-first guards.

Free and connected path tools share a 164x146px live summary instead of the former
278px free-path rail. Active type, surface, geometry and Y settings are distinct;
passive rows no longer imitate selectable controls. Settings use a separate 240px
rail grouped around geometry and surface/recovery. The park reticle at x=256 stays
outside it. Surface-grid cancellation and undo cancellation retain their options
focus. Native placement queries, errors, connected geometry, cost and undo logic
are unchanged. Live D-pad/A/B/X/Y/Select and camera/slope bindings are preserved.

## Migration guide for the remaining screens

| Area | Shared behavior inherited now | Deliberate next migration |
| --- | --- | --- |
| Ride entrances/exits | New management hierarchy and full ride workspace | Native station cards/preview; retain point/context/quote guards and connected-path shortcuts |
| Scenery selection | Existing native visual picker remains | Group/category browsing and object detail; do not replace sprite grids with generic rows |
| Landscaping | Existing contextual tool/area controls remain | Adopt compact active-tool summary, then validate brush/height preview and HUD safe areas |
| Finances | Heading/action/status/overflow foundations | Opt overview into summary metrics; preserve editable highlighted-stat and ledger/graph layout |
| Research | Shared content/confirmation foundations | Project progress summary plus priority controls; retain native funding/research eligibility |
| Staff/guests | Shared actions, headings and status | Roster subtitles, role/needs summaries and selective grids; retain filters, identity and focus rules |
| Park information/marketing | Shared typography and management layout | Separate summaries, history/detail and campaign targets; preserve native data and paging |
| Other construction | Existing native geometry/actions untouched | Reuse compact summary only after each tool's controls, costs and viewport footprint are checked |
| Save/load/text entry | Existing identity, atomic publication and Cancel-first guards | Review long-path/name overflow separately; keep keyboard columns and native file preview |

For each adapter: opt into presentation metadata, keep native action ordering,
use `MeasureManagement` for any new paging/navigation, add a dispatcher-level
workflow test, then inspect a real 512x384 production render. Do not enable a
multi-column layout without migrating its D-pad movement. Do not make draft,
confirmation or graph-direction bindings behave like category switching.

This milestone is not a full visual migration. Some remaining information screens
still supply dense native text; very long bodies, translated labels, advanced
construction menus and specialized graph/picker layouts need their own inspections.


## Verification and delivery

The actual AArch64 GUI and CLI link successfully with the retained Cortex-A53 /
GCC 16.2 / glibc 2.26 SDK. All 160 UI objects were rebuilt for the shared font/header
changes; affected dependencies were rebuilt again after visual iteration. The
final dependency audit found zero stale UI objects. Unchanged native engine
objects from the exact baseline were reused. Both executables run their version
command under QEMU with the matching target loader/runtime. The generated version
banner still contains its historical recovery reference; use the evidence manifest
source and executable hashes to identify this build.

The new UiRedesignTests driver passes against the final production GUI objects:
right-to-enter sections, bounded grid axes/top edge, shoulder paging, section and
Back focus retention, unavailable admission guard, Cancel-first demolition and
unchanged cash/ride, path settings/surface/undo cancellation, native entrance/exit
path shortcuts, and a paid connected landing with exact native accounting.
The overview fixture also asserts that all six controls fit its measured layout.

Twelve separately relinked preservation drivers pass: PauseResumeTests,
RideAccessRelocationTests, AccessShortcutTests, MazeAccessPreviewTests,
SceneryPathParityTests, PresentationLifecycleTests, ConstructionTests,
MazeConstructionTests, RideConstructionTests, PathScenarioUsabilityTests,
OrdinaryPlayUiTests and PathActionsTests. These retain native query/execute,
held-input/draft safety, costs, access/construction and native save/reload checks.
PresentationLifecycleTests addresses action destinations using the retained grid
focus; its native assertions were kept. Its old assumption that Down means the
next linear action was intentionally replaced; UiRedesignTests covers the grid.
The unchanged retained standalone CTest binaries pass 5/5 (input, launcher, save
preservation, file browsing, named saves); they were not rebuilt in this session.

Fourteen individual production component PNGs at 512x384 and a contact sheet are
in captures/ui-redesign/. They show dashboard rail/content, ride roster and both
action pages, statistics, disabled action feedback, safe demolition review, free
path settings/live panel, connected settings/landing/endpoint, and access shortcut.
All were visually inspected. These are actual X8 production rendering over a blank
park background, using loaded public object metadata and a synthetic native park.
They are not AI mockups, private-asset gameplay captures or physical Brick images.
The installed original tab sprite is clipped to its slot; missing original sprites
use the semantic fallback. Offscreen SDL hardware-renderer warnings remain in logs;
the component captures use the software X8 drawing engine directly.

No physical Brick display/input test, sustained park operation, full private-asset
render sweep, localized-text sweep, dense-park performance test or multiplayer test
was performed. No installable update ZIP was requested or produced for this bounded
source/UI milestone. Existing launchers, assets, saves and configurations remain
outside the change. The remaining screen migrations above are not claimed complete.

To reproduce the new driver, build the GUI with Ninja, then run:

```sh
python scripts/handheld/verify-ui-redesign.py --build build-handheld \
  --data /absolute/path/engine-data --profile /absolute/path/test-profile \
  --captures /absolute/path/captures
```

For cross builds add `--runner-prefix` containing QEMU and the matching loader /
library path. The script links the driver against the real existing GUI objects;
it does not supply a separate mock implementation. Native RCT2-compatible public
object metadata is required by the fixture. Evidence contains observed test/build
logs, source/executable/capture hashes and the complete preservation result list.

During the pause before publication the checkout's shared Git alternate disappeared.
The working source, executable and capture files survived; their recorded hashes
were checked again. The authenticated canonical branch still matched the audited
baseline. Publication therefore uses the exact canonical tree plus verified changed
blobs and an expected-head, non-forced branch update, followed by tree/head readback.
