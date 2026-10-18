# Single-row footer and TrimUI Brick RC1

Continues canonical Phase 8B `e5fdf0e` on `handheld/modern-ui-paths`.
Phase 8B draft protection, native Undo, camera isolation, caller/tab/roster
bookmarks, native need semantics and all gameplay rules are retained.

## Final footer

Ordinary screens use one **28-pixel** shared strip at 512×384. Navigation and
shoulder navigation group left, A remains near the center, contextual face actions
follow it, and B aligns to the right edge. Dense contexts may move A slightly left
to preserve readable labels. Glyphs retain their size and the accepted slate palette.

Compact pause/file overlays retain their existing embedded one-row A/B controls.
They do not add another row to the gameplay strip.

Hints declare normal, secondary or guide-only importance. Only declared secondary
shortcuts may leave an overflowing strip. Complete prompts remain available in
**Start → Park menu → Controls...**, which preserves the hints from menu entry.
Existing tool panels still explain specialized controls. No bindings changed.

- Dashboard: D-pad Navigate, L1/R1 Section, A Select, B Park. The alternate Select
  entry is in the guide; all six Park actions remain visible without pagination.
- Ride management: navigation, L1/R1 Tab, A Select, relevant X action, B Back.
  Select Sections remains in the guide.
- Coaster: L1/R1 Piece, A Build, X Bank/Speed, Y Chain/Speed/Turn, B Undo/Cancel.
  Select Pieces and L2/R2 Slope remain in the construction panel. Camera retains
  D-pad pan, shoulders zoom, X rotation and safe A/B return to Build.
- Paths: movement, A Place/Start/Review, contextual X/Y and B Back/Cancel/Preview.
  Select Surface remains in the guide and native options. Existing Line/Fill
  previews, stop-on-failure and per-tile charges are unchanged.
- Scenery/landscape: movement/adjustment, A placement/build, Y Adjust, relevant
  Select controls and B Back/Build. Shoulder zoom is available in placement.
- Reviews/editors: current A action and B cancellation retain priority. Draft
  exits still use Continue Editing / Apply / Discard and native callbacks.

Exception: unusually long future/localized hints can wrap as complete prompts,
without reducing the font. None of the captured ordinary English cases required
this fallback. Broad translated-screen/device readability remains open.

## Build and package

Use `packaging/handheld/nextui/BUILD.md`, not the historical proposed PortMaster
recipe. RC1 uses the documented GCC 16.2 AArch64 toolchain, Cortex-A53 target,
private glibc 2.26 runtime and vendor SDL 2.26.1. The restored SDK compiler hash
matches the documented pin. `setup.sh` now supplies the libpng12 include directory
expected by the vendor pkg-config file, fixing a reproducible clean-build failure.

The existing package layout is `Tools/tg5040/OpenRCT2 Pocket.pak/`, with `launch.sh`,
configuration template, `game/` ARM GUI/CLI/input-map, private runtime libraries,
matching generated graphics, languages, public objects/title sequences, source
and dependency provenance, manifest and instructions. Commercial RCT2 data is
excluded. The launcher retains device GPU paths and existing working mappings.

Package validation uses `packaging/handheld/nextui/verify-package.py`: ZIP CRC,
entry paths/modes, hashes, ARM64 ELF machine, complete DT_NEEDED closure, generated
resources, resource lookup paths and isolated user-data location. Optional QEMU
checks execute packaged GUI/CLI version commands through the private loader.
These are loader/runtime checks, not GPU or physical gameplay validation.

## Installation

1. Extract the release ZIP at the SD root, merging the existing `Tools` folder.
   Keep `Tools/tg5040/OpenRCT2 Pocket.pak/launch.sh` at that exact location.
2. Retain/copy your own RCT2 `Data`, `ObjData` and `Scenarios` under the package's
   `RCT2/` directory. `RCT2/Data/g1.dat` is required. An existing installation can
   also be selected using `OPENRCT2_RCT2_PATH`.
3. Safely eject and launch **OpenRCT2 Pocket** from NextUI Tools. Use the in-game
   Save and Exit action for normal sessions.

Saves/config/autosaves stay in `.userdata/tg5040/openrct2-pocket/`; the archive
contains no user-data tree. The launcher copies the template only if no user
config exists. Updating does not erase saves or original assets. Preserve these
folders when your desktop asks whether to merge or replace an existing package.

## Verification

Desktop native GUI/CLI builds and AArch64 GUI/CLI builds succeed. Both ARM version
commands execute under QEMU 8.2.2 with the private runtime. Six standalone tests
and NextUI launcher fixtures pass. Sixteen native integration drivers pass,
covering A/B, tab changes, focus, camera, construction, Line/Fill, duties/research,
review safety and save/reload. Footer height assertions run in each park-backed
capture; bounds checks cover representative controller combinations.

The final guide snapshot passes focused People/Pause reruns. Package CRC, modes,
hashes, resource paths, AArch64 ELF/dependency closure and packaged GUI/CLI QEMU
version checks pass. The transfer archive is `OpenRCT2-Pocket-Brick-RC1-20261018.zip`.
Twenty-two representative PNGs plus a Phase 8B/final comparison are in
`captures/footer-brick-rc1/`; logs are in `evidence/footer-brick-rc1/`. Native runtime captures are
512×384 using installed RCT2 assets, distinct from physical-device testing.

Remaining: actual Brick display/GPU/audio/button comfort, suspend/resume, SD-card
save round trips and larger-park performance; translated layouts; wide Fill panel
occlusion (P7-01); optional bright-sand capture diagnostic (P5-01). No physical
Brick validation is claimed. These are tracked separately from verified desktop
and package checks in `UI_POLISH_BACKLOG.md`.
