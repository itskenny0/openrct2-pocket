# Controller-native UX phase 2

Canonical baseline: 374c5b81168ceafe0b20c94a42ccc41d2346f9f8, tree
0bcb05147b2b1e877e7f8d74cd477c7aa0ff8ce4. Local source/index/working files
were verified against that complete tree and reconstructed as a self-contained
safety checkout after the previous shared Git alternate disappeared.

## Controller audit (before implementation)

Counts below start with the indicated screen/tool active, default focus, and one
D-pad tap per move. A means mapped confirm, B means mapped Back. Native placement,
queries, drafts and confirmation are distinct operations and must remain distinct.

1. Selecting a ride: park centre A opens the small quick-action card; A opens entrance/exit
   management for a completed supported ride (or resumes unfinished construction). Alternatively Dashboard > Rides > A enters roster > A selects ride.
2. Opening ride: root action grid mixes Open/Close/Test with access, Locate and
   appearance. Navigate to Open, A opens Cancel-first review, Up then A confirms.
3. Admission: find Admission price in the root grid, A opens price editor;
   Left/Right changes draft, navigate to Review, A, Up/A confirms. Back restores
   root action focus. No native change should occur merely by entering the editor.
4. Statistics: find Statistics, A opens seven native fields, then a second action
   group opens customers/finances/graphs. Back traverses these levels.
5. Operations/maintenance: both are near the end of the two-page root list.
   L/R page the list; each opens its own native settings/editor/review flow.
6. Paths: Select opens Build, choose Paths, choose normal/queue/connected mode.
   Free construction uses D-pad pan, A place, X remove toggle, Y settings,
   Select native surface grid, L/R zoom. B ends tool; reopening needs Build again.
7. Direction/elevation: connected direction requires Y > Down > A > B (4 presses
   from default settings focus); connected slope already has L2/R2, but the footer
   does not advertise it. Free elevation requires Y > Down twice > A > B (5), or
   a longer sequence if returning to another retained settings row. Free triggers
   currently do nothing. Free slope/orientation also require settings.
8. Surfaces: Select directly opens the native visual grid. A chooses a surface;
   B cancels without spending money or losing tool/mode. Retain this route.
9. Extension: connected mode selects an existing native endpoint with A, then A
   repeatedly extends native queried geometry. Guided entrance/exit landings use
   exact station/height context. Undo/restart and safe reviews remain in settings.
10. Return: settings B closes the menu and retains tool; live B ends construction.
    Start opens dashboard and owns only its own pause; returning must retain tool.

## Implementation plan

- Four semantic ride tabs: Overview (status/access/admission/locate), Operations
  (operating/dispatch/vehicles/maintenance/construction), Statistics (live native
  ratings/customer/finance summaries and direct detail routes), Settings
  (appearance/name/music/design/destruction). Tabs preserve independent focus.
- Keep native action dispatch and all eligibility/identity/query/confirmation
  guards. Project native rows to semantic tabs; never interpret visual index as a
  simulation action. L/R switches tabs only at tab root; drafts remain isolated.
- Direct path Build/Adjust modes: Y switches to Adjust; D-pad adjusts geometry
  without panning; B/Y returns to building, A changes slope/levels, X opens More.
  Select retains the surface grid. Build retains A placement, X removal/camera,
  L/R zoom; L2/R2 advertise connected slope and add free height adjustment.
- Shared tab rendering, measured content sizing for confirmations, focus memory,
  and context-aware construction hints. Keep the accepted slate palette/type.
- Validate real production menu dispatch, native accounting and cancellation,
  held-input handoffs, native saves and 512x384 component renders; report actual
  runtime/physical limitations and measured before/after sequences at completion.

## Delivered architecture and controls

`TabbedActions.h` owns independent tab focus. `RideTabs.h` projects the complete
native inspection action set into four semantic groups and maps selections back
to the original dispatcher indices. Native operations, drafts, query validation,
callbacks, accounting, save representation and destructive reviews are unchanged.
Statistics reads the existing native statistics presentation directly, with
Customers, Finances and Graphs one level below. New targets reset tabs; returning
from editors retains the active tab and row. Select still switches dashboard panes;
L1/R1 switches tabs only at the root, and retains existing detail paging elsewhere.

Shared management presentations now carry tab/status metadata, metric density and
content-sizing intent. Confirmations use a measured, narrower panel; sparse ride
rosters shrink to their content. Existing slate colours, fonts and focus remain.

Path controls use an explicit transient Build/Adjust state, reset on Begin/End:

| Control | Build | Adjust |
|---|---|---|
| D-pad | Move park/target | Left/right turn; up/down free height or connected slope |
| A | Place, remove, select endpoint or extend | Free flat/sloped toggle; connected flatten |
| B | End tool, preserving existing guided-return behavior | Return to Build |
| Y | Enter Adjust | Return to Build |
| X | Free remove toggle / connected camera | More (existing settings, undo, recovery) |
| Select | Native surface picker | Same; Back retains Adjust |
| L1/R1 | Existing zoom | Existing zoom |
| L2/R2 | Free height / connected slope | Same |
| Start | Dashboard | Dashboard, retaining tool mode |

Adjust consumes navigation before viewport panning or placement. Boundary actions
suspend held input and process only one action per frame. Geometry drafts still
pass through existing native path query and purchase rules. No slope rule or
placement restriction is bypassed. Surface selection and More remain usable from
either mode; their Back returns to the retained tool state.

## Representative interaction comparison

Counts are source/controller-driver traces, not physical-device timing. Ride
counts start at newly selected completed carousel's management root with default
focus (all optional native actions matching the fixture). Confirmation counts
include Cancel-first review. Each D-pad direction below is one tap. L/R means
L1/R1. Focus memory can reduce subsequent visits further.

| Task | Before | After | Presses |
|---|---|---|---|
| Open | Right, Down, A, Up, A | A, Up, A | 5 → 3 |
| Enter admission editor, where native park policy permits | Down ×3, A | Right, Down, A | 4 → 3 |
| Operations | R(page), Right, Down, A | R(tab), A | 4 → 2 |
| Maintenance | R(page), Down ×2, A | R(tab), Right, A | 4 → 3 |
| Read statistics | Down, A | R, R (values visible immediately) | 2 → 2; one fewer submenu |
| Open graphs | Down, A, Down ×2, A | R, R, Down, A | 5 → 4 |
| Free height +1, live tool | Y, Down ×2, A, B | R2 | 5 → 1 |
| Connected turn, then build | Y, Down, A, B | Y, Right, B | 4 → 3 |
| Four connected turns while adjusting | Repeated settings navigation | Y, Right ×4, B | 6 total; each repeat one tap |
| Select first surface | Select, A | Select, A | 2 → 2 |
| Repeated connected placement | A per native validated section | A per native validated section | 1 → 1 |

Ride roster entry remains Dashboard → Rides → roster A → selected ride A. Park
centre A retains the existing small quick-action card for access placement,
Open/Close and construction resumption; it is **not** the tabbed dashboard. This
legacy entry and the Build menu's path mode choice are deliberately preserved
for compatibility and remain candidates for the next interaction milestone.
Live path B ends construction; re-entry still uses Select → Paths → mode. The
common adjustment loop now stays live and no longer needs that exit/re-entry.

## Verification and capture interpretation

The authoritative scaling implementation remains the existing handheld launcher
configuration and renderer documented in `UI-REDESIGN-MILESTONE.md`: 512×384 logical
canvas and requested 2× presentation on the Brick's 1024×768 screen. This milestone
did not replace the renderer or change scaling. Physical output is unverified.

The complete UI target (160 translation units) was rebuilt with AArch64 GCC 16.2,
configured SDL 2.26.1 headers and ICU 76.1 headers against the recovered private
runtime libraries. GUI and CLI links passed. The unchanged native engine archive
is reused from the preceding verified build; this is not a fresh full-engine
rebuild. Final incremental rebuilds include every edited production dependency.

`UiRedesignTests.cpp` drives production menu/Explorer dispatch through synthetic
controller frames on the real AArch64 engine under QEMU. It checks tab mapping,
axis behavior, focus restoration, unavailable admission, native statistics,
Cancel-first demolition, direct connected turn/reverse/slope/flatten, free height,
no Adjust placement/pan, surface/More return, Build Back, mode reset, native guided
landing query/cost, and undo cancellation. Input state is initialized to normal
as a game loop would; duplicated frame actions exercise handoff isolation.
`PresentationLifecycleTests` additionally uses SDL virtual gamepads for held-input
and draft handoffs, native setting actions and save/reload preservation.

Captures in `captures/controller-ux-phase2/` are **real native X8 component renders**
at 512×384 from that production dispatcher/fixture, not designed mockups and not
full-park or physical screenshots. Public G2 font/icon assets and public objects
are available; proprietary world sprites are absent, so the backdrop and native
surface thumbnails are blank. Inspected: all four tabs, active free paths, height,
free/connected Adjust, surface picker and content-sized destructive review.
Text, focus, action availability, tab/metric/action separation, footer fit and
geometry visibility fit the logical canvas. Surface labels ellipsize intentionally;
full selected name is available in the picker.

## Remaining work and migration

- Unify the park's legacy quick inspection with tab management while retaining its
  fast access/construction routes and its Back-to-park contract. Add explicit
  entry-context restoration rather than treating every entry as a roster visit.
- Test on Brick/NextUI with original assets: physical mappings, held D-pad repeat,
  actual park visibility, surface thumbnails, pause/resume and crisp 2× output.
- Exercise long localized ride names, heavily populated rosters, and more coaster
  families. The adapter currently keys on existing English action labels, as the
  dispatcher already does; stable semantic action IDs are a useful future cleanup.
- Port staff/guest/park workspaces to the shared tab/status metadata where their
  information architecture warrants it. Keep native editors and validation.
- Consider contextual help exposing Select/Start and L/R in Adjust; the compact
  footer prioritizes geometry, safe return and More. Bindings remain available.
- Installation ZIP/device packaging is not produced by this source milestone.

Final result: new UI driver PASS; all 12 preservation drivers PASS (see
`evidence/controller-ux-phase2/preservation-results.json` and individual logs).
GUI/CLI QEMU `--version` PASS. `git diff --check` PASS. Runtime warnings from the
offscreen SDL display renderer remain; component rendering uses native X8 directly.
Build/version/test logs and SHA-256 hashes of binaries, retained engine archive
and captures are recorded alongside the report. No claim of physical-device or
full-park visual verification is made.
