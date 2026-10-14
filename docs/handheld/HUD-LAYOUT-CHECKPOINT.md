# Pocket HUD and shared 512×384 presentation — 2026-10-08

Continues the completed custom-coaster checkpoint
`8645e18c766c16ed045e2d5c9ef08ad8dbe9b996`. Presentation only: no gameplay
action, controller mapping, park graphics, simulation rule or save format changes.

## Native HUD

The 496×24 solid Pocket strip sits at (8,6). It prioritizes native formatted cash,
native day/short month/year, guests in the park, and pause/simulation speed.
Short month names come from the native language table; the year is one-based,
matching the desktop date panel. Cash uses the existing shared native formatter.
Negative cash uses gold; no-money parks say No money. Simulation multipliers
derive from the engine's exponential speed state: 1×, 2×, 4×, 8× and native debug
128×. Pause reflects GameIsPaused(), including non-user pause reasons. This is a
read-only HUD; it does not add speed or pause controls. Rating and open/closed
status deliberately remain in Overview to avoid crowding this small strip.

Cash has approximately 38% of the strip, date 31%, guests 20%, speed 11%.
Fields ellipsize within their cells for exceptionally long native values;
Finances retains the complete contextual amount. The people glyph follows the
existing palette-safe Pocket line-icon vocabulary. No original assets enter git.

## Visibility and layout contract

| Context | HUD | Content origin |
| --- | --- | --- |
| Exploration | Full | Park remains unobstructed below y=30 |
| Terrain, Water, surfaces, Paths, removal, Trees, Shops | Full | Compact panel at (8,36) |
| Flat rides, saved designs, custom track, entrance/exit placement | Full | Same construction inset |
| Guided RideAccess Paths | Full | Same path panel and native controls |
| Path Options and visual pickers | Yields | Menu origin (8,8) |
| Ride/shop/guest/staff inspection and access review | Yields | Existing menu/management shell |
| Dashboard, drafts and confirmations | Yields | Full two-pane management area |
| Pause, Build, save/load, other modal/blocking native windows | Yields | Existing menu layout |
| Non-playing scenes / teardown | Removed | Existing native scene lifecycle |

`Layout.h` owns outer margins, HUD rectangle, construction/menu origins,
six-pixel gutters, the unchanged 28-pixel action strip, content height and clamped
floating context anchors. `ConceptStyle.h` exposes the construction and menu
origins so callers cannot accidentally shift dense menus below the HUD.
`Dashboard.h`, `ObjectPicker.h`, Path Options and the management compatibility
wrapper explicitly use the menu area. Construction renderers inherit the inset.

Explorer maintains a separate input-transparent native redraw window for the top
strip. It shares the existing CanExplore/modal guards, removes windows when the
park yields to menus, and recreates them on return. Rendering checks the clipped
target before formatting HUD state, avoiding work in unrelated panel/footer clips.
There is no cached serialized HUD state or fake mouse interaction.

The shared cost badge is 160×26, with a larger native amount field. Normal tools
keep the contextual centre-offset anchor. Custom construction uses a bottom-right
anchor above the action strip; its redraw window and its cost/error renderer now
use the same helper. Previously those custom feedback coordinates disagreed,
which could hide live feedback despite full-target component captures showing it.

Dashboard preserves existing stats, hierarchy and paged action rows. Confirmation
titles use gold, while Cancel-first action focus remains unchanged. Explicitly
unavailable rows use muted ink. Read-only body lines now obey pane truncation.
The shared text renderer recognizes the existing GOLD token directly through
Pocket's gold ink instead of sending it to the native-format fallback.
Custom track footprint rendering is factored into DrawPiecePreview; the live
picker and component driver use exactly the same native TED metadata renderer.

Palette remains unchanged: panel 118, border/separator 119, focus 53, primary 21,
secondary 18, selected ink 10 and positive amounts 103. All surfaces are solid.
There is no dithering, fake transparency, blur, gradient or new asset pipeline.

## Validation

- Native Release GUI and CLI build successfully.
- CTest input/launcher/save-preservation passes 3/3.
- Full graphics-disabled native handheld suite passes: paths/ghosts, water,
  surfaces, removal, trees/picker/ghosts, shops/management, finances, staff,
  Dashboard, research, marketing, guests, rides, Overview, flat rides and access,
  accounting, stale targets, native restrictions and save/reload.
- New native HUD tests stress negative/large native cash, 100,000 guests, late
  dates, all native speeds, different pause reasons, no-money and safe areas.
  Tests restore their temporary fixture state.
- New real SDL/window-manager lifecycle driver uses public G2 fonts and a
  graphics-disabled native park fixture: exploration/construction HUD, panel
  origin, Dashboard and Build/pause hierarchy, yielding/return, save/reload and
  context shutdown. It closes initialization's pending progress window normally;
  production modal guards remain intact. SDL dummy renderer warnings are expected.
- Component rendering asserts HUD pixels stay within its reserved rectangle.
  Long names, selected/disabled states, paging, confirmation and dense construction
  layouts are rendered with production components. Changed-source diff checks
  and Python capture-driver compilation pass.

The old private assets are unavailable. Their captures were not recovered or
regenerated. Asset-backed saved-design/custom-coaster/elevated guest-operation
scenario runs therefore were not repeated. Their gameplay/actions are untouched.
No physical Brick/ARM package or hardware performance validation is claimed.

## Captures

`render_shell.py` supports exploration, terrain, paths, construction, picker,
inspection, dashboard, confirmation, hud-stress and path-options-last pages.
Individual PNGs are exactly 512×384, produced by the native indexed renderer.
These are **component fixtures with no live park**; financial/roster labels are
sample presentation data. The construction fixture honestly has an unavailable
endpoint, not a manufactured coaster. Picker footprints use existing native TED
previews, not object placeholders. The contact sheet labels every fixture.

```sh
python3 tools/handheld/render_shell.py --build BUILD --data PUBLIC_ENGINE_DATA \
  --ninja NINJA --output EXTERNAL_OUTPUT/component-sheet.png \
  --frames-dir EXTERNAL_OUTPUT --pages exploration terrain paths construction \
  picker inspection dashboard confirmation hud-stress path-options-last
python3 tools/handheld/run_native_test.py --build BUILD --data PUBLIC_ENGINE_DATA --ninja NINJA
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  python3 tools/handheld/run_native_test.py --build BUILD --data PUBLIC_ENGINE_DATA \
  --ninja NINJA --source test/handheld/PresentationLifecycleTests.cpp
```

## Checkpoint/workspace preservation

The surviving workspace was an older checkout with an unrelated Esperanto edit.
It was preserved untouched. A separate copy was hydrated with the existing GitHub
checkpoint's changed source/docs/tests; its committed baseline tree was verified
as `3df830e76b6dccfd67e5d7bb735777e417bd7a2e`, exactly the canonical checkpoint tree.
Local implementation checkpoints were committed incrementally. Publication uses
that canonical remote parent and a guarded branch update, excluding the unrelated
language edit. No clone/reset/clean/restore was performed on the existing checkout.

## Physical test / intentionally deferred

Check date/cash/guest legibility at arm's length, gold focus visibility, bottom
prompt spacing, HUD viewport-smearing during rapid pan/zoom, custom feedback,
long native/localized names, picker minification and dense management pages on the
3.2-inch Brick. Global values intentionally yield inside management and are shown
in their relevant sections. No management stats were removed merely to eliminate
duplication. Existing English Pocket labels and localized native fallback remain;
full localization, rare dense-page restructuring and gameplay feature depth are
deferred until hardware feedback. No new subsystem should precede that feedback.

Next: physical Brick play/readability test → focused hardware-feedback cleanup →
desktop-versus-Pocket depth matrix → subsystem depth/completeness.

## Shop appearance capture closeout

Native shop appearance content uses a 330-pixel indigo panel at (8,8), bounded
native palette paging, gold focus and contextual controller footer, leaving the
native shop/park visible. Management keeps the existing HUD suppression; shop
preview uses the existing compact HUD and overlay/footer. All 28 final 512x384
PNGs were inspected for clipping/overlap/contrast/focus/controls/HUD collisions;
see SHOP-APPEARANCE-COMPLETION.md. No hardware/performance result is inferred.


## First Brick playtest follow-up — 2026-10-14

Native base desktop chrome was still composited underneath the Pocket overlay.
The retained-hidden native window flag now excludes desktop toolbar/status/date/
weather/news and title chrome from painting, occlusion and hit tests in Pocket
mode, while preserving the native viewport, Pocket windows and error dialogs.
See [BRICK-DESKTOP-UI-SUPPRESSION.md](BRICK-DESKTOP-UI-SUPPRESSION.md) for native
and AArch64 emulated compositor checks, rebuilt binaries and hardware retest
limits. Ordinary desktop mode remains tested and unchanged.
