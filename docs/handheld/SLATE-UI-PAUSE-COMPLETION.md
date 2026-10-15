# Slate UI and explicit park resume — 2026-10-15

Continues published 8b6debe7935517194c83507a4e652509318557ef,
complete tree ef4b0b916c40d700079e896b58dc24bdb336b9d7.
The matching local safety source remains preserved. The user explicitly chose
Neutral slate for this pass, superseding the earlier blue/gold styling preference.
Existing Esperanto bytes remain excluded from milestone commits.

## Pause correction

A pre-paused park reproduced the reported issue: Resume park only closed the
menu, and onClose released only the pause acquired when that window opened.
A park already paused on entry therefore could not resume through that action.

Resume park and B/Start closing the root park menu now call a shared resumePark
method. It invokes native PauseToggle only when GAME_PAUSED_NORMAL is set,
releases the menu's ownership and closes normally. Modal and saving-track pause
reasons remain untouched. Non-Resume tool/inspection closes preserve a pause
that existed before opening the menu. Owned-menu cleanup now checks the actual
normal bit; it cannot reintroduce normal pause merely because another engine
pause reason remains active. No simulation-state repair or save-format change.

## Presentation and navigation

Shared PocketPalette, Panel and ConceptStyle use the stable native grey/slate
ramp. The world's palette and actual scenery/ride colours are unchanged. HUD,
tool panels, Dashboard, confirmation cards, native scenario selection highlight
and action strips use the quieter palette. Selected text remains dark on a
light grey focus surface. Native tree and folder sprites replace the related
presentation glyphs; existing native terrain/path previews remain.

The oversized circular face-button badges are replaced with 16-pixel tiles.
Start/Select use plain readable text labels, without boxed hardware mockups.
The D-pad indicator is smaller and crisp. The path footer was re-spaced after
rendered inspection exposed overlapping Select/Surface and B/Back labels.

Build and path-mode lists retain focus when returning within the same menu.
B from the root Build list returns directly to the park; path choices retain
their parent relationship. Existing per-section navigation, shared keyboard,
Cancel-first reviews, input rearming and pending action guards remain. This is
an initial shared-style/navigation pass, not a claim that every specialised
screen's visual hierarchy or number of clicks is fully polished.

## Verification and exact limits

PauseResumeTests fails on the previous GUI objects with the normal pause still
set. It passes on final target GUI objects under QEMU7.2/shipped SDL offscreen:

- Explicit Resume and B across running, normally paused, modal-paused and
  saving-track-paused fixtures; only normal pause is cleared.
- Cleanup does not add normal pause when another reason remains.
- A non-Resume tool close preserves a pre-existing normal pause.
- Native paused .park save/reload followed by explicit Resume.
- Build/Paths back hierarchy and restored queue-choice focus.

The final AArch64 target rebuild compiles all 25 dependent GUI objects with the
preserved CMake target commands/dependency graph, then relinks the actual game.
After final header spacing and scenario colour edits, affected objects were
rebuilt before relinking/tests. Production source stayed fixed during each
compilation. The native engine archive, CLI, runtime and controller mapping
remain unchanged. Generated --version still names the previous recovery build;
the update manifest records the new canonical source tree and binary SHA256.

PathScenarioUsabilityTests and SceneryPathParityTests pass against final target
objects. The latter uses its documented argc=3 headless contract; asset/real
window branches are not run. Existing unchanged input/launcher/save/file driver
binaries pass CTest 5/5. No physical Brick, original private assets, foreground
renderer or exhaustive asset-backed preservation run is available here.

Four genuine native 512x384 component renders (exploration, paths, Dashboard and
confirmation) were regenerated after final linking and all inspected at native
size in a contact sheet. A first render linked against older Menu objects exposed
mixed old/new inline UI definitions; relinking the preview after the complete
rebuild removed that stale result. Final frames show consistent neutral surfaces,
readable focus/controls and no footer overlap. These are clearly labelled component
fixtures with sample presentation data and no live park, not hardware or gameplay
evidence. They contain no private assets and are included for visual review.

The update ZIP includes rebuilt GUI, retained launcher/runtime/input helper,
complete source, GPL licence, test logs, component previews and SHA256 provenance.
Merge payload into the installed executable directory and launch.sh into the
.pak root; retain data, RCT2 assets, config and .userdata saves. Publication uses
expected_sha and force=false from the canonical parent above, complete tree
comparison and branch/commit readback.
