# Native maze and ride-access preview completion — incremental checkpoint

## Baseline integrity

Canonical baseline 7ac014a7a4f1810bccbc2d81e110af98c0c344a1 and local safety
c6778cd7698575c2c8b422cd9a58899b51bb837f have identical complete tree
f4434bd62e3b645b887e3db9dd4372cfe24eb6aa. Current canonical branch verified;
no recovery required. Sole unrelated edit is protected data/language/eo-ZZ.txt,
SHA256 e37ba559b4d9d761e74f2bb284d1ec762b813e789655c0495a882c83651890b5,
preserved and excluded. No clone/reset/clean/restore/history rewrite/force push.

## Native audit and Phase 1 — ride-access previews

Native desktop RideEntranceExitPlaceAction ghost execution writes station access
references and resets entrance queue metadata. Removal also updates real maze
hedges and path connections. Reusing its ordinary ghost flag alone cannot satisfy
Pocket real-state purity. Desktop provisional access stores separate ghost
coordinates; its legacy behavior remains unchanged.

An opt-in pure ghost mode in the existing native placement/removal actions only
inserts/removes native ghost EntranceElements and invalidates rendering. It skips
real station references, queues, maze hedge and path mutations; non-ghost use is
rejected. No independent simulation or UI live-state repair is added.

FlatRideEntrancePlaceAction uses that native mode for Pocket ghosts and carries
serialized native allocation/object/version/station/access/nearby-real-tile review
context and optional exact quote. Query and Execute revalidate. Nearby tile lists
exclude ghosts and normalize the physical last-element flag; no real map writes
are used to normalize. Native loaded ride/station descriptors, researched state,
station geometry, object flags, clearance/pause/money policy and source block
metadata are included. No new park-save gameplay state exists.

The existing FlatRides station resolver/stage supports audited flat, conventional
and maze station contexts. Its UpdatePreview now renders native missing-entrance/
exit ghosts, preserving selected station/type/direction/height. A binds the latest
matching preview/context/native quote; actual placement clears the ghost before
native execution, uses existing busy/edge input and advances native station stages.
Invalid sides, missing objects and changed station context remain native errors.
This phase does not add moving an already present access point or arbitrary
unsupported station-family geometry.

AccessPreview retains value identities, never map pointers, and participates in
nested provisional suspension/restoration. Existing FlatRides clear/end/focus/menu/
load paths clear its request so restoration cannot resurrect a canceled preview.
Native cleanup removes only ghosts, including after real ride status changes.

Phase 1 validation: Release GUI/CLI, asset-backed native/SDL
MazeAccessPreviewTests, entire PathActionsTests in both documented environment
(OPENRCT2_HANDHELD unset) and forced=1, CTest 5/5. Focused tests demonstrate native
Carousel station/path/map/cash purity, serialized guards, loaded object-version
changes, native same-slot ride reuse rejection, changed/invalid targets, nested
suspension, actual placement/stage/accounting and ghost-free native autosave/load.
Physical Brick/AArch64 and real foreground display are not claimed.

## Maze audit — implementation follows Phase 1 publication

MazeSetTrackAction's existing ghost flag is not safe for editing real maze cells:
its lookup can resolve real entries, build/fill modify neighboring wall masks,
and insertion/removal update real mazeTiles/station state. Its native wall-bit
algorithm must be reused on isolated native ghost entries, rather than performing
UI rollback or independently reproducing geometry. Phase 2 and final end-to-end
preservation/capture closeout remain pending at this incremental checkpoint.

Whole-tick native provisional intents also suspend access previews around paused
queue/snapshot/autosave work, nesting with the broader simulation/save scopes.
Focused SDL tests align the real viewport to the native side, complete native
input reset/cull, exercise a native build-in-pause setting and assert paused frame
restoration or observed-focus cleanup with unchanged real map/cash. Native research
is inserted into the fixture's actual research list during initialization so
save/reload reconstructs eligibility; tests do not unlock objects after import to
force a pass. Actual reloaded native object indices are re-resolved before reuse.

## Phase 2 — isolated native maze proposals

Phase 1 was published at 5279377b68aa216dee12bf5e9e71c5277e589a5f, tree
7a6be7ad9faaec9dc40ffd38c2d3141021af5357; local safety commit
3deff45e45d5bead6f1d6703d440d9c880b2dc6e.

MazeSetTrackAction now has an opt-in serialized pure-ghost mode and exact native
context/quote guards. It copies native real maze elements into native ghost
elements, then runs the existing native wall-bit algorithm on those ghost
elements. Real maze tiles, neighboring walls, stations and cash remain intact.
Cleanup removes only matching native ghost maze cells, even after a ride becomes
stale. Ordinary native execution and legacy design placement remain native actions.
No UI map repair or independent preview geometry is introduced.

Pocket Mazes refreshes native build proposals at the half-cell cursor, direction
and supported manual height. Fill review previews native wall changes and starts
on Cancel. Move remains native cursor navigation without a geometry/cash mutation;
the subsequent Build proposal is shown. A complete removal leaves no ghost cell
to render, so the native selection/review marker remains the destination cue.
No empty-cell hedge is fabricated. Failed queries clear invalid ghosts and retain
native errors. Queued Apply binds exact allocation, descriptors, station/real
neighbor geometry, money/pause policy and native quote, repeated inside Query.

Maze previews join nested simulation/save and whole-tick paused suspension, and
clear immediately at menu/focus/tool changes. Restoration queries current native
context. No preview ride or proposed station state is serialized into .park.

Phase 2 validation: Release GUI/CLI, focused original-asset native/SDL
MazeAccessPreviewTests, MazeConstructionTests and forced-Pocket PathActionsTests;
CTest 5/5. Focused assertions compare actual native ghost wall rosters with
committed results for initial, within-tile, neighboring-tile and fill operations;
prove cancellation/map/cash/station/count purity, nested scopes, invalid targets,
height, insufficient funds, changed object/version and serialized quote/money-mode
rejection, and native autosave/load ghost exclusion. Final broad preservation,
end-to-end family coverage, captures and handoff remain pending.

## Phase 3 — end-to-end and capture closeout

Phase 2 was published at 4f3b198931792b437f1a4e8d782e7bc3df30f85e, complete tree
eae892dd76ba350755b7cbfd78e436dd4009f776; matching local safety commit
4ec8cb911fd22dc07726783f9cf5096a44a08a71. Both phase publications verified every
changed blob and complete tree, expected_sha / force=false fast-forward ref,
read-back native parent/tree and branch head. The original baseline was
7ac014a7a4f1810bccbc2d81e110af98c0c344a1 / f4434bd62e3b645b887e3db9dd4372cfe24eb6aa.

Focused native/SDL tests now include original Carousel, Hedge Maze and Monorail
station access previews. Exact real map bytes (normalized physical sentinel),
station structs, native maze wall/count rosters and cash are asserted. Native
same-slot ride reuse, object-version changes, invalid sides/targets, quote and
money-mode changes, pending repeated Apply, native insufficient funds, actual
wall results and native cost deductions are checked. Source objects are loaded
and fixture research initialized before saves; no import-time unlocking is used.

The maze flow completes Cancel-first Fill via real SDL virtual held A without a
second submission, immediate menu cleanup, construction return, actual native
entrance/exit placement, native Open and .park save/reload. A conventional
three-piece Monorail station demonstrates real station/access purity and return
to native path-access management. Its incomplete circuit is not claimed Open or
operating. Native Maze Open evidence is status/access eligibility, not sustained
individual-family guest operation. No customers/income/guest paths/statistics are
manufactured in captures. The preservation Wooden circuit/access/natural-guest
regression exercises actual operating guests separately.

Paused full native ticks for access and maze assert either valid restoration or
observed focus cleanup with exact real state preservation. Native viewport
alignment and normal-input return precede those ticks. Native load can leave the
normal pause flag set; recovery tests use native PauseToggleAction rather than
assuming an imported park is running. The real Pocket load sequence ends tools
before native import. An absent-file early failure through that sequence preserves
the real map/cash, clears previews and recovers through an explicit maze resume.
Direct Context::LoadParkFromFile is not itself a general tool-reset contract.
Late native object/import failures retain the limitations in SAVE-LOAD-BROWSING-
COMPLETION.md; no new rollback guarantee exists. Native successful load rejects/
clears a queued maze action, resets drafts and excludes ghosts. Actual native
scene transition to title and context pre-unload also clear maze/access drafts.
Camera and menu transitions clear previews immediately. Nested scope requests
restore only while their native identities still match.

Capture inspection found recovered valid proposals could retain an old funds/
stale error. UpdatePreview now clears prior feedback after a successful native
query; native failures still retain error detail. The fix was rebuilt before
relinking all drivers. No production source changed during builds or validation.

### Actual final validation

Release GUI/CLI passed. CTest 5/5 passed: handheld-input, handheld-launcher,
handheld-save-preservation, handheld-file-browsing and handheld-named-save.

All 18 relinked native/SDL drivers passed on final production objects:
SceneryPathParityTests, ConstructionTests, PathActionsTests,
PresentationLifecycleTests, ParkMapInformationTests, GuestInformationTests,
GuestCompletionTests, StaffCompletionTests, FinanceResearchInformationTests,
RideConstructionTests, SeatRotationTests, FamilyOperationTests,
MazeConstructionTests, MiniGolfConstructionTests, SelectedTrackEditingTests,
DesignSubstitutionTests, ShopAppearanceTests, MazeAccessPreviewTests.
PathActionsTests also passed separately with OPENRCT2_HANDHELD unset; forced=1
results are reported independently. SDL video/audio dummy were used. Driver argc
contracts were inspected; fresh profiles/output directories preserve prior
exports. No assertions were skipped or weakened. Focused additions for conventional
access, early failure, native title and pre-unload were subsequently compiled and
passed against the same final production objects.

All six relinked original-asset regressions passed: SelectedTrackEditingCapture,
DesignCapture, RideConstructionCapture, MazeConstructionCapture,
CustomCoasterCapture (original Wooden circuit/access/natural guests),
SaveLoadBrowsingTests. Factory Capers and African Diamond Mine are native source
scenarios in their applicable drivers. Private source assets and outputs are not
published. SDL dummy renderer warnings and unavailable headless changelog text do
not constitute physical-display validation. Physical Brick/AArch64, real focused
display and dense-park performance remain unconfirmed.

### Captures

OpenRCT2-Pocket-Maze-Access-Preview-Captures.zip contains 29 inspected native
production-rendered 512x384 PNGs, actual dimension/SHA256/source/binary provenance
manifest, PNG checksums and README. ZIP SHA256:
7782fe9768d31e143ff5d5b3ae4757930583b9bcb0a56d5f02346fdd53c663c1

Native X8DrawingEngine/ViewportRender uses actual original g1 and loaded objects;
actual Pocket HUD, Mazes, FlatRides and Menu components draw panels and controls.
These are production-component captures with SDL dummy, not physical device
screenshots. Each PNG was inspected for panel/footer/HUD clipping, overlap, focus,
contrast and readability. Native long-name ellipses and native action-money sprites
remain honest. Headless missing-file console output is tested, not represented as
a fabricated visual error window. No assets, source parks, generated designs,
profiles, private archives or inspection contact sheets are in the ZIP. Existing
completed milestone packages are preserved.

### Controls and precise remaining limits

Access retains D-pad target movement, A native placement, B return, station/
entrance/exit recovery and Start menus. Native station geometry resolves its side,
direction and base height; unsupported sides remain native errors. Preview guards
bind exact native ride allocation, loaded descriptors/flags/version/checksum,
station/access/real-neighbor tiles, target and actual quote. Ghost modes are opt-in;
legacy desktop/design behavior remains default. No network-peer compatibility or
network review validation is claimed by this DISABLE_NETWORK host build.

Maze retains half-cell direction on L1/R1, A Build/Select/Confirm, X native cursor
Move, Y Cancel-first Fill, B Cancel/Finish-access, L2/R2 initial height or selection
corner, Select camera and Start menu. Movement does not invent committed geometry.
Native ghost copies include the wall stencil; a fully erased cell has no hedge to
render. The native selection/review marker identifies that target.

Only missing entrance/exit placement is delivered. Relocating/removing an existing
access point and its queue/path/maze-hedge consequences remain an ordinary gap.
No exhaustive family/station/diagonal/special parity is inferred from these three
fixtures. Direct selected-seat/further parameters, diagonal/special/exhaustive
selected replacement and positive transitions; flat/special saved-design adapters
and unsupported substitutions; continuous scenery gestures/automatic-height/network
reviews; native file-location/format/keyboard and late-import limits; sustained
individual family guest operation; physical Brick/AArch64, focused display and
dense-park performance remain as recorded in the matrix. No atomic replacement,
automatic restoration or independent rollback simulation is claimed.

Next substantial work: audit and complete controller-native existing ride
entrance/exit relocation/removal and access recovery, using native actions and
explicit consequences/reviews. Do not begin it as part of this milestone.

## Verified publication closeout

Phase 3 implementation/tests/capture closeout published at
b6bd017973ddbb3855dc903f91635d03c425ec3d, complete tree
e7cb62982b64afe34e47239aa1081e2ecca05981, matching local safety commit
5e754ba5db3b0341c94e81167429f3fdbbd15bf1. Every changed blob and complete
source tree matched; expected_sha / force=false publication advanced from the
verified Phase 2 parent, and canonical ref/native parent/tree were read back.
This publication-history follow-up changes documentation only. The capture ZIP
was saved and made downloadable; completed prior capture packages remain intact.
The sole preserved unrelated working-tree edit is data/language/eo-ZZ.txt, SHA256
e37ba559b4d9d761e74f2bb284d1ec762b813e789655c0495a882c83651890b5, excluded
from all milestone commits. No next subsystem has started.
