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
