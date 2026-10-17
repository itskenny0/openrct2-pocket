# Phase 7: Line and Fill path construction

## Canonical baseline and audit

Canonical branch `handheld/modern-ui-paths` was inspected at
`8824460cea0fcca2caa37d471b036b6b4d8e158e`, tree
`c803617b4d0c1597fe9e54a45e886a972b264a19`. The restored workspace initially
contained an older source snapshot and an unavailable Git alternate. Source and
current Phase 2–5 documents were restored and checked against canonical blob
hashes before editing. Historical captures missing locally remain in the base
tree; they are not deleted by this milestone. No Phase 6 implementation was
assumed from the milestone numbering.

Inspected production `Paths.cpp`, `Explorer.cpp`, `Menu.cpp`, `PathSurfacePicker.h`,
`ControllerInput.cpp`, native `FootpathPlaceAction`, `FootpathLayoutPlaceAction`,
`FootpathRemoveAction`, `GameActionRunner`, native map/terrain/finance APIs, and
existing path/preview/action tests. Important findings:

- Single uses the viewport centre, native terrain placement and optional fixed
  height/slope. Connected construction has an independent endpoint, direction,
  slope, guided access landing, callback guards and reviewed native removal undo.
- Build uses D-pad pan, A placement, X remove, Y Adjust, Select surface, L1/R1
  zoom, L2/R2 height. Adjust owns the D-pad, A slope, X More and B/Y Build.
  Connected Adjust additionally supports turn/grade. Start opens the dashboard.
- Surfaces come from loaded eligible native normal/queue/legacy objects. Existing
  surface selection already validates object identity and preserves the tool.
- Native ghosts are temporary single-tile actions. They must be removed before
  real queries/save operations. Multi-tile ghosts would interfere with queries,
  conversions and connection updates, so the new footprint preview does not
  insert provisional map elements.
- Both native path placement actions operate on **one tile**. Layout placement
  additionally supplies design edges; it is not a multi-tile transaction.
- The action runner re-queries at execution, applies native finance and connection
  updates, runs script hooks and queues UI actions. It offers no path transaction
  or native rollback. Save snapshots are not a gameplay rollback mechanism.
- Cancelling a directly opened surface picker went to the old options page.
  This unnecessary return transition was reproduced and corrected.

## Delivered behavior and architecture

`PathSelection.h` enumerates the inclusive selection footprint only. Line accepts
one tile, horizontal or vertical endpoints in the native tile grid. Diagonal
endpoints display an alignment rejection; no route is chosen. Fill enumerates an
inclusive rectangle. Both stop at **64 tiles** before allocating/validating a
larger footprint. Fill uses deterministic row order; Line uses increasing grid
coordinate order, regardless of which end was anchored first.

`PathBatch.h` owns a transient selection, native query results and sequential
execution progress. It is not saved gameplay state. Every tile uses
`GameActions::Query(FootpathPlaceAction)` with flat slope, the anchor's fixed Z,
selected native surface/railings and normal/queue/legacy flags. There is no custom
path price, connectivity, clearance, terrain, ownership or accounting formula.
The displayed quote sums native tile queries; the native finance API also checks
whole-selection affordability. Any known invalid tile or unaffordable total
prevents entering the build review. Failed selections do not advertise the sum
of their valid subset as a total quote.

`Paths.cpp` integrates the draft with Single/Build/Adjust, surfaces and height.
Single geometry is bookmarked when leaving Single and restored on return.
Line/Fill always use flat geometry; the whole draft can be raised/lowered through
the existing controls. Native actions decide whether the resulting elevation is
valid. Follow ground cancels the anchored selection. A successful build leaves
its mode/material/elevation active, ready for a new start. Existing connected
construction, guided queues, slopes, native ghosts, removal and undo remain.

The native renderer projects each selected tile's footprint through the active
viewport transform and zoom. Outlines remain visible over the park, with a dark
understroke, `+` new tiles, `=` existing paths, `X` rejected tiles, and labelled
`S`/`D` endpoints. These are **footprint previews**, not textured path ghosts.
Preview drawing is clipped outside the HUD/footer. The established slate palette
and fonts are retained. The compact tool panel shows mode, material, type,
dimensions/tile count and height; the contextual badge shows the native quote.
Selection rejection/result messages use a bounded, content-sized feedback panel
so longer native reasons remain readable. Existing screens keep their normal
feedback layout.

## Controls and return flow

| Context | Controls |
|---|---|
| Free path Adjust | L1/R1 cycles Single / Line / Fill; Up/Down or L2/R2 height; X More; Select surface; B/Y Build |
| Single Adjust | Existing Left/Right slope orientation and A flat/slope; Single geometry restores across mode changes |
| Connected Adjust | Existing turn/grade/flatten and shoulder zoom; no batch modes |
| Line/Fill Build, no start | D-pad moves the park target; A anchors; B exits; Y Adjust; Select surface; L1/R1 zoom; L2/R2 height |
| Line/Fill Build, anchored | D-pad moves destination; A opens valid selection review; B cancels selection and retains tool; other Build controls remain |
| Build review | A explicitly submits sequential construction; B returns to live preview; other input is consumed and cannot move the reviewed destination |
| Request pending | Input is consumed until native callbacks complete; there is no mid-purchase cancel claim |
| After completion/failure | Tool remains active, with result/count visible; A starts a fresh selection; B exits when no anchor remains |
| Direct surface picker | A selects and resumes the tool; B now resumes the tool directly, preserving mode/Adjust/anchor |
| Surface picker from More | B returns to More; its B resumes the tool |

Build X retains the existing remove toggle. Entering removal leaves the batch
mode and restores Single geometry; removal still uses the native removal action.
Start/dashboard remains available outside review/pending; it retains draft state.
No physical binding, Start/Select quit chord, analog or mouse dependency was added.
A frame consumes only one boundary action and suspends held input, preserving
purchase protection. Adjustment never pans or purchases.

## Transaction and failure policy — not atomic

The live quote is **not an atomicity guarantee**. Before any purchase, the player
sees the explicit review: **“Build sequentially? Stops on failure. Built tiles
remain / charged.”** This extra confirmation is deliberate.

Confirmation refreshes all native queries and whole-selection affordability. A
changed quote requires renewed review. Execution then submits one ordinary
native path action at a time, waiting for its actual callback before submitting
the next. Each action is revalidated, accounted and connected by the engine.
There is no direct map mutation, no `noSpend` real placement, no refund fabrication,
no removal-based rollback and no temporary save/load transaction.

If the park changes, a script rejects a later command, native capacity fails or
another runtime condition changes after confirmation, execution stops. The result
reports **“Stopped: N/M built”** with the native error. Completed tiles and their
native charges remain. There is no automatic retry; a fresh start is required.
This behavior is tested by changing later-tile ownership after confirmation and
before processing the queue: exactly the first tile remains, only its native cost
is charged, and all later tiles remain absent. A changed game-state identity also
cancels remaining commands. Nothing is silently described as complete.

A quote can change when earlier native placements remove/modify shared scenery or
adjacent paths. The runner's actual per-tile results and costs remain authoritative.
Existing paths can be natively updated; the tool does not delete/rebuild them.
Queue rectangles remain subject to the native queue connection behavior; this is
not automatic ride-access routing. The specialized connected/guided queue tool
remains available. Multi-tile atomicity and batch undo are **not** provided.
Native tile removal remains the explicit recovery route and has native costs.

## Responsiveness and safeguards

- Maximum 64 inclusive tiles in either batch mode. No thousands-of-tiles loop.
- Preview results cache by endpoints, material, cash, pause state and tick age.
  Unchanged selections are refreshed at most every eight simulation ticks, while
  destination/material/cash/pause changes invalidate immediately.
- Every confirmation forces fresh queries. Cached validation never authorizes a
  purchase without the runner's own native execution validation.
- No provisional tile insert/remove churn for batch previews. Drawing is O(64).
- Sequential commands run through the ordinary native queue/callback mechanism.
  The limit also bounds worst-case same-tick command work; desktop timing does
  not establish Brick frame rate.

## Before / after interaction examples

Counts below count A presses after choosing a tool and positioning the first tile;
D-pad panning is continuous pixel motion, so tile distances are not misreported as
physical button-tap counts. Mode entry is Y, R1 for Line (or L1 for Fill), B.

| Task | Single before | New batch workflow |
|---|---|---|
| Eight new straight tiles | Eight A purchases, moving through seven tile intervals | A anchor, move to end, A review, A build: **8 → 3 A presses** |
| Four-by-four empty plaza | Sixteen A purchases and traversal of every row | A corner, move to opposite corner, A review, A build: **16 → 3 A presses** |
| Another segment/rectangle | Repeat every individual placement | A fresh start, move endpoint, A review, A build; mode/material/height retained |
| Cancel a live proposed area | Individual purchases already changed park | B discards uncommitted selection; no map/cash change |
| Cancel direct surface browsing | Select, B to old options, B to tool | Select, B directly to retained tool: **3 → 2 presses** |

The new tools reduce purchases and, especially for Fill, eliminate row-by-row
cursor traversal. They do not introduce diagonal routing or automatic slopes.

## Verification

See `evidence/controller-ux-phase7/verification.json`, logs and capture README.
Clean Linux x86-64 GUI **and CLI**, native engine and UI were built from source
with GCC 13.3, SDL 2.30, ICU 74.2 and scripting enabled. This is a clean native
build, not reuse of the older retained AArch64 engine archive. A build-local
workaround demoted GCC's existing `picosha2.hpp` stringop-overflow false positive
for `ScenarioPatcher.cpp` only; no native engine source was modified. HTTP,
multiplayer, OpenGL, TTF and optional codecs were disabled for this test profile.
Generated G2/font/palette/track data match the recovered package byte-for-byte.
GUI/CLI version checks passed. Build banners identify the canonical baseline
because configuration preceded the milestone commit.

`PathBatchTests` links production UI objects and the real engine. It drives
production Explorer/menu dispatch with duplicated frame actions, native action
queues, save/reload and original assets. Coverage includes Single, all four
cardinal line directions, existing joins, repeated line/fill, queue line/fill,
full rectangle accounting, ownership/occupancy/uneven terrain/water rejection,
whole-selection funds, cancel/review, explicit partial-failure reporting and exact
retained charge, mode/Single geometry restoration, surface/More return, preview
cache, 64-tile bounds and native save/reload. See logs for measured desktop cache
and bounded queue timings.

Twelve existing gameplay/UI drivers and all five standalone controller/file/
launcher tests passed. Preservation covers native path actions, path usability,
scenery parity, pause, presentation lifecycle, access/relocation, construction,
ride flow and staff/guest workspaces. Python runner checks and targeted Git
whitespace checks passed.

The 15 captures and contact sheet were initially supplied separately. The user
explicitly authorized repository publication with Phase 6; those original PNGs
are now included, retaining the recorded hashes and original provenance. Raw
asset files are excluded.

The 512×384 captures are real **desktop offscreen native runtime fixture** renders
with the user's original G1 graphics and official objects. They include live Line
and Fill, review, completed paths, alignment, queues, obstruction, stopped result,
mode adjustment, native surface selection, existing paths, water/elevation and
selection bounds. They are not mockups, QEMU captures, a mature live park playtest,
or physical TrimUI Brick validation. Proprietary asset files are not committed.
No new AArch64 package or physical-device performance/input/scaling claim is made.

## Limitations and follow-up

- Physical Brick/NextUI validation, held-D-pad feel in long sessions, crowded parks,
  long translated labels and hardware timing remain outstanding.
- Footprint outlines do not show textured surface sprites. Material/type is named
  and the native surface picker still supplies native previews.
- Some selection tiles can pass behind the fixed contextual panel; see the polish
  backlog. Error text beyond four wrapped lines is ellipsised.
- There is no sloped batch, diagonal/polyline routing, bulk removal or atomic undo.
  These are separate proposals, not missing existing native functionality.
- The previously documented optional bright-sand capture failure was not used as
  an acceptance test or claimed resolved here. Grass and native water captures
  passed in this desktop runtime.
