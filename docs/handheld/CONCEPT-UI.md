# Concept B presentation system

The user-supplied OpenRCT2 Brick UI Concept B is the visual specification. This
pass supersedes the deliberately sparse layouts in UI-REFINEMENT.md, and remains
limited to the existing terrain/path slice. It does not add construction actions,
change controller mappings, or alter world sprites, simulation or the game palette.

## Inspection and reusable primitives

The earlier implementation differed from the concept in five central ways:
flat purple cards, one font weight, text-only rows, ungrouped option lists, and
unlabelled cost chips. Making the cards smaller had removed the concept's hierarchy.

`ConceptStyle.h` now owns the slice's reusable surface, section heading, icon-led
row, selected state, separators and layout tokens. `ModernText.h` provides distinct
regular and true bold text plus light, dark, muted, gold and positive-value inks.
The bold atlas is generated from licensed DejaVu Sans Bold, renamed Pocket Sans
Strong, and embeds into the binary without a device font dependency. It uses the
same licence as PocketSansData.h. The old unused sparse-tool primitives are removed.

| Reference feature | Native implementation |
| --- | --- |
| Left rail about one-third of the screen | 164 pixels out of 512, at (8, 8) |
| Blue-navy inset panels with quiet blue-grey edges | Solid deep-indigo surface, single restrained border |
| Gold active rows with dark, stronger text | 148 × 28 solid gold rows, true bold dark selected labels |
| Icon + label alignment | 16-pixel icons, 31-pixel text inset, consistent baseline |
| White section headings, lighter subordinate detail | Bold titles, muted section headings, regular rows and secondary state |
| Grouped controls | Terrain/Brush; Path type/Slope/Options; Height/Slope/Surface |
| Controller strip integrated along the bottom | 28 pixels, centered face-button lettering, D-pad cross, short labels |
| Compact cost context | Bordered 110 × 26 box, muted Cost label and green native queried amount |

Terrain displays both supported tools, with the active one highlighted, followed
by the existing single-tile brush. Path placement displays the selected path type,
loaded surface, current slope/height and existing options/remove controls. Two gold
rows express active configuration, as in the reference. Removal highlights Remove
and labels the hit path's height appropriately. Path options presents all seven
existing actions in groups with one controller focus. This restores the concept's
density while keeping roughly two-thirds of the park unobstructed horizontally.

Unsupported concept features such as wide paths are not presented as working
controls. The original X remove/place and Y surface mappings are preserved, so
footer labels describe the real behavior rather than copying incompatible labels
from the poster. The existing native map-selection highlight and centre cursor remain.

## Colour fidelity boundary

The indexed software renderer lacks the concept's exact dark navy and smooth RGB
gradients. This pass uses solid existing entries only: no dithering, stippling,
checkerboards, or artificial texture. `PocketPalette.h` centralizes the shared
palette tokens. Panel interiors and selected rows are constant-color fills;
rounded corners and a one-pixel border provide the only surface treatment.

| Role | Palette index | RGB |
| --- | --- | --- |
| Panel / action strip | 118 | 15, 19, 55 |
| Border / separator | 119 | 39, 43, 87 |
| Selection | 53 | 243, 203, 27 |
| Primary text | 21 | 239, 243, 243 |
| Secondary text | 18 | 159, 175, 175 |
| Selected text | 10 | 23, 35, 35 |
| Positive amount | 103 | 139, 223, 115 |

These are the actual default game palette entries, not requested RGB targets.
Text uses these exact endpoints at full coverage; antialiased glyph edges use
nearest existing palette entries. This is glyph coverage, not patterned panel
colour approximation. The hue is deep indigo rather than the reference's teal
navy. Non-ASCII/localized text still uses the complete native text fallback.

Terrain is 164 × 174; Path placement remains 164 × 278; Path options is 164 × 338.
Nonselected rows no longer each carry a separator; spacing and quiet section
headings provide grouping. The bottom strip uses a single top rule and solid
fill. All actions, focus indices and controller mappings are unchanged.

## Validation and captures

Native GUI build passes. All three existing CTest suites pass. Native path tests
pass placement, queue conversion, funds, fixed height across tiles, raised slopes,
rotation, save/reload, removal, ownership/pause rules and research funding.
Native component captures were inspected for raise/lower terrain, normal/queue
paths, manual/automatic slopes, removal, error feedback, and last-row menu focus.

The three delivered PNGs are each 512 × 384. They call the production UI primitives
with sample state and a reference park backdrop. They are native UI renders, not
physical Brick screenshots or a live simulation capture. No generated replacement
world art is used by the game. A fresh ARM package and physical Brick verification
remain pending.

Reproduce normal captures with `tools/handheld/render_shell.py --frames-dir DIR`.
Use `--pages terrain-lower paths-queue paths-remove paths-ground paths-error
path-options-last` to inspect the alternate states. This uses the same native
software renderer and components as the normal captures.
