# Phase 4: connected ride interaction and construction

Baseline: canonical `handheld/modern-ui-paths` at
`5e1539d7dae84fd8269d0bddd327a71b5170461d`, tree
`de0eb3654caf4b6358d448e914df1411ac7c94e0`. Source and Phase 2/3 documents were
inspected before implementation. The older workspace was reconciled against that
exact tree; publication overlays only the changed/new files, retaining historical
captures and evidence absent from the local checkout.

## Source audit and resulting changes

| Journey | Baseline source behavior | Delivered behavior |
|---|---|---|
| Park centre A on a ride | `HandheldRideOpen` chose legacy `Page::ride`, while the Dashboard roster chose modern `ridesInspect` | Both choose the existing four-tab workspace; shops retain their specialized interface |
| Status and inspection | Park quick card exposed only status/access/construction; full management required leaving and finding the ride in the Dashboard roster | Overview exposes the established Open/Close/Test reviews, admission and Locate; Operations, Statistics and Settings remain unchanged |
| Access | Completed access handed off to another optional-action list; quick access and full management were disconnected | X at any ride tab opens existing access points, or the missing-access handoff; completed placement returns to the modern ride workspace |
| Locate | Existing action changes native camera position and closes the panel | Preserved; opening a ride alone never relocates the camera |
| Resume construction | Multiple callers closed the panel and began a tool without recording their origin | Shared launch helper records copied ride identity, tab focus, park/roster origin and roster row; failed launches reopen management |
| Coaster Back | Undo in Build; camera Back could invoke Undo; station/replacement exit lacked consistent return | Native Undo retained in Build; camera B returns to Build; station/replacement cancellation returns to management |
| Coaster piece catalog | Back closed the catalog; explicit Return to park discarded management origin | Back still resumes building; explicit Return to ride restores management; Start does the same from the catalog |
| Flat ride/access | Back ended the tool; completed entrance/exit opened the optional-action list | B cancels unplaced preview to park, or returns an existing ride to management; completion returns to management |
| Maze | Back cancelled review or left the builder, including camera mode | Review B cancels review, camera B returns to Build, Build B returns to management |
| Guided access paths | Back opened the optional access menu | Back and Start return to the originating modern workspace |

The standard park ride entry no longer reaches the old quick card. Its unused
legacy code remains for now; no new intermediate inspection menu was added. Direct
entrance/exit point inspection remains useful and retains native point validation,
relocation and connected-path actions. No native gameplay state or serialization
was added.

## Controls and return rules

| Context | Controller behavior |
|---|---|
| Ride workspace | D-pad action grid; A select/review; L1/R1 tabs; X contextual access when supported; Select section rail; B returns to park or the originating roster |
| Operations/details/reviews | B returns to the owning tab and remembered action; confirmation retains Cancel-first focus |
| Existing ride construction | Start returns to its ride workspace, including from track-piece browsing; no demolition or refund occurs on leaving |
| Coaster Build | A native build, B native Undo, L1/R1 piece, L2/R2 slope, X bank or speed+, Y chain/turn or speed-, Select piece catalog |
| Coaster camera | Existing catalog Camera controls entry remains reachable with D-pad/A; B restores Build, L1/R1 zoom and X camera rotation |
| Coaster station/replacement | B cancels the current placement workflow and returns to the ride; native partially built/removed geometry remains authoritative |
| Flat/access placement | Existing D-pad positioning, A placement, Select orientation, triggers/shoulders retain their native roles; B/Start return to the existing ride; unpurchased flat B cancels to park |
| Maze | A build/select/review, X move, Y removal review, L1/R1 direction, L2/R2 height, Select camera; B cancels review, then camera, then returns to the ride |

Start+Select retains the existing global quit chord. No new physical button or
analog input is required. The existing left-stick camera action is optional;
coaster camera controls are also available in the piece catalog. Duplicate action
frames and transition suspension are exercised by the new driver.

`RideNavigation.h` is UI-only transient context. Each successful management-to-tool
handoff stores a copied `RideManagement::Target`, per-tab rows and origin. Begin/End
clear transient bookmarks; return takes a copy before ending construction and
revalidates native identity before opening management. There is no stored Ride*
and no new simulation state. Returning from park preserves the camera; explicit
Locate and construction positioning retain their original behavior.

## Representative sequences and counts

Counts are discrete presses, starting from the stated context; D-pad repeats and
roster search are variable. Native action reviews are intentionally retained.

| Task | Before | After |
|---|---|---|
| Park selection to full Overview | A quick card, then leave it, open Dashboard, select Rides and find the same ride; variable presses and another selection | A: **1** |
| Completed ride access from park | A quick card, A first Entrances/exits action: **2** | A Overview, X Access: **2**, also X from any tab |
| Incomplete ride access from park | A quick card, A Place entrance/exit: **2** | A Overview (Place focused), A: **2** |
| Existing coaster to its workspace | Start opened Dashboard; return required finding the ride again | Start: **1**, restores the originating tab/action |
| Piece catalog to management | B to building, then Dashboard/roster search; or Return to park then reselect | Start: **1**, or catalog Return to ride |
| Camera back to building | Coaster B invoked Undo; maze B left the tool | B: **1**, no native action or charge |
| Operations detail and return | Modern roster route already supported remembered focus | A detail, B: **2**, now available from park selection too |
| Open a completed ride from park | Quick card A, Down to Open, A: **3**, without modern review | A Overview, A review, Up Confirm, A: **4**, preserving the accepted safe native review |

The improvement is continuity and fewer searches, not elimination of native safety
reviews. Existing access paths, advanced track variants, block editing, demolition,
special pieces, banking, slopes, ghosts and costs remain reachable.

## Construction presentation

The established slate-green system, tabs, status cards, action grids and compact
confirmation layout are preserved. Coaster live controls now include the selected
piece's existing native descriptor footprint diagram, direction, endpoint height,
slope/bank and explicit Start/Select hints. Important geometry labels use brighter
text. The panel ends before the viewport centre reticle. Flat/access and maze
panels explicitly state their return and camera controls. Picker Start hints now
accept caller context; track pieces say Ride instead of Pause. Workspace X hints
are shown only where access is supported.

Track catalog miniatures are intentionally native descriptor footprint diagrams.
They are not missing original sprites. In-world track and placement ghosts use the
native renderer and original assets.

## Assets and reproduction

The user's uploaded Data, ObjData, Scenarios and Tracks archives were extracted
privately outside the repository. `Data/g1.dat` (16,942,164 bytes) is present and
was loaded successfully. Native grass, carousel/ghost, coaster and maze graphics
are visible in the Phase 4 captures. Original binaries are neither downloaded
from third parties nor included in this commit.

Use a case-correct installed layout:

```
/private/RCT2/
  Data/g1.dat
  ObjData/*.DAT
  Scenarios/*.SC6       (optional for the initialized fixture)
  Tracks/*.TD6         (optional for the initialized fixture)
```

Keep matching public OpenRCT2 engine data and the public object repository in the
separate engine-data location. A full original installation is recommended for
ordinary play, including audio and expansion assets appropriate to that install.
Runtime path configuration is already supported: `--rct2-data-path=/private/RCT2`
and the existing `[general] game_path` setting. See BUILDING.md for launch arguments.
The driver supplies the same native platform/config path before initialization.

Reproduce against an existing built GUI target:

```sh
python scripts/handheld/verify-ui-redesign.py --driver RideFlowTests \
  --build /absolute/build --data /absolute/openrct2-data \
  --profile /private/test-profile --rct2-assets /private/RCT2 \
  --captures /absolute/captures
```

For an AArch64 build, supply `--runner-prefix` with QEMU and its matching target
loader/library arguments. `POCKET_RIDE_FLOW_CAPTURES` is set by the script. This
driver initializes a native owned-land scenario fixture, buys native rides and
access, exercises production menu/input dispatch, and renders the actual native
viewport plus production UI to indexed PNGs. It is not a mockup or a capture of a
commercial scenario session.

The staging config still requests 1024×768 output, window scale 2, DPI inference
off. `UiContext::Resize` derives the logical canvas by dividing by windowScale;
integer scaling selects nearest-neighbour filtering. Captures explicitly render
512×384 at scale 1. Physical display mode, crisp 2× presentation and device timing
still require Brick verification.

## Verification and limits

See `evidence/controller-ux-phase4/verification.json` for build provenance and
driver results. All 160 GUI translation units were rebuilt for AArch64, and GUI
and CLI linked against the retained unchanged native engine archive. A later
incremental rebuild includes final shared hint and navigation refinements. This
is not a fresh rebuild of every engine translation unit.

The new asset-backed RideFlowTests checks native park-centre hit selection,
Overview/Operations/Back, camera preservation, missing-access cancel/resume,
completed access, native Open/Close, Cancel-first status/demolition review,
coaster build/Undo/refund/slope/piece catalog/camera, Start from piece catalog,
explicit Return to ride, continued construction and focus restoration, flat
preview cancellation and exact purchase cost, maze build/camera/leave, and
roster-to-builder-to-roster focus.

Seventeen preservation drivers cover pause/resume, access shortcuts/relocation,
maze access previews, scenery/path parity, construction, native actions/accounting/
save-reload, presentation lifecycle, maze/ride construction, path scenarios,
ordinary UI, shared redesign, Phase 3 construction UX, selected-track editing,
seat rotation and family operations. These are real AArch64 executables under
QEMU. The final UI renders are inspected at 512×384 with the supplied assets.

The headless SDL fixture reports its expected hardware-renderer Resize warning;
PNG capture uses the software X8 renderer and actual native viewport. No visible
interactive desktop session or physical TrimUI Brick test was performed. Crowded
commercial parks, long translated ride names, physical input timing and GPU/output
scaling remain manual playtest coverage. Resuming a ride uses native endpoint
selection, not a newly serialized construction cursor. Already removed replacement
blocks retain the existing native recovery behavior; returning does not silently
rebuild or refund them.

## Next milestone

Unify staff and guest selection with their management workspaces. Prioritize a
compact staff roster with role/status filters, direct assignment/patrol handoffs
and return focus; then readable guest needs/thoughts and context-preserving follow/
locate/filter flows. Audit actual bindings and existing native actions first, and
retain the established visual and confirmation components.
