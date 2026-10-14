# Native save/load browsing — incremental completion

Baseline canonical 64751a6dc47d3e32af5ad0bdd19a2e4f599bfd18, tree
dd71aecc86d34215e2cef3dbd6eae2347c1d9e74; local safety d9d79abc.
Workspace inspection found the exact matching tree, no staged/untracked differences,
and only protected Esperanto at its required SHA256. No recovery was needed.

## Phase 1 — parks and autosaves

Load retains three native Pocket slots and adds Browse parks / autosaves. The browser
starts at FileBrowser::GetInitialDirectoryByType(park), using the desktop native
GetFilterPatternByType: .park/.sv6/.sc6/.sc4/.sv4/.sv7/.sea, case-insensitive.
Actual regular files and real directories are alphabetically listed, folders first.
Parent navigation reaches the filesystem root; autosave is the native save/autosave
subdirectory. No parks are installed, migrated or deleted. Staging .pending. files,
backups, symlinks and nonregular entries are excluded. Empty and directory iteration
errors are distinct; errors never leave a misleading partial listing.

D-pad up/down selects bounded pages; left/right shows successive path detail chunks.
A enters folders or selects a file; B returns to the originating slot/title/park
menu. Parent navigation refocuses the departed folder. Cancel-first active-park
discard review returns to the selected browser row. Selecting or rendering does not
import a park. Capture/revalidation records full-byte FNV-1a, size, timestamp and path,
rejecting removed/unreadable/nonregular/symlink and same-size/date replaced files.
This is an accidental-change guard, not cryptographic authentication or an OS lock
against simultaneous external replacement after the final check.

Load calls the ordinary Context::LoadParkFromFile. Explicit load ends native ghosts,
construction/access/pickup state and guest follow, then closes the old menu (clearing
reviews and transient filters). Failure reacquires a fresh menu rather than accessing
a deleted window; browser directory is restored with native failure feedback. Native
classification/parse failures can preserve the active park. After successful importer
parsing, Context deliberately marks failures after object loading/import as destructive
and falls back to title; Pocket does not promise rollback the native API cannot supply.
Native missing-object/version/ride errors retain their existing reporting.

## Validation

Validation/publication results are appended after observed completion. No ARM or
physical Brick results are claimed. Named Save As and design browsing remain pending
until their later independently validated phase sections.

Phase 1 Release native GUI/CLI builds PASS after regenerating stale build metadata
that omitted the existing Mazes.cpp module. Production source was fixed during each
build/test attempt. Filesystem tests PASS for real enumeration, case-insensitive
filters, staging/backup/symlink exclusion, empty/missing folders, parent recovery,
300-file directory and same-size/date replacement. CTest 4/4 PASS (input, launcher,
save preservation, file browsing). Asset-backed SaveLoadBrowsingTests PASS on African
Diamond Mine: controller menu dispatch, autosave/folder/paging, selection purity,
Cancel-first discard/return, corrupt native import preserving park, stale byte guard
and native autosave round-trip. Eight actual production 512×384 captures generated;
final inspection/package follow milestone closeout. Early corrupt importer failure
was exercised; late object/import fallback is audited native behavior, not a newly
injected failure test. Physical Brick, power-loss durability and hostile concurrent
filesystem changes remain unclaimed.
PresentationLifecycleTests PASS at Phase 1 source, including SDL controller/HUD,
construction/menu handoffs, existing native save-reload and shutdown cleanup.

## Phase 1 publication

Canonical a31d613fcc815de4d018a4078725b3b0ce28c85c; local safety
1311496d1e0fa1e9918da65ea7eef0eda03333ae; complete tree
8f2c47ce294ab71ac4c348770dfe387d424d3d2c. All seven blobs and complete tree matched;
current canonical parent 64751a6d and expected_sha/force=false fast-forward verified,
then branch ref read back. Esperanto excluded and checksum retained.

## Phase 2 — named native Save As

Save / Save and exit retain quick slots and add Named Save As. D-pad/A selects actual
folders or an existing filename; Save in this folder opens the shared 32-code-point
UTF-8-retaining keyboard. ASCII labelled keys, whole-code-point deletion, case/clear,
OK Apply and B/Back Cancel retain the established controls. Native .park extension
replacement applies; no new gameplay format/state. Unsafe path/control names and
trailing dot/space reject. Existing filenames can retain UTF-8; the keyboard does not
claim arbitrary Unicode input or a desktop-length filename editor.

OK creates a Cancel-first review showing exact basename/directory. A explicit Save
or Replace calls the existing ScenarioSave/WriteSaveAtomically publication helper.
A draft binds canonical chosen directory, POSIX device/inode and exact destination
file fingerprint (or absence). Checks before serialization and immediately before
rename reject stale files or replaced folders; UI selection cannot redirect the
reviewed write. Symlink/nonregular/unreadable targets and occupied staging paths
reject. Failed serialization/publication retains the prior destination. Native current
save identity changes only after success; save-and-exit calls ContextQuit only after
that same successful publication. No file migration/deletion is provided.

Release GUI/CLI build PASS. CTest 5/5 PASS, adding NamedSaveTests for UTF-8/extension,
invalid names, partial serialization preservation, changed destination during writer,
staging symlink preservation, nonregular/symlink targets and replaced directory.
Asset-backed SaveLoadBrowsingTests PASS including keyboard Apply, Cancel-first review,
named .park native round-trip/current identity, UTF-8 overwrite review, changed target
rejection and occupied-stage failure/retry preserving prior save. Production source
fixed during builds/tests. Save-and-exit success ordering is retained source behavior;
process exit is not asserted by this integration driver. Exact OS permission failures
under an unprivileged Brick account and power-loss durability remain unverified.

## Phase 2 publication

Canonical 40a6db57b3852a9d9b1193aa9719eccabbb786a4; local safety
0f4a4f41021cc4091f1400941b18c9bc0b641a70; complete tree
d051fdd3e24b97dec280df817b2014baa14e88ea. Eight blobs/full tree verified;
canonical parent a31d613f and expected_sha/force=false publication, ref read back.

## Phase 3 — native design file inspection and placement

Saved designs retains the native repository visual picker; X Browse files enters the
controller browser. Native repository Scan already recursively indexes the configured
RCT1, RCT2 and user track-design directories; it remains the authority for ordinary
indexed discovery. Folder inspection adds arbitrary readable locations without
installing, copying or migrating designs. It uses the same native desktop track filter
(.td4/.td6/.td7), TrackDesignImport, common Designs::FileEntry thumbnail path and
SavedTrackDesignPlaceAction::Eligible. No second repository/index/save format.

D-pad pages/long-path detail; A enters folders or opens native metadata inspection;
B returns to the repository picker and originating Build/placement. Inspector shows
native vehicle descriptor, actual track/maze/scenery counts, parsed native format and
real TrackDesignDrawPreview pixels. Metadata inspection does not begin placement or
change rides/accounting/research. Missing loaded vehicle/station, unresearched type/
vehicle, incompatible vehicle/family, unsupported family/geometry and native parse
rejection remain explicit. No object installation, unlocking or substitution occurs.

A Place revalidates full file identity and current native eligibility then invokes
existing BeginSavedDesign. Native ghosts, costs, scenery warnings/choices, rotation,
manual/automatic elevation, reviews, native construction and entrance/exit recovery
remain authoritative. Reopening the exact same file preserves height/rotation/scenery;
changed bytes invalidate cached previews and placement queries/review confirmation.
Y reopens the file browser while placing; B leaves placement through EndTerrain and
returns to the selected browser file. Native load/exit clears browser-return tool
context along with existing provisional state. No gameplay state is serialized.

Park browsing now also opens an inspection page before any load, including at title.
For .park only, desktop's preview-only ParkImporter::CreateParkFile/LoadFromStream
(skipObjectCheck=true)/GetParkPreview supplies the actual stored name/rating/guest/ride
summary. Legacy/corrupt/missing stored-preview states remain explicit without invented
summaries or compatibility claims. A separate Load enters active-park Cancel-first
review; failure restores both directory and selected path. Native preview parsing
never calls Import or loads objects into the active park.

Release native GUI/CLI builds and focused asset-backed SaveLoadBrowsingTests PASS:
browser/metadata purity, nested native exported TD6, preview, manual-height/rotation
reinspection, cancel-to-file return, stale same-path design and corrupt format.
The same driver covers park/Save As workflows, keyboard Back/Apply, real procfs native
serialization failure without changed save identity, failed Save-and-exit keeping the
park/no quit, and successful publication followed by native quit request. That extends
Phase 2's earlier ordering-only coverage. Captures use a native viewport initialized
at the imported wooden ride's real overall-view location; no fabricated world/UI.

## Preservation sweep

All fourteen independently linked native/SDL drivers PASS: SceneryPathParityTests,
ConstructionTests, PathActionsTests, PresentationLifecycleTests, ParkMapInformationTests,
GuestInformationTests, GuestCompletionTests, StaffCompletionTests,
FinanceResearchInformationTests, RideConstructionTests, SeatRotationTests,
FamilyOperationTests, MazeConstructionTests and MiniGolfConstructionTests.
Original assets supplied only to drivers whose actual argc contract supports them;
other drivers use native public-data fixtures. CTest 5/5 PASS: input, launcher,
save preservation, browsing and named save. Asset-backed DesignCapture,
RideConstructionCapture, MazeConstructionCapture and original Wooden
circuit/access/natural-guest CustomCoasterCapture all returned observed exit 0.
These are their real existing assertions, not every-family sustained-operation proof.
GUI objects were rebuilt after header/interface changes. Test links removed only
GUI Ui.cpp main and the linker dependency-output flag; production dependency records
were retained. Source fixed throughout each compile/test run. After the sweep, a
filename keyboard empty-state label was corrected and GUI/focused workflow rerun;
no simulation, input, action or lifecycle behavior changed in that label correction.

## Exact remaining ordinary gaps and limits

This milestone does not deliver 100% ordinary gameplay. Construction/variants,
atomic/station/inverted selected replacement, broader family sustained guest use,
scenery continuous-drag/automatic-Z/network review, shop appearance and other matrix
gaps remain. Flat/special design limitations and manual substitution/scenery warning
choice gaps are unchanged. Native repository installation/removal/rename/migration,
new-folder creation, delete, Windows drive picker and OS file-dialog integration are
outside this milestone. Parent navigation exposes readable configured/native mounted
locations on Linux; there are no separate location favourites/last-directory config
writes. Symlink entries are excluded. Existing filenames retain UTF-8 and long paths
have detail paging, but keyboard input is labelled ASCII and 32 code points.

File fingerprint checks are not OS locking or adversarial-race protection. Atomic
rename is Linux publication, not crash/power-loss durability; occupied existing staging
entries are preserved/refused and another filename/destination can be chosen. A failed
save never advances current-save identity or invokes quit. Native late object/import
failure can replace the active park and return to title; that audited guarantee is not
an injected late-failure test. Early corrupt parsing and real filesystem save failure
are tested. No AArch64/physical Brick, sustained dense-directory performance, native
network operation or exhaustive TD4/TD7 family coverage is claimed. Desktop formats
are enumerated; actual parser/eligibility decide support, without compatibility promises.

Final presentation hardening bounds long-path detail by 48 complete UTF-8 code points
per page, clamps navigation at both ends and tests non-ASCII reconstruction with no
skipped/split code points. CTest 5/5 and the complete asset-backed browsing/Save As/
design workflow were rerun after that presentation-only change. The focused driver
also injects an actual SDL virtual controller A press into Cancel-first discard review
and verifies one mapped action, cancellation and held-A suppression after transition.
Earlier fourteen-driver/four-capture preservation passes precede only these label/
path-detail presentation corrections; native simulation/action/placement/lifecycle
code remains exactly the swept code. No unrun repeat sweep is claimed.

Final focused driver returned observed exit 0 with actual SDL virtual held-A checks,
real native Save-and-exit failure/success ordering and all prior workflows. Native
placement handoff additionally uses the scenario's permitted maximum ParkSetLoanAction,
real UpdatePreview/Query at tile origin (704,704), native ghost and actual £32,461.10
quote, rendered with production Designs::DrawPanel/DrawCostBadge. No cash/terrain/
research/ratings/statistics are patched. All twenty-four final 512×384 PNGs are visually
inspected; unchanged images from the earlier inspected run were byte-hash matched,
and every changed/new final image individually inspected. No clipping/overlap/focus/
contrast/control collision requiring another production change remains. Intentional
ellipsis and bounded path paging remain. Final checksum/provenance ZIP follows.
