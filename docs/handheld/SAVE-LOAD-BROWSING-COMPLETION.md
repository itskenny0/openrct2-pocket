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
