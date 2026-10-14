# Brick NextUI testing release — incremental checkpoint

The canonical source remains authoritative. Workspace recovery found Staff HEAD
db96e886864a04bfbc60398493f59a221c68c55d and only the protected Esperanto edit.
All 147 differing blobs were recovered from the canonical API, each Git blob hash
verified before writing; original files were backed up. Complete staged source
tree equals 3e43fab9c1f29e2b93221afee6b73872ae48dfa8. Local recovery safety commit
8aed2847fbeacd826f6acd9c44b2e39e95a9fe21 precedes compilation. No history reset,
clone, clean, discard or unrelated overwrite occurred.

## Small scenery usability pass

Existing tree/small/large/wall/banner/native-addition placement already retains
its tool and chosen object; native actions commit each released A press without
reopening the picker. Explorer feedback now identifies tree/scenery repeat
placement instead of incorrectly reporting Land changed. No automatic purchases,
held-A repetition, scatter algorithm or independent placement simulation was added.

Manual scenery elevation now starts at the exact existing native ghost height,
or the successful native placement query position when the ghost was cleared by
the options menu, before adjusting by one native height step. Automatic reset,
rotation and native restrictions remain. Native eligibility/clearance/costs own
placement. This is not desktop Shift automatic clearance-search parity.

Existing connected path/queue extension, native slopes/elevation, styles, removal,
rights, terrain/water, eyedropper and colours are retained. They were audited; no
path/construction rewrite was warranted for the small useful scope of this release.

Release x86-64 GUI/CLI pass. SceneryPathParityTests pass headless/native/SDL, with
new automatic/manual reset, repeated two-tile placement, retained selection, exact
quoted cash and native save/load persistence assertions. Existing native clear/
style/stale/Cancel-first assertions remain. CTest 5/5 passes after correcting local
pkg-config/shared-SDL link configuration. Original RCT2 assets are not present in
this refreshed workspace; asset-backed captures and physical validation are not
claimed. Cross-compiler and dependency provisioning are underway; package closeout
follows this validated source checkpoint.


## Phase A publication

Canonical 9f42abcdc01c4ab7cb8f4f71979278af7a4973c6, complete tree
783ae0f432e09f52195faab732519c4b0ad1230c, matching local safety
6024801a45fd9219258f8c38d0b23eb3492df173. Expected-parent non-force publication
and native parent/tree/ref readback were verified.

## First AArch64 / NextUI testing package

Real Release GUI and CLI cross-builds completed (733 target build steps), using
GCC 16.2.0, glibc 2.26 and official TrimUI SDK SDL 2.26.1. ICU 76.1, libzip
1.11.3 and zstd 1.5.7 were cross-built. The packaging helper verifies the
AArch64 ELF dependency closure, includes a private child-only runtime, matching
native-generated graphics, pinned public objects/title sequences and source.
No proprietary RCT2 assets are included. See packaging/handheld/nextui/BUILD.md
for reproducible commands and source URLs, INSTALL.md and FIRST-PLAYTEST.md.

NextUI Tools/tg5040/OpenRCT2 Pocket.pak/launch.sh derives the SD root and stores
config/saves/logs in .userdata/tg5040/openrct2-pocket. Existing user config is
never overwritten. The SDL helper uses the runtime GUID and physical indices
from NextUI's tg5040 platform.h; physical mapping and joystick enumeration are
not yet verified. Native 1024x768 software output with window_scale=2 gives the
existing 512x384 logical UI; integer scaling selects native nearest-neighbour.
Physical graphics/input/audio and return to NextUI require the first playtest.

Packaged GUI/CLI version checks pass under QEMU using a guest address offset;
normal interpreter mode also passes. The explicit-loader test without offset
crashes at QEMU's fixed 0x400000 mapping: that check alone is not a physical
launch failure or success. Target SDL has no dummy video driver, so target GUI
integration could not run headlessly. Version banner reports recovery 8aed284
because configuration preceded the Phase A commit; the compiled production
files include the Phase A changes and packaging manifest records source tree.

Host Release GUI/CLI, focused SceneryPathParityTests and PathActionsTests,
CTest 5/5 and isolated NextUI launcher path/missing-asset/config/mapping/exit
checks pass. MazeAccessPreviewTests failed in this no-original-assets headless
fixture with GfxGetG1Element assertions and paused-frame preservation failure;
it is not counted as a pass or dismissed as harmless. No assertions weakened.
Original-asset regressions, the broad driver sweep and physical Brick tests
were not rerun here. Prior checkpoints retain their demonstrated validation.

This is a first hardware-testing release, not desktop parity or a physically
validated release. Existing access relocation and unrelated roadmap work were
not started. Preserve the protected local Esperanto file outside milestone
commits. No save migration, delete or proprietary asset redistribution occurs.
