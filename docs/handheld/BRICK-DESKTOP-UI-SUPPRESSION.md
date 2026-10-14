# Brick desktop-chrome suppression — 2026-10-14

The first hardware playtest reported the complete native desktop toolbar/status
UI behind Pocket panels, including date/weather when paused. Pocket overlays
previously left those native windows in the compositor. This change suppresses
only base desktop chrome in OPENRCT2_HANDHELD=1 mode.

## Behavior and native architecture

WindowManager::Create marks topToolbar, gameStatusBar, parkInfoPanel,
dateInfoPanel, newsTicker and titleLogo/titleExit/titleOptions/titleVersion
with a transient WindowFlag::hidden. The native objects and update callbacks
remain alive, preserving callers that depend on toolbar/status window existence.
The shared compositor skips hidden windows in full drawing, transparent passes,
viewport-region drawing and occlusion. FindFromPoint skips them too. The native
main viewport, Pocket HUD/menu, adapted title menu and native error/recovery
windows remain drawable. Visibility is independent of the pause state. Ordinary
desktop mode does not set the flag. No GameAction, simulation, accounting,
park state, serialized save format, original asset or user configuration changes.

The runtime/hat-or-axis controller work at canonical parent
72d0edbb7a88fe68202a768c9d6db72413e57e7d is retained. The recovered complete
source tree was verified against 916f631027f84dac7e48f81f34b0acce57c493c3
and committed locally as 4e561941a2b939f12efa33817792e9f5ece2f282 before
compilation. Existing Esperanto bytes and all overwritten old source were
preserved outside the recovery; Esperanto remains excluded from this milestone.

## Validation

Host Release GUI and CLI were rebuilt after the shared flag/header changes.
DesktopChromeTests links against the final native GUI objects and runs in
separate Pocket and desktop processes. It asserts opaque and transparent
chrome pixel/hit-test behavior, close/recreation and retained Pocket/title/error
windows. These are synthetic colour-window compositor assertions, not park
screenshots. The error theme is naturally transparent; its fixture retains an
underlying viewport layer. Both modes pass.

PresentationLifecycleTests passes with SDL dummy video/audio and public engine
data: native HUD/menu/build return, held-input protection, construction panel
insets, native save/reload and shutdown. Fresh standalone handheld CTest passes
5/5: input, launcher, save preservation, file browsing and named save. The host
SDK's static SDL link initially lacked backend dependencies; selecting installed
shared SDL corrected the test-build dependency setup. No assertion was weakened.

AArch64 Release GUI and CLI compile and link successfully. Both execute
--version under QEMU 7.2 with the original private glibc 2.26 runtime plus the
retained librt/ALSA fixes. The same compositor driver is linked against target
GUI objects and passes both Pocket and desktop modes under QEMU with shipped
SDL's offscreen backend. The initial dummy-video invocation fails because this
target SDL provides mali/offscreen, not dummy; using its actual offscreen driver
runs the test without replacing SDL. These are emulated compositor/runtime
checks, not physical graphics, audio or controller validation.

Reproduce native compositor checks with the existing GUI-linked runner:

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy python3 tools/handheld/run_native_test.py \
  --build HOST_BUILD --data PUBLIC_DATA --ninja NINJA \
  --source test/handheld/DesktopChromeTests.cpp --test-arg pocket
# Repeat with --test-arg desktop in a fresh process.
```

The runner's optional repeatable --test-arg adds driver arguments; existing
invocations remain unchanged.

No proprietary assets or connected physical Brick are available here. This
update has not been physically display/controller tested. Original-asset and
broad gameplay preservation suites were not rerun for this narrow compositor
change. The earlier virtual-controller polling override failure is not converted
into a pass; actual D-pad behavior still needs the user's hardware retest.

## Build and update

The cross build uses the existing GCC 16.2/glibc 2.26 AArch64 Cortex-A53
compiler and the original release's exact target SDL/ICU/zip/zstd/png/zlib
libraries. Missing SDK PNG/zstd headers were supplied from the official SDK or
matching dependency sources; source remained fixed during production compilation.
The scratch toolchain supplies the compiler sysroot and restricts library/header
lookup to the target. Release switches disable HTTP/network/TTF/OpenGL/Discord,
Vorbis/FLAC, IPO and benchmark dependencies, matching the software-rendered
handheld build. The executable is the native game, not a mock frontend.

The small OpenRCT2-Pocket-Brick-UI-Fix.zip is an update to an installed testing
package. Copy the payload contents into the folder containing the working
openrct2 executable (normally Tools/tg5040/OpenRCT2 Pocket.pak/game), replacing
files rather than deleting folders. Copy launch.sh into the .pak root. Retain
existing data, config.ini, RCT2 assets and SD .userdata saves. The diagnostic
launcher supports both game/ and direct .pak layouts. Included librt/ALSA fixes
and input-map preserve prior runtime/controller updates. Source and GPL licence
are included; original RCT2 assets are not.

Eject the SD cleanly, launch, and verify the desktop top/status/weather panels
are absent in exploration, Build, paused menus and after native save/reload.
Check D-pad navigation and native error dialog recovery. If input still fails,
retrieve .userdata/tg5040/openrct2-pocket/launch.log, including the v2 joystick
GUID/buttons/axes/hats line. Do not reformat or remove saves for this UI fix.

Publication uses the canonical parent above, verified new blobs and complete
local/canonical tree equality with expected_sha and force=false, then branch
readback. Exact resulting identifiers and binary/archive hashes are recorded in
the delivered update manifest and final handoff.
