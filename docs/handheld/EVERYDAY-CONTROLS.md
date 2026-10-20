# Paths, safe quitting and everyday simulation controls

Continues canonical 81bc3bb499c628e3b83da6e674f0cb7507fed2b7. The existing coaster construction, actionable native news, palette, catalogs, continuous scrolling, focus restoration and repeated-Close crash recovery are retained.

## Implemented

- Native quit requests during play, including the Explorer quit chord and SDL/native window quit requests, route through Pocket's Save and quit / Quit without saving / Cancel review. Discard has a second, Cancel-first Pocket confirmation. Confirmed discard and successful atomic saves finish the native loop without reopening the desktop prompt. Back restores the originating menu and focus, or closes a standalone review. Save failure, cancellation and overwrite cancellation remain in the controller flow. The Controls guide now returns to its originating pause-menu row with B.
- Paused, 1x, 2x and 4x are direct pause-menu choices, with the current simulation state marked. They use native GameSetSpeedAction values 1/2/3: GameState derives simulation updates as 1 << (speed - 1). Selecting a running speed releases normal pause; selecting Paused retains it. Back preserves the pre-menu player pause and other native reasons; menu cleanup releases only its own pause. Existing shortcuts remain.
- Ground Single/Line/Fill start on native terrain. Batch ground selections resolve each tile's native base height and slope, and keep following ground after completion/cancellation. A batch anchor no longer silently turns a ground operation into a persistent elevation plane. Intentional level placement remains available through Adjust/height controls; a selected elevated path anchors a level continuation. Connected endpoint elevation/slope logic remains unchanged.
- Batch queries freeze the same position/slope geometry used by sequential native execution. Native ownership, clearance, queue, slope, affordability, cost and connection rules remain authoritative. Failed/cancelled queries do not execute; partial execution retains and reports completed purchases.
- Lamps and banners use the real path selected by the reticle, carrying its height into preview and execution. Without a reticle-selected path, automatic placement chooses the real path nearest the terrain rather than the first tile element. Ghost paths are ignored. Native interior-edge, slope, queue and crossing restrictions are preserved.
- Ordinary ground state reads Follows ground; explicit height is labelled Level. Quit confirmations have enough space for their complete rows. No new coordinate displays were added.

## Reproduction and validation

The original dc4d8c6 Paths.cpp and PathBatch.h were compiled into a comparison driver while retaining the same engine and test fixture. The second Line on terrain 64 world units higher reproduced a real path below terrain: the original driver fails with `Repeated ground Line built below the terrain`. The corrected driver passes. This is an actual native reproduction of the retained-height defect, not proof that every reported physical-device path failure had that cause.

Native Linux x86_64 GUI and CLI builds pass. Native runtime suites pass:

- EverydayControlsTests: mapped SDL virtual-controller D-pad/A/B, current menu navigation, player/menu/external pause ownership, Paused/1x/2x/4x, repeated entry, native quit routing, final discard, caller focus, cancelled saves, an unwritable /proc save destination, successful atomic save and reload; no desktop savePrompt.
- PathReliabilityTests: repeated and cancelled ground anchors, flat and varying terrain, native slopes, explicit elevation, selected elevated anchors, stacked-path lamp selection, interior-edge rejection, exact cash and save/reload including a real lamp.
- PathBatchTests: Single, cardinal Line, Fill, queues, query/execute accounting, connections, cancellation, land/funds/clearance failures, partial outcomes, cache/bounds and persistence. Both action-queue and full gameStateUpdateLogic runs pass.
- PathCursorTests: actual native screen-coordinate hit tests and controller-dispatch Line/Fill selection, real non-ghost elements, exact charges and reload.
- PauseResumeTests and SceneryPathParityTests pass, including connected path/access and scenery persistence coverage.
- Seven standalone CTest checks and the NextUI launcher checks pass.

Captures are real 512x384 software-rendered native frames using supplied RCT2 graphics. Everyday-control images use Electric Fields; path images identify initialized fixture maps. They are not mockups or photographs of a Brick.

## Build and device scope

Host dependencies were extracted into a private workspace prefix. The ARM compiler and TrimUI SDK archives match their pinned SHA-256 values. The existing package supplies the matching ICU76/zstd and target runtime; libzip 1.11.3 headers were generated using its native cross configuration. No system or device libraries are replaced.

The installable AArch64 update, final executable integrity/QEMU checks, exact checksum and installation instructions are recorded in [EVERYDAY-CONTROLS-DELIVERY.md](EVERYDAY-CONTROLS-DELIVERY.md). Physical-device verification remains separate.

No physical Brick is connected. Real device display, controls, audio, suspend, SD-card saves and dense-park performance still need hardware verification. Ground following preserves native connection geometry; it does not construct terrain ramps across incompatible cliffs. The original affected Brick save was not supplied for comparison.
