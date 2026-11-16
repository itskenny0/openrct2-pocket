# RC4 investigation checkpoint — incomplete

Canonical starting point: `bc683b4670859a7016d1e8ecfe18cfedb4d8409e` on
`handheld/modern-ui-paths`. The restored RC3 release source was compared with
GitHub blob hashes for source, handheld tests and packaging: no differences.

**This is not RC4 acceptance or a path fix.** The user reports that Line and Fill
charge money without visibly building on flat ground on the actual Brick. That
report remains a critical, unresolved defect. No runtime gameplay code changed
in this checkpoint. Passing desktop fixtures do not disprove the device report.

## Path investigation

Traced Explorer input and Adjust/Line/Fill transitions, target hit testing and
height-plane conversion in Paths.cpp, selection/quote/review/sequential callbacks
in PathBatch.h, FootpathPlaceAction query/insert/update, GameActionRunner queue
and native finance payment, provisional removal/restoration, and rendering.
No root cause was established. No balance manipulation, fabricated refunds,
native eligibility changes, or transaction changes were made.

The old PathBatchTests passes against unchanged RC3. Added an optional
`POCKET_PATH_LIVE_TICKS=1` run which uses `gameStateUpdateLogic()` instead of
calling the action queue directly. Both modes assert real non-ghost native path
elements, native surface/queue flags, connectivity, exact cash changes, partial
failure behavior, and save/reload persistence. They cover Single, cardinal Lines,
rectangular Fill, queues, existing paths, blocked ownership, funds, uneven terrain,
obstructions, cancellation, changing quotes and 64-tile bounds. Both runs pass.

Added PathCursorTests to exercise actual native screen-coordinate hit testing on
an initialized flat map. It enters Line through Y Adjust → R1 → B, selects both
endpoints, confirms, performs a full native update, asserts every built non-ghost
tile and exact charge, and repeats for Fill. Both selections persist through
native save/reload with unchanged cash. It passes. The driver positions the camera
programmatically and explicitly invokes the native cursor hit test because a
headless window's focus differs from a device window; it is not physical input QA.

Native 512×384 software-rendered screenshots in `captures/rc4-investigation/`
show preview and constructed Line/Fill paths using the supplied RCT2 graphics.
These are **baseline RC3 reproduction evidence**, not screenshots of a fix.
They were visually inspected: built paths are visible on flat ground.

Logs are under `evidence/rc4-investigation/`. Timing output is desktop fixture
timing, not a claim about Brick performance.

## Build and test scope

- Desktop GUI and CLI compiled successfully (GCC 13; scripting/network/HTTP,
  TTF/OpenGL and compressed music support disabled for this investigation).
- Supported Cortex-A53 AArch64 GUI and CLI compiled successfully using the
  pinned GCC and TrimUI SDK, matching ICU76/libzip/zstd runtime libraries from
  the verified RC3 package and matching dependency headers.
- ELF inspection identifies the ARM GUI as AArch64, interpreter
  `/lib/ld-linux-aarch64.so.1`, Linux 3.10.108 target.
- PathBatchTests passes in queue and full-live-tick modes; PathCursorTests passes.
- Existing NextUI launcher tests pass; `git diff --check` passes.
- No full RC4 gameplay regression suite, physical Brick test, installation test,
  or RC4 archive integrity check was performed. No RC4 installation archive was
  produced: the critical defect has not been fixed.

## Remaining work and input needed

An affected `.park` save from the Brick, together with the scenario and selected
path material if known, is needed to reproduce the reported map state rather
than substitute another passing fixture. Ideally preserve a save before the
operation and one after money is charged; the after-operation save alone is useful.
No new proprietary asset upload is needed: the supplied Data/ObjData/Scenarios/
Tracks archives are already available for this investigation.

| Requested area | Current status |
| --- | --- |
| Line/Fill root cause and correction | Unresolved; not reproduced in desktop fixtures |
| Native mutation/cost/render/persistence tests | Pass in baseline fixtures; do not verify the hardware defect |
| Park guest entry | Traced: Explorer falls through native viewport click to desktop GuestOpen; no fix implemented |
| Coaster ghost and selection | Inspected existing native ghost, piece cycling, eligibility and descriptor-footprint catalog; no changes implemented |
| Native park news | Existing copied-target/history/management routing inspected; no changes implemented |
| Controller changes | None |
| RC4 package | Not produced; acceptance blocked by critical path defect |

Continue with the path reproduction and verified correction first, then the
guest, coaster, news, regression and packaging work in the user's requested order.
Do not label this checkpoint an RC4 release or mark the four hardware reports fixed.
