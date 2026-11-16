# Native menu feel experiment

Branch: `experiment/native-menu-feel`, based on the published working checkpoint
`14d08a737119f81d01d656d585a49226d44fd68f`.

This trial changes menu presentation and feedback, following feedback that a
palette swap left the menus feeling alike. It keeps the stable slate palette,
existing pages, action ordering, controller bindings, focus restoration, native
object previews, and single-row footer. It does not implement a new quick-card
workflow or alter construction/quit/speed behavior. The earlier warm palette
trial remains separate on `experiment/friendlier-pocket-ui`.

## Changes

- Native RCT/OpenRCT2 sprites replace generic glyphs in Build categories and
  common actions: ride, path, terrain, scenery, locate, open/close/test, save,
  entrance/exit, vehicles, music, appearance and demolition.
- Illustrated management tabs use their native ride, gears, graph, guests,
  finance, research, marketing and staff animation sequences. Only the selected
  tab/sidebar icon animates, at one sprite frame per 120 ms.
- Action buttons have a raised face and lower edge. Focus adds a mint edge and
  a 120 ms underline reveal; confirming gives a one-pixel, 90 ms pressed state
  when the action remains on screen. Actions execute immediately. There is no
  animation wait, extra input step, slide or screen fade.
- Artwork uses integer scaling and clipped drawing. Small native transparent
  rims are tolerated before downscaling to avoid unnecessarily tiny icons.
  Dense Build cards reserve space for both label lines; illustration size follows
  the existing row height.
- Non-animated menus stop requesting animation redraws once focus/press motion
  settles. The final release frame is explicitly invalidated so buttons cannot
  remain visually depressed.

## Validation

Release x86-64 native application built with the existing private dependencies.
Four initialized native runtime drivers passed: EverydayControlsTests,
PathCursorTests, RideFlowTests and BrickAuditTests. They exercise real controller
input frames, real park rendering, menu navigation and the existing behavior
regressions. Captures are actual 512x384 native rendering, including original
RCT2 park/ride graphics. Footer assertions still require one 28-pixel row.

Eight standalone checks passed, including launcher, input, save preservation,
browsing, named saves, footer, list scrolling, and the new motion release/idle/
page isolation regression. A short native runtime recording demonstrates Build
focus movement and selected ride/operations tab animation at 20 capture frames
per second. This is a scripted native fixture, not a physical device recording.

No experimental AArch64 build, QEMU performance measurement or physical Brick
verification has been performed for this trial. The stable installed package and
`handheld/modern-ui-paths` branch are unchanged. Preview downloads are visual
review artifacts, not an installable device update. Motion, icon size and contrast
still need judgment on the actual 2x-scaled Brick display before integration.

## Midway / Slate theme trial

The same experiment now has a theme choice at the end of the pause menu.
A switches between Midway and Slate in place; B returns through the existing
menu flow. Midway is the experiment default. Slate retains the original colors
alongside the accepted native artwork and motion. Earlier actions keep their
indices and bindings. Selecting a theme preserves focus, pause ownership, cash,
park state and the current tool.

Midway uses native palette entries: purple panel 154, raised button 156,
lavender selection 164, turquoise focus 199, pale text 165 and yellow accent 55.
The original indexed palette makes the purple more saturated than a muted RGB
plum. These are real native colors, not a recolored screenshot; world graphics,
object colors and global palette entries are unchanged.

The preference is stored separately as `pocket-theme.cfg` in the existing user
profile. It is outside park saves and the native configuration. Missing or
unknown names use Midway. Complete preference writes are staged and renamed;
a failed write retains the previous preference and the active theme remains
usable for the session with an explicit status message. Text antialiasing caches
are separate per theme to avoid stale text colors after switching.

Validation for this follow-up:

- Native x86-64 application rebuilt successfully.
- All nine standalone tests passed, including new preference persistence,
  unknown-name fallback and failed-write preservation checks.
- EverydayControlsTests, PathCursorTests, RideFlowTests and BrickAuditTests
  passed with Midway; captures include native park graphics at 512x384.
- Actual SDL virtual-controller navigation selected the theme, switched back
  using the retained focus, reloaded the preference, and returned with original
  simulation state and money. Both themes retain the single-row footer.
- RideFlowTests also passed with a saved Slate preference. Its stat panel and
  text pixels match the earlier Slate native-menu-feel capture exactly, checking
  the original colors and text rendering after introducing theme selection.
- A nine-second native motion recording shows the Midway Build and ride tabs.

This remains a visual experiment, not an installed Brick update. No new ARM/
QEMU or physical-device testing is claimed. The stable branch is unchanged.

## Daylight follow-up

Following visual review, the active alternate theme is now **Daylight**, replacing
the purple Midway trial. Midway remains in source history at `1df16b4`.
Daylight is this experimental branch's default; **Slate** remains selectable
through the same pause-menu row, with the same saved-preference behavior.
An old `midway` preference falls back to Daylight as an unrecognized theme.

Daylight uses light blue-gray panels (20), ivory raised buttons (225), teal
selection (194), dark main text (10), muted text (13), and small burnt-orange
accents (180). Selected button text uses light ink (21) against teal. Semantic
failure, caution and disabled inks are adjusted for a light background. Both
themes use separate glyph caches. All native sprites, tab animation, button
feedback, controller mappings and focus behavior are preserved. The global
world palette and native object colors are unchanged.

Validation: native x86-64 application rebuilt; nine standalone checks and the
four native runtime regression drivers passed. SDL virtual-controller testing
switched Daylight to Slate and back, restored focus, reloaded the saved
preference, and verified unchanged pause ownership and cash. Actual 512x384
park rendering was captured for pause, Build and ride menus, with a nine-second
motion recording. This remains a visual preview, with no new ARM/QEMU or
physical Brick test, and no changes to the stable branch or installed package.

The light-theme capture exposed a native-font fallback issue: currency symbols
and other non-ASCII text kept a forced white prefix on ivory surfaces. The
fallback now follows the light theme's ink polarity, including white selected
text on teal. A native rendering regression draws `£0.00` on both backgrounds
and checks readable glyph pixels, covering the actual engine fallback rather
than only the custom ASCII font. Slate's previous fallback is unchanged.

## Accepted Daylight layout follow-up

See [DAYLIGHT-LAYOUT-POLISH.md](DAYLIGHT-LAYOUT-POLISH.md) for the focused
post-review fixes: bounded two-line Build labels, readable narrow tabs,
uniform heading, guest warnings, unavailable actions, native graph/currency
contrast, full keyboard labels and contained construction hints. The accepted
palette/artwork and stable branch are preserved.


## Native sprite coverage follow-up

[NATIVE-SPRITE-COVERAGE-PASS.md](NATIVE-SPRITE-COVERAGE-PASS.md) records added
native staff/guest/ride/park/finance/research artwork, section-aware tab icons,
full-label space checks, roster-name isolation and native navigation validation.
The accepted palette and existing control routes remain unchanged.
