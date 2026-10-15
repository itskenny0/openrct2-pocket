# Meaningful icons and entrance/exit shortcut — 2026-10-15

Continues published 96641ed05725aae30acc92fc3acaeed32b12ea06,
complete tree 355b53d9e74a38066c572556e82a08389b8375e1.
The user's physical Brick feedback confirms the preceding slate build functions
well. This update addresses repeated folder icons and buried access placement.

## Delivered

Unspecified MenuPresentation icons now render as plain text with the icon gutter
removed. Real file/repository folders keep their explicit folder icon. Clear-area
settings use native G2 trees, large scenery, footpath, walls and path-furniture
sprites for their actual categories; Width/Height remain plain numeric controls.
Build icons now cover all fourteen rows in the correct order, without a Back
icon on Rides or folder icons on shop/settings actions. Palette and controls stay
as in the slate build. No new assets or invented gameplay symbols are required.

For eligible closed rides with missing access, **Place entrance / exit...** is
the first action in both quick ride management and Dashboard ride inspection.
The Access / paths page also provides the same direct handoff instead of telling
the player to return elsewhere. Confirm resumes existing native FlatRides access
placement and locates the ride. Its NextStation logic selects the first real
station missing entrance or exit, then advances through remaining missing points.
The normal path/queue handoff remains available after access exists. Open/ineligible
rides do not acquire a placement action or get silently closed.

This only exposes the delivered missing-access workflow. Existing entrance/exit
relocation/removal is not delivered. Native station/family support, exact-context
placement guards, ghosts, pending-action/input protection, costs and save cleanup
remain unchanged. Menus do not place access or paths as a side effect of inspection.
No simulation, GameAction or save-format changes.

## Validation and package

The actual AArch64 GUI is rebuilt from all 25 affected GUI objects (including
header dependents) using preserved CMake target commands, then fully relinked.
Production source stays fixed during compilation; native engine archive, CLI,
runtime and input mapping are unchanged. Generated --version retains its old
engine recovery reference; the package manifest identifies this source tree and
actual executable hash.

AccessShortcutTests uses native loaded/researched Carousel construction actions,
then exercises actual menu Confirm/Back, quick ride, Access/path and inspection
handoffs, exact map/cash/station preservation, original Close routing and stale
allocation rejection; native entrance commit followed by first-action recovery of
the remaining missing exit without changing the entrance/cash. Optional captures draw actual production windows at 512x384
with G2 assets and the native fixture; blank park background is intentional.
They are not physical-device screenshots or original-asset park-render evidence.

PauseResumeTests and PathScenarioUsabilityTests are relinked against the final
target GUI objects and rerun through QEMU with shipped SDL offscreen. CTest 5/5
uses the existing unchanged input/launcher/save/file driver binaries. No physical
Brick, original private-asset regression sweep or dense-park measurements are
available here. SDL offscreen headless window creation emits an invalid hardware
renderer warning; production X8 window/component captures are rendered separately
and inspected, not evidence of foreground SDL/device rendering. Broader historical
suites are not claimed as rerun.

The update archive includes the actual stripped AArch64 game, retained runtime,
input helper and launcher, complete matching source, GPL, test logs and inspected
native component renders. Merge payload contents into the installed executable
directory (normally Tools/tg5040/OpenRCT2 Pocket.pak/game) and launch.sh into the
.pak root. Preserve all installed game assets, config and .userdata saves.

Publication follows the canonical parent above with expected_sha / force=false,
each changed blob verified and complete source-tree comparison, then branch and
native commit parent/tree readback. Esperanto bytes remain unchanged and excluded.
