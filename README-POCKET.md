# OpenRCT2 Pocket — TrimUI Brick fork

Controller-first presentation and input on top of OpenRCT2, targeting a 512×384 logical canvas. Preserve the original park renderer, simulation, game actions and park format. Desktop mode remains the default; enable the experimental layer with `OPENRCT2_HANDHELD=1`.

This repository retains the upstream sources, history, contributors and GPLv3 licence (`licence.txt`). Original RollerCoaster Tycoon installation assets are user-supplied and must never be committed or distributed here.

## Checkpoints

- `brick-explorer-proof1`: source `b4c220d` identified by the previously delivered ARM build. The user reports stable readable park rendering, smooth D-pad pan, zoom, rotation, inspection and back on a physical Brick.
- `pocket-menu-native-0.2`: restored controller-menu source checkpoint. Native Linux build/input tests only; **not a new verified Brick release**.

See [current validation and continuation notes](docs/handheld/CHECKPOINT-2026-10-06.md). Earlier reconnaissance and build records remain in `docs/handheld/`; their validation dates and scopes matter.

## Current controls

| Context | Controls |
|---|---|
| Park exploration | D-pad pan; A inspect at reticle; B close/cancel; X rotate; L1/R1 zoom |
| Title | D-pad choose; A activate; B exit or close overlay |
| Scenario list | Up/down choose unlocked scenario; L1/R1 change source tab; A start; B back |
| Pause menu | Start opens; up/down choose; A activate; B/Start return |
| Emergency quit | Start + Select, including staggered presses; exits **without saving** |

These A/B labels assume `OPENRCT2_HANDHELD_SWAP_AB=1`, as set by the NextUI launcher to correct the observed SDL inversion. Without that override, SDL A confirms and SDL B backs. Physical mappings remain replaceable; L2/R2 and additional distinct buttons are not assigned in this slice.

Options and toolbox screens remain desktop workflows. The terrain/save slice adds a native Build panel and three save/load slots; other save files still use the legacy browser. Native path construction, dashboard and finance screens are not implemented.

Do not replace the currently working Brick executable or launcher with a native x86-64 binary or this menu-source launcher alone. A matching ARM build must be produced and audited first. Keep original assets, saved parks and `save-handheld/` when updating.

## Terrain/save slice

The next source slice adds a blue/yellow shared controller panel, Select → Build → raise/lower terrain, a selected-tile outline and contextual command strip, and three native save/load slots with Save and exit. See [terrain/save controls and validation scope](docs/handheld/TERRAIN-SAVE-SLICE.md). A matching ARM build and physical tests are still required; this is not yet a Brick release.
