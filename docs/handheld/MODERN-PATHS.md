# Pocket modern UI and paths — 0.7 checkpoint

Target: TrimUI Brick / NextUI + PortMaster, AArch64. Logical UI 512 × 384.
Branch: `handheld/modern-ui-paths` in `keeganwhitacre/openrct2-pocket`.

## Implemented

- Embedded smooth Pocket Sans text, derived from licensed DejaVu Sans. Native
  sprite text remains the fallback for non-ASCII/localized strings. No additional
  device font library is required. Circular face-button badges and quiet rows
  replace the pixel-font controller menus and footer. Native G2 path artwork is
  used for the Paths category. The software renderer still uses an indexed
  palette; this does not implement a full RGB/vector rendering backend.
- Live park remains visible beside compact pause, build, terrain and path panels.
  Terrain/path costs are queried through the native action API and displayed in
  a separate badge near the centre cursor. The preview uses the real components
  with a reference park backdrop and sample data; it is not a device capture.
- Paths: normal paths, queues, loaded surface cycling, place/remove toggle,
  terrain-following slopes and editing an existing path at its selected height.
  Actions use native collision, ownership, funds and pause rules. They are queued
  through the game action system; feedback says requested rather than claiming
  completion before execution.
- Park dashboard: live overview, finances, guests and research information.
  Research funding can be cycled using the native action, preserving priorities.
- Selecting a ride in the park opens a controller panel with live status, riders,
  reliability and profit. Open/close uses the native ride status action.
- Existing three-slot atomic save/load and explicit overwrite/discard confirmations
  remain available. Cancel is the default for destructive confirmations.

## Controls / first test

1. Launch an existing park. Select opens Build; Start opens Pause.
2. Build → Paths → Normal path or Queue path. Move owned, clear ground beneath
   the fixed centre cursor with the D-pad. A requests the edit. Y cycles surfaces;
   X switches place/remove; B exits the tool. L/R zoom as before.
3. B does not undo an edit. Start → Save and exit → choose a Pocket slot. Relaunch
   and verify the placed path or queue persists.
4. Pause → Park dashboard → Finances / Guests / Research. In Research select
   Cycle funding. The native action allows this while paused.
5. With no tool active, centre the cursor over a ride and press A to inspect it.
   Try its Open/Close action, then B to return to the park.

The previously hardware-tested terrain loop still works through the same native
land actions. This revision's physical display, input, performance, paths and new
management pages need a Brick test. Start + Select is the existing emergency exit
without saving.

## Validation

Native GUI compiles. Input, launcher and save-preservation CTest suites pass.
The new headless integration test loads real native path objects and exercises
production path query/execution and the native action queue. It verifies normal
placement and cash debit, queue conversion, .park save/reload persistence,
removal, ownership and paused-edit rejection, invalid targets, tool cleanup and
research funding while paused without changing priorities.

The AArch64 GUI and CLI build uses the existing proof installation's runtime
libraries. The delivered ZIP includes an ABI audit and build metadata. Emulated
version launches validate dependency resolution, not physical device gameplay.

## Next work

Explicit bridge/tunnel heights and manual slopes, a placement ghost and improved
error presentation; ride construction and the remaining ride operations;
scenery, shops and staff menus; fuller finance/research controls and graphs;
settings/help. Most desktop dialogs have not yet been replaced. This is a
working incremental checkpoint, not a fully handheld-converted game.

## Update installation

Merge the update ZIP into the existing SD card `Ports (PORTS)` folder. Replace
only supplied files, including both launcher copies. Do not replace the entire
`.ports` directory. The ZIP omits runtime libraries, proprietary assets, engine
objects/music/languages and `save-handheld`; retain those from the working
installation. The root engine `config.ini` is only a template, copied by the
launcher if `save-handheld/config.ini` does not exist. Existing user configuration
and saves remain in `save-handheld`. This is not a standalone first installation.
