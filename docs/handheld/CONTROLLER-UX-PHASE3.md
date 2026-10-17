# Phase 3: scenery and landscaping controller UX

Baseline: canonical `078bd9e082fa16817f55b2a2c0ceeb1e2cfd2fac`, tree
`e9f3d587eee6ed8141059a9a1e510e5c19f23d2f`. Verified before editing; no newer
canonical work existed. Recovered the published Phase 2 source into this older
workspace before implementation. Existing source was backed up. Historical
capture/evidence files absent locally remain referenced unchanged in the canonical
base tree; publication overlays only the explicitly changed/new files.

## Source-based interaction audit

The actual baseline binds D-pad to park movement, X to camera rotation, L1/R1 to
zoom and L2/R2 to scenery height. Both Y and Select open the same scenery options.
Rotation has no direct live shortcut: it is options row 2. Colours occupy rows
7–9, mode row 10, eyedropper row 14 and scatter rows 16–18 of a twenty-row menu.
The native category/catalog, groups/search, picked-object identity, previews,
scatter review/results and sign/style workflows are already implemented.

Area tools use D-pad target movement, A native action/review, X camera, L/R zoom;
Y and Select both open area options. Width/height are rows 2/3, selection row 4,
surface/edge rows 5/6 and colours rows 7/8. Simple land/water Y toggles raise/lower;
quick-surface Y promotes the current material into area painting and opens options.
These repeated settings visits interrupt the construction loop. Ordinary menu B
closes options and retains the tool; live B ends it. New bindings must retain
native action validation, camera access and each tool's existing cancellation.

## Delivered design and interaction

`ConstructionAdjust.h` supplies compact contextual adjustment panels, shared
transient group/row focus, availability styling and mode-aware action strips.
Only UI state is new. Existing `SceneryTools::Settings` and `AreaTools::Settings`
remain the authoritative tool drafts. Begin/End reset adjustment UI; returning
from the catalog, material picker, More or dashboard retains the current adjustment
context. No new gameplay state or serialization is introduced.

Scenery has Object, Colour, Mode and Scatter groups. Groups use two readable rows
of segments instead of squeezing four labels across 224 pixels. Object controls
retain orientation, native small-scenery quadrant and automatic-height reset;
triggers retain direct height adjustment. Native object flags disable automatic
rotation and unsupported colour channels. Colour values, placement/recolour/remove/
eyedropper modes, scatter enable/size/density use the existing Configure path.
The expanded twenty-row menu remains available for search/groups, sign editing,
style, advanced controls and scatter diagnostics.

The live scenery panel identifies the object with its native Object::DrawPreview,
name, mode, orientation/quadrant, height/scatter state and contextual hints.
Eyedropper keeps the existing hovered-object/selection overlay. The catalog itself
retains its native sprite grid, research eligibility, identity, search and groups.
Direct catalog Back returns to the active adjustment context; existing category
and advanced-menu entry routes retain their previous parent screens.

Terrain has Brush (operation and independent dimensions), Shape (native full-tile/
corner/edge selection), and Paint (surface, edge, supported colours) groups.
`TerrainMaterials.h` presents loaded native surfaces/edges in the existing visual
picker using native icon sprites. It includes Keep unchanged. Selection revalidates
type, exact loaded index and descriptor before updating the existing paint draft,
resets that material’s native colour while preserving the other material’s colour,
and selects Paint as the current operation. It
never purchases or paints on selection. A in the park retains the existing paint
review and native accounting. Cancel leaves the material unchanged.

Simple land/water Y now enters Brush adjustment with the matching current operation.
Quick-surface Y enters Paint adjustment with its native selected material; Select
opens the material grid. Simple land/water L2 selects Lower and R2 selects Raise,
keeping the common operation switch direct before promotion to area tools. Other
construction families, rights tools, area clearing and their controls are unchanged.
Area L2/R2 cycles the existing seven operations in native tool order; operation and
brush/material are shown in the overlay. Existing native slope selection, smooth,
water, paint and ownership validation remain in AreaTools.

| Control | Scenery Build | Terrain Build | Adjust |
|---|---|---|---|
| D-pad | Move target | Move target | Up/down field; left/right value |
| A | Place/recolour/remove/pick; existing scatter review/apply | Native operation; existing paint review | Return to Build without placement; Paint surface/edge opens picker |
| B | End tool | End tool | Return to Build, retain tool |
| Y | Enter Adjust | Enter/promote to Adjust | Return to Build |
| X | Rotate camera | Rotate camera | More: existing expanded options |
| Select | Rotate selected object where native flags allow | Surface material picker for area tools | Scenery catalog; terrain surface/selected edge picker |
| L1/R1 | Zoom | Zoom | Previous/next group |
| L2/R2 | Existing height shortcuts | Previous/next existing operation for area tools | Same as Build |
| Start | Dashboard | Dashboard | Dashboard; retain adjustment context |

Paths retain their Phase 2 controls. No physical button binding was added or
changed in ControllerInput. Adjustment consumes D-pad before park movement and
processes at most one boundary action per frame, suspending held input. A used to
leave Adjust cannot also place; B used to leave Adjust cannot also end the tool.

## Representative before/after sequences

Counts begin in active placement with fresh/default focus; D means one D-pad tap.
More still offers all previous functions. These are source/controller traces,
not a timed physical-device benchmark. Retained group focus improves repeat visits.

| Task | Before | After |
|---|---|---|
| Rotate once | Y, Down x2, A, B (5) | Select (1) |
| Change primary colour once, resume | Y, Down x7, A, B (10) | Y, R1, Right, A (4) |
| Change mode once, resume | Y, Down x10, A, B (13) | Y, R1 x2, Right, A (5) |
| Enter eyedropper | Y, Down x14, A (16) | Y, R1 x2, Left, A (5 from Place) |
| Enable scatter, resume | Y, Down x16, A, B (19) | Y, L1, Right, A (4) |
| Width +1, resume (area tool) | Y, Down x2, A, B (5) | Y, Down, Right, A (4) |
| Width and height +1, resume | Y, Down x2, A, Down, A, B (7) | Y, Down, Right, Down, Right, A (6) |
| Select next area operation | Y, Down, A, B (4) | R2 (1) |
| Surface change | Y, Down x5, value cycling, B | Select, visual grid movement, A |
| Return from settings | B to live tool | B from More to Adjust, then B/A/Y to Build |

While Adjust stays open, each further value change is one left/right tap; there
are no repeated menu transitions. Normal placement remains one released A per
native operation. The new explicit return from More has one extra step, reserving
More for less frequent settings while preserving its caller's adjustment context.

## Validation and provenance

The existing 512x384 logical / requested 2x Brick rendering configuration is
unchanged. AArch64 UI translation units are rebuilt against restored GCC 16.2,
configured SDL 2.26.1 and ICU 76.1 headers, using the retained unchanged native engine
archive and private runtime libraries. GUI and CLI links/version checks and actual
QEMU driver results are recorded under `evidence/controller-ux-phase3/`.

`ConstructionUxTests` drives production Explorer/Menu dispatch with duplicated
semantic frame actions plus a mapped SDL virtual controller for held Y/B handoffs.
It checks direct rotation/colour/mode/scatter controls, per-group return from More
and catalog, no adjustment pan/purchase, native wall cost, brush width/height,
material selection, cancel and stale identity rejection, native area height/cost,
and tool boundary handling. Native preservation drivers retain their existing
simulation/save tests. The prior quick-surface parity assertion is updated for
Paint Adjust plus X More instead of expecting Y to open the legacy options.

Captures are native 512x384 X8 production UI renders driven by the runtime fixture,
not mockups or physical-device screenshots. Public fonts/object metadata are
available; proprietary scenery/terrain/world sprites are not. Native thumbnail
and ghost rendering paths are preserved, but thumbnails/world are blank in these
captures and cannot establish original-asset or physical rendering correctness.
Scenery object, colour, mode, scatter, eyedropper; terrain brush, Paint, both material
pickers and return to Build are included. Groups, focus, disabled states, hints,
panel footprint, text and footer spacing were inspected at logical resolution.

## Remaining limitations / next milestone

- Test on physical Brick/NextUI with original assets: thumbnails, ghosts, material
  icons, held repeat, park visibility, actual mappings and crisp 2x output.
- Advanced settings still use existing text menus, including groups/sign style,
  detailed scatter output, brush settings via More, area clear and rights options.
- Adjust intentionally uses D-pad as a field/value editor; it cannot pan the park
  until returning to Build. Camera rotation/zoom are available in Build; shoulders
  are group navigation in Adjust. These meanings are shown explicitly.
- Automatic rotation follows native small-scenery flags; unavailable fields remain
  visible and disabled. Unsupported actions are not approximated.
- Catalog/surface grids retain the existing six-item paging and selected-name
  ellipsis. Large catalog and long-localization physical testing remains pending.
- Separate legacy simple-terrain/surface tools remain entry points; Y promotes
  them into the new controls. Rights/clearing have not received this redesign.
- No installation ZIP is produced. Recommended next milestone: staff and guest
  roster/detail composition, focusing on hiring, orders, patrol access and needs.

## Reproduction

Build the current `openrct2` GUI target first, then run:

```sh
python3 scripts/handheld/verify-ui-redesign.py --driver ConstructionUxTests \
  --build /absolute/ninja-build --data /absolute/runtime-data \
  --profile /absolute/test-profile --captures /absolute/captures
```

For AArch64 cross builds, add `--runner-prefix` containing the matching QEMU,
loader and library-path arguments. The wrapper takes compile definitions/includes
and GUI object/link arguments from Ninja and removes only the GUI main object.
The existing UiRedesignTests default is preserved. This session used the equivalent
retained compile-command manifest because its original Ninja/SDK paths disappeared;
all UI sources were rebuilt and all drivers linked against the final GUI objects.

Final verification: all 13 preservation drivers PASS against the final GUI
objects, plus ConstructionUxTests PASS including mapped SDL held-Y/B handling.
All 160 UI translation units rebuilt; final changed-dependency GUI/CLI links and
both QEMU version checks PASS. Staged whitespace check PASS. Twelve individual
512x384 component renders were inspected; final rerun renders exactly match the
recorded PNGs. Build/runtime/test logs and SHA-256 manifests are committed with
this report. Original sprite warnings and offscreen SDL renderer warnings are
recorded and do not constitute physical device or original-asset validation.
