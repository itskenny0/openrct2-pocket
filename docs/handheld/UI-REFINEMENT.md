# Compact handheld presentation pass

Targets: terrain, path placement and path options at 512 × 384. This changes
presentation and hierarchy while retaining the current action mapping and all
seven path-option actions.

## Layout

- Terrain and paths: one 164 × 94 panel at (8, 52). A small muted mode title sits
  above the prominent gold selection. Terrain shows only the active raise/lower
  tool and brush size. Paths shows the active surface and height/slope. No repeated
  control prompts or inactive tool rows occupy the panel.
- Bottom: one 28-pixel action strip, replacing the 34-pixel two-line footer.
  Button prompts have consistent spacing. Terrain retains A edit, Y switch,
  B done and D-pad move. Paths retains A place/remove, X mode toggle, Select
  options, B done and D-pad move; Y surface cycling and L/R zoom remain available.
- Context: the queried cost appears in an 88 × 24 badge without a repeated Cost
  label. Action feedback and invalid-target messages use the same context anchor
  in up to two lines, replacing the cost temporarily. No permanent status row.
- Path options: a 164 × 188 left panel displays four of the seven actions at once.
  The list follows focus with subtle scroll chevrons. D-pad navigation order,
  wraparound, A activation and B return remain intact. A single action strip sits
  at the bottom. The transparent modal drawing window covers the screen bounds,
  but paints only the small left panel and bottom strip; the park remains visible.
- Shared smooth text has a muted ink colour for titles and secondary information.
  Quiet panel edges and flat gold focus replace the more ornate tool cards. The
  existing indexed-palette software renderer and Unicode fallback remain.

## Validation

Native GUI build passes. Existing input, launcher and save-preservation CTest
suites pass (3/3). Native path integration checks pass: placement, queues, cash,
fixed height across tiles, elevated slope rotation, native save/reload, removal,
ownership and pause rules, and research funding.

Individual native component captures were inspected at 512 × 384 for terrain,
paths and scrolled path options. They use a reference park backdrop and sample
values, not a device capture or live game screenshot. The render helper's
`--frames-dir` option saves these individual PNGs alongside its QA contact sheet.

This pass has not yet been rebuilt for AArch64 or tested on the physical Brick.
The existing downloadable 0.7 package predates both this presentation pass and
manual path options. Next device check should include all seven option entries,
path removal, terrain switching, invalid-placement feedback, readability,
park visibility, and save/reload.
