# Native handheld UI previews

`src/openrct2-ui/handheld/Panel.h` owns the palette tokens, rounded frames, quiet
icon rows, tabs, detail cards, controller badges and gameplay footer. Menus now
use `DrawOverlayShell`; the larger `DrawMenuShell` remains available for future
information-heavy screens. The common drawing components share the same style.

## Park-first overlays

Pause is 244 x 278 at the left of the 512 x 384 logical screen. Build and slot
selection shrink to 244 x 182. Confirmation pages widen to 330 pixels so explicit
save/discard wording fits. Window bounds, frame widgets and position resize when
changing page. Transparent, background-free windows let the live main viewport
show around the rounded edges; no screenshot is inserted into the actual game.

Active terrain editing adds a 184 x 164 tool panel at (8, 52), separate from the
34-pixel gameplay footer and 19-pixel centre reticle. It shows the current
raise/lower mode and the existing Y button to switch. The panel is an empty-widget,
input-transparent HUD window so map selection still hits the main viewport. It
uses HUD number 1; the footer uses number 0. Both are removed when exploration is
blocked. Keeping them as separate small windows avoids a full-screen overlay
redraw while panning.

D-pad movement, centre tile selection, terrain queries/actions, pause semantics,
atomic save handling and default-to-Cancel confirmations retain their existing
behavior. Save failure status takes precedence over normal confirmation hints.

Rows retain 32-pixel spacing. Labels clip within their own columns. Stable
palette entries avoid cycling water colours. Native G2 artwork works without
proprietary RCT2 graphics. Palette colours and sprite fonts still constrain the
result compared with the RGB concept poster.

## Capture without installing on the Brick

With a completed native **Ninja GUI build**, matching OpenRCT2 graphics data and
Python Pillow installed:

```sh
python3 tools/handheld/render_shell.py \
  --build /absolute/path/to/native-build \
  --data /absolute/path/to/share/openrct2 \
  --output /absolute/path/to/overlay-preview.png
```

Use `--ninja /absolute/path/to/ninja` if needed. The native build must use sprite
fonts (`DISABLE_TTF=ON`), as does the Brick package. The helper reuses the existing
GUI link command, substitutes `test/handheld/PanelPreview.cpp` for the application
entry point, and invokes OpenRCT2's software renderer. Two unscaled 512 x 384
frames are captured on one labelled contact sheet: pause and active terrain.
The capture calls the same overlay, footer and reticle components as gameplay.

An optional `--background /absolute/path/to/park-image.png` fills the background
for layout review. This image is fitted to 512 x 384 and quantised to the native
palette before drawing the UI. The output is labelled a **backdrop composite**.
It is not a live game screenshot or device capture. Without the option, the
background is blank. The displayed terrain cost is fixture data; gameplay uses
the actual native land action query. A manually linked preview executable also
accepts `build` as its page argument to inspect the tool-picker overlay.

## Validation for this checkpoint

Native GUI build completed. Existing handheld input, launcher and
save-preservation CTest suites passed. Native overlay captures were visually
inspected at 512 x 384. This revision has not yet been built for AArch64 or tested
on the physical Brick. The supplied review image composites native UI with an
unobstructed park crop from the user's Concept B reference.
