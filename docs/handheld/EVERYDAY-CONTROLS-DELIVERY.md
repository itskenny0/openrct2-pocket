# Everyday controls: build and verification

Implementation commit: 0d7ff87d588b9c17035fece8f70da3111abbdf87 on keeganwhitacre/openrct2-pocket, branch handheld/modern-ui-paths. This rebuild preserves the newer source and repeated-Close crash recovery.

## Source and native runtime

All 1,877 canonical source, CMake, packaging, script and test build inputs match their published Git blob hashes. The four generated native graphics files match the installed UI Audit package byte for byte, so those assets are retained.

The native Linux x86-64 GUI and CLI build successfully (GCC 13, -O1 -DNDEBUG). Nine initialized native runtime suites pass: EverydayControlsTests, PathReliabilityTests, PathBatchTests, PathCursorTests, PauseResumeTests, SceneryPathParityTests, RideFlowTests, ConstructionUxTests and Rc4IntegrationTests. PathBatchTests also passes with POCKET_PATH_LIVE_TICKS=1. Seven standalone CTest checks and the NextUI launcher checks pass.

Coverage includes mapped SDL virtual-controller navigation, pause ownership and Paused/1x/2x/4x, caller focus, native quit requests, deliberate discard, save cancellation, an actual unwritable save destination, successful atomic saving and reload without the desktop confirmation. Paths cover ground Single/Line/Fill, varying terrain and slopes, intentional elevation, queues, real elements versus ghosts, native connectivity and lamp rules, exact cash, cancellation, unavailable land/funds, partial outcomes and persistence. Coaster construction, spatial editing and actionable news regression suites pass.

Screenshots are actual 512x384 native software-rendered frames. Pause and quit captures use Electric Fields with supplied RCT2 graphics. Path captures use initialized fixture maps with real native terrain and committed tile elements. They are not mockups or physical-device photographs.

## Brick build and package

The AArch64 GUI and CLI are Release -O3 -DNDEBUG, Cortex-A53, GCC 16.2 and the original private glibc 2.26/SDL 2.26.1 runtime. Both pinned compiler/TrimUI SDK archive SHA-256 values match BUILD.md. Scripting remains enabled; network, HTTP, OpenGL, TTF, Vorbis and FLAC remain disabled as in the installed build.

The final stripped executables run their version checks under QEMU 8.2.2 using the package's private loader and libraries. The full merged package passes the existing archive verification: CRC, safe paths, executable modes, complete manifest hashes, AArch64 ELF segments and dependency closure, generated/native resources, title sequences, launcher and isolated save paths. QEMU verifies runtime loading, not target gameplay or device rendering.

The small update replaces only the two executables, their checksum/manifest and new provenance/install/validation documents. It preserves existing RCT2 assets, data, libraries, settings and saves. Its cumulative source patch applies to the original package's bundled source ZIP and includes the repeated-Close recovery fix.

Install onto the existing UI Audit package, with or without its separate Crash-Close update: quit the game, back up the existing executables/checksum/manifest, extract the update onto the SD-card root, and merge/replace included files. Do not delete OpenRCT2 Pocket.pak or its game folder. Keep .userdata/tg5040/openrct2-pocket intact. Roll back by restoring the backed-up executables/checksum/manifest.

## Limits

No physical TrimUI Brick is connected. Device input/display/audio/suspend, SD-card save behavior and dense-park performance are not physically verified. The original affected Brick save was not supplied. Ground following preserves native placement geometry and does not build ramps across incompatible cliffs. Repository-wide formatting CI fails on formatting differences; these successful local builds/runtime checks are recorded separately and are not a claim of green GitHub CI.

## Download identity

Update: `OpenRCT2-Brick-Everyday-Controls-20261020.zip` (10,310,655 bytes; 9.83 MiB).

SHA-256: `86fbe7ef1d2a717075e619c20fab5b674859f5a8c90c3a46f71afac34895e09c`.

The update, verification archive and representative PNGs were saved durably. The saved update was downloaded again and its exact checksum and ZIP CRC verified. The saved verification archive also passes CRC. This is an update for the existing UI Audit package, not a complete replacement installation.
