# Native handheld UI development

`src/openrct2-ui/handheld/Panel.h` provides rounded overlay frames, quiet icon
rows, gold focus, controller badges, information pages and the gameplay footer.
`ModernText.h` draws the embedded Pocket Sans coverage atlas in the native
software renderer, blending against the current background through stable
palette ramps. Unicode/complex formatted strings fall back to native sprite
text. See `PocketSans.LICENSE`; `generate_font.py` regenerates the renamed atlas
from DejaVu Sans. No TTF runtime is required on the Brick.

Pause/build and editing panels occupy the left side of the live park viewport.
Editing uses separate small, input-transparent HUD windows for the footer, tool
panel and cost badge. Window bounds resize on menu page changes. The rendered
colour range remains constrained by the native indexed palette.

## Capture without installing on the Brick

With a completed native Ninja GUI build, matching graphics data and Pillow:

```sh
python3 tools/handheld/render_shell.py \
  --build /absolute/path/to/native-build \
  --data /absolute/path/to/share/openrct2 \
  --output /absolute/path/to/overlay-preview.png
```

Use `--ninja /absolute/path/to/ninja` if necessary. The helper derives compile
settings and the GUI link command from Ninja, replaces the application entry
point with `test/handheld/PanelPreview.cpp`, and calls the real UI components.
Three unscaled 512 × 384 frames show terrain, paths and path options with sample data.

An optional `--background /absolute/path/to/park-image.png` fits a park image to
512 × 384 and quantises it before drawing. Output explicitly labels this a
backdrop composite with sample data, not a live game or device screenshot.

## Native integration checks

```sh
python3 tools/handheld/run_native_test.py \
  --build /absolute/path/to/native-build \
  --data /absolute/path/to/share/openrct2
```

This runner links `test/handheld/PathActionsTests.cpp` against the completed GUI
build and uses an isolated temporary profile. Real path objects are required in
the data directory. It verifies native placement, queue conversion, save/reload,
removal, ownership, pause rules, elevated sloped paths, fixed height across tiles
and research funding. Compile definitions and
include paths are inherited from the real build to preserve context ABI.

See `docs/handheld/MODERN-PATHS.md` for implemented features, device controls,
remaining work and the validation boundaries.

For individual 512 × 384 captures, add `--frames-dir /absolute/path/to/captures`.
See `docs/handheld/UI-REFINEMENT.md` for the compact tool layouts and validation.

The authoritative Concept B styling and current validation are documented in
`docs/handheld/CONCEPT-UI.md`, superseding the sparse tool-layout pass.
`ConceptStyle.h` owns its shared primitives; generate the bold atlas with:

```sh
python3 tools/handheld/generate_font.py /path/to/DejaVuSans-Bold.ttf \
  src/openrct2-ui/handheld/PocketSansStrongData.h --namespace PocketSansStrong --width 16
```
