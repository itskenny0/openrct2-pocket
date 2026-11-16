# Terrain and save vertical slice

The reference is the supplied "OpenRCT2 Brick UI Concept B": blue panels, filled yellow focus rows, short lists, contextual command strips, and the original park graphics remaining dominant. This is the first functional Build and persistence slice, not a conversion of every management window.

## Controller loop

1. On the existing controller title menu, choose New Game and select a scenario.
2. Select opens Build. Choose **Terrain: raise land** or **Terrain: lower land**.
3. D-pad uses the established direct viewport pan. The ground tile beneath the fixed reticle is outlined with OpenRCT2's native map selection. This does not move a mouse pointer.
4. A applies one whole-tile height change. Y switches raise/lower. X rotates and L/R zoom as before. B leaves editing; it is not an undo. Costs, ownership, clearance and paused-state restrictions come from the existing LandRaiseAction/LandLowerAction query/execute pipeline.
5. Start opens the park panel. Choose **Save and exit**, then one of three named Pocket slots. Existing slots require replacement confirmation, initially focused on Cancel. The process quits only after serialization and atomic file publication succeed.
6. Relaunch and choose the title menu's saved-game option. The native slot picker loads the saved `.park` through OpenRCT2's ordinary park importer. Loading from an existing park requires confirmation before discarding unsaved changes.

The park panel also provides Resume, Build, Save, Load, and explicitly confirmed exit without saving. Start+Select remains the existing emergency exit without saving.

## Persistence

Slots are `Pocket-1.park` through `Pocket-3.park` in the configured user saves directory. The NextUI launcher's existing persistent user-data path remains unchanged. These are normal OpenRCT2 park files; the save format, simulation and renderer are untouched.

The saver writes to a staging file beside the destination and renames it after a successful `ScenarioSave`. Failed serialization, exceptions, or failed publication leave the existing slot untouched. This is atomic replacement on the Linux target, not a claim of power-loss durability. Three slots avoid needing a virtual keyboard for this first test. Other save files are not enumerated yet; no existing user parks are deleted or migrated.

## Validation boundaries

- Earlier explorer source remains physically Brick-tested according to the user's report.
- The new terrain/slot workflow is source-level progress and requires a matching AArch64 package before any hardware test.
- Automated save-publication tests cover fresh saves, replacement, partial serialization failure, thrown serialization failure, and rename failure while preserving the prior file.
- Controller and launcher tests remain applicable.
- This is not yet a game-state save/load round-trip test or a visual/hardware test. Original installation assets are not available in this environment.

The fixed reticle overlay remains the original small viewport overlay. A separate 34-pixel contextual strip provides edit mode, prospective cost or action failure, and controls. Blue/yellow list styling is shared by Build, pause and save/load panels. Desktop behavior remains available outside handheld mode. Paths, other build categories, a financial dashboard, and the full North Star HUD are later slices.

## Repository continuity

The full foundation was successfully imported into `keeganwhitacre/openrct2-pocket`. `main` checkpoint `5882ad9` has exactly the tree of source checkpoint `9e334fb`, with both histories preserved. Tags `brick-explorer-proof1` and `pocket-menu-native-0.2` remain recoverable. The older checkpoint report's GitHub blocker predates that completed import.

## Native browsing / named Save As follow-up — 2026-10-14

SAVE-LOAD-BROWSING-COMPLETION.md supersedes the three-slots-only discovery limitation.
Slots/autosave remain native; real folder/park/autosave browsing, preview-only .park
metadata, explicit native load/recovery and reviewed named .park Save As are delivered.
Current-save identity and quit request occur only after publication. Occupied staging
entries now refuse/preserve rather than following/overwriting them. Native importer
late object/import failures retain native title fallback rather than promised rollback.
