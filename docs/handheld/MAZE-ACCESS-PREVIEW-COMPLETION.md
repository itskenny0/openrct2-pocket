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
