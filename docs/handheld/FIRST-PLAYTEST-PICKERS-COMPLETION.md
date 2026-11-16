# First Brick playtest — path picker and scenario navigation (2026-10-15)

Continues canonical 582216077dfebe80d56dd2a56cbc56a7f39d692f,
complete tree c446d729b346d4e4f57ed7424e5afdacf39fc375.
The vanished checkout was recovered from the exact saved source archive into an
absent directory, with complete Git tree equality verified and local recovery
safety commit eeb1b2688beb78dda8cc1c7750bb5a1a71c1edda before compilation.
Existing scratch artifacts and the preserved Esperanto edit remain untouched;
Esperanto SHA256 is e37ba559b4d9d761e74f2bb284d1ec762b813e789655c0495a882c83651890b5.

## Path selection and native sprites

Select in an active free or connected path tool opens the shared image-grid
picker. D-pad chooses, A applies the focused native surface and returns to
placement; B returns to Path Options without applying. B again resumes placement.
Y opens Path Options, including direction, slope, height and undo where supported.
The Change surface row is now Choose surface... and opens the same grid.
The existing accidental cycling shortcuts no longer select surfaces.

The grid uses loaded FootpathSurfaceObject/FootpathObject::DrawPreview, their
native sprites and native names, with bounded paging and full-name detail.
Normal versus queue flags and editor-only restrictions are retained. A selected
choice is checked again against the current active mode, object type, slot and
descriptor before changing gFootpathSelection. Removed or changed choices remain
recoverable; cancel does not apply selection or buy a path. Existing placement,
height, connection, GameActions, native prices and save formats are unchanged.
Legacy mode retains its existing legacy-only object choices rather than inventing
cross-adapter behavior. Railings and other object selectors are not generalized
into new grids in this update.

## Scenario access and native previews

Controller navigation previously discarded every locked row before moving,
which made later groups appear unreachable. It now visits locked and unlocked
rows, shows a native G2 lock sprite and explains the lock. A still checks the
native lock flag before invoking the load callback. X explicitly toggles the
native scenarioUnlockingEnabled setting and saves it to the native config.
The current row remains selected when still present. Unlocking On means native
progression limits apply; Off permits ordinary scenarios without progression.
Native Mega Park hiding and other visibility rules remain in force.

L/R source tabs, category headings and native scenario parsing remain. Controller
movement now requests the existing native preview loader, as mouse movement
already did. Pocket chooses the native minimap, fitting the 148-pixel preview
pane; desktop retains its screenshot/minimap preference. No park loads merely
from selection or preview. The footer reserves space for controls and lock detail.
This is focused native-sprite polish for these screens, not completion of a
whole-interface art/style pass.

## Actual validation

All 107 GUI objects depending on the changed UI headers were compiled with the
preserved target CMake commands/dependency graph, then the real AArch64 GUI was
relinked. The exact canonical native engine archive and unchanged CLI are reused.
Production source stayed fixed during each build; a self-contained header include
and final scenario rendering changes were followed by rebuilding their objects.
A temporary recovered-file metadata mismatch caused pragma-once redefinition;
compiling against stable file metadata resolved it. The recovered SDK lacked
libdl/libpthread development symlinks; restoring links to the existing matching
private shared libraries resolved final linking without substituting host libs.
The compiler remains GCC16.2/AArch64/glibc2.26 and the shipped private runtime.
The stripped final GUI passes QEMU7.2/private-loader --version.

PathScenarioUsabilityTests links against the final target GUI objects and passes
under QEMU with the shipped SDL offscreen backend and public engine data:

- Normal/queue image-grid activation, focus, A apply and B cancellation.
- Connected-tool mode preservation on apply/cancel.
- Removed descriptor and wrong normal/queue context rejection.
- Native placement query purity, exact cash delta and selected surface on native
  .park save/reload.
- Expert/locked-row navigation, blocked locked Start, explicit native setting,
  unchanged repository count and exact same expert file after toggling.

The scenario fixtures are native exported .park files. That format deliberately
indexes as Other, so the legacy-lock test prepares only its in-memory fixture
index with ScenarioSources::TryGetByName metadata, exactly as S6Importer does,
then selects the native first-source tab. This is test setup, not a production
indexing change, original scenario archive or claim of full legacy import coverage.

SceneryPathParityTests passes in its argc=3 headless mode under the same target
runtime; its asset-backed/real-display branches are not run. SDL renderer warnings
are expected without a display. Existing unchanged handheld test binaries pass
CTest 5/5 (input, launcher, save preservation, file browsing, named save).

No physical Brick, original private assets, native foreground display or broad
asset-backed preservation sweep is available in this run. Hardware visual and
controller retesting remains required. No fabricated screenshots or statistics.

## Update

OpenRCT2-Pocket-Brick-Pickers-Fix.zip contains the rebuilt game and retains the
previous working launcher, runtime fixes and input helper. Its payload merges
into the directory containing the installed executable, normally
Tools/tg5040/OpenRCT2 Pocket.pak/game. launch.sh belongs in the .pak root.
Keep assets, config.ini and .userdata saves. Source, GPL licence, test logs and
binary/source provenance are included. Exact published identifiers and hashes
are in the package manifest and final handoff. Publication uses a guarded,
non-force fast-forward from the canonical parent above and verifies the full tree.
