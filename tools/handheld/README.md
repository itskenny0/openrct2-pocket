# Native handheld shell previews

`src/openrct2-ui/handheld/Panel.h` owns the shared palette tokens, cut-corner frame,
icon rows, tabs, detail cards and controller footer. The working park, terrain,
save/load and confirmation pages all call `DrawMenuShell` with a presentation model.
The existing controller actions, pause handling and atomic save logic are unchanged.

The shell is 494 x 342 logical pixels and fits the Brick's 512 x 384 screen.
Rows retain 32-pixel spacing. Labels are ellipsised within their own columns.
Stable default-palette indices avoid cycling water colours; native G2 tree and
terrain icons work without proprietary RCT2 graphics. Palette colours and sprite
fonts constrain the result compared with an RGB concept poster.

## Capture without installing on the Brick

With a completed native **Ninja GUI build**, matching OpenRCT2 graphics data and
Python Pillow installed:

```sh
python3 tools/handheld/render_shell.py \
  --build /absolute/path/to/native-build \
  --data /absolute/path/to/share/openrct2 \
  --output /absolute/path/to/shell-preview.png
```

Use `--ninja /absolute/path/to/ninja` if needed. The native build must use the
sprite font (`DISABLE_TTF=ON`), as does the Brick package. The helper reuses the
existing GUI link command, substitutes `test/handheld/PanelPreview.cpp` for the
application entry point, and invokes OpenRCT2's software drawing engine. It
captures two unscaled 512 x 384 frames on one labelled contact sheet.

These are actual component renders with fixture park information, not loaded
park screenshots or device captures. No Brick installation is required. The
park backdrop and future ride/path/management screens are not represented by
this capture. A live park preview and richer tool-specific detail content can
be added within the reusable cards later.

## Validation for this checkpoint

Native GUI build completed. Existing handheld input, launcher and save-preservation
CTest suites passed. The preview was visually inspected at native resolution.
This shell revision has not yet been built or tested on the physical Brick.
