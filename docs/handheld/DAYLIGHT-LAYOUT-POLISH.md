# Daylight layout polish

This focused follow-up keeps the accepted Daylight palette, native sprites,
selected-tab animation, press feedback, existing routes and controller bindings.
It is on `experiment/native-menu-feel`, after `67e9743`; the installed package
and stable `handheld/modern-ui-paths` branch are unchanged.

## Confirmed fixes

- Compact Build cards did not reserve enough space for a second text line and
  descenders. `Remove scenery` visibly extended below its button. Cards now
  budget the art/text gap for the measured label and clip their contents to the
  button interior. Single-line labels retain the larger artwork. Remove scenery
  and Shops & facilities remain fully readable on two lines.
- Narrow tabs now favor their complete label over adding an icon that would
  force needless abbreviation. Spacious tabs retain the native artwork.
- The role-wide uniform grid had a duplicate scope subtitle drawn over the
  heading. The redundant line is removed; the role title and Apply to role
  footer remain explicit.
- Guest attention captions overlapped their percentages. Directional metric
  cards now reserve a separate warning line above the meter, in both good and
  attention states. Native values, warning predicates and meter directions are
  unchanged.
- Focused unavailable actions used selected white text on an unselected light
  surface. They now retain disabled ink, including their subtitles. Native
  availability rules remain unchanged.
- Native finance/park graph labels were forced white despite light panels.
  Their native dark-background flag now follows the theme. Localized positive
  money uses readable native black ink in Daylight instead of luminous green;
  ASCII semantic labels retain their theme colors. Native formatting, graph
  data, ranges, missing samples and Slate rendering are preserved.
- Search and Save As keyboards used a narrow eight-column panel, abbreviating
  Space/Delete/Case/Clear/Done/Back. They now use the full-width keyboard shell,
  with the existing key order, text-entry behavior and cancel semantics.
- The maze's redundant `SEL Camera | START Ride` hint extended beyond its
  panel. It is now a bounded `SELECT Camera`; Start/Ride stays in the footer.
- Terrain's operation shortcut floated over the park below the panel. It is
  now bounded inside the existing panel's final line.

## Evidence and verification

150 pre-fix native screenshots were reviewed across management, staff/guests,
construction, scenery, path tools, catalogs, maps, keyboard, quit and pause
screens. This is representative coverage, not proof of every state/locale.
The after set contains 166 actual 512x384 native screenshots, including all 14
Build focuses and the Path and Scenery category grids. Original park graphics
are loaded by initialized fixtures; no mockups or recolored screenshots.

The native x86-64 application built successfully. Seven native driver suites
passed: BrickAuditTests, ManagementUxTests, PeopleUxTests, SceneryPathParityTests,
EverydayControlsTests, RideFlowTests and PathCursorTests. After the final
construction-caption changes, the four affected suites were rerun and passed;
the final Build/category capture additions also passed. All nine standalone
checks passed. Saved Slate preferences passed BrickAuditTests and
ManagementUxTests, covering alternate-theme layout and graph labels.

BrickAuditTests additionally renders compact cards against sentinel pixels in
normal, selected and pressed states. It checks that drawing never escapes the
button and that two-line labels retain visible second-line ink. Controller
input frames visit every Build card and the nested category grids without
executing construction. Existing runtime footer assertions still require one
28-pixel row; native action/cash/cancel/focus regressions remain active.

No ARM/QEMU or physical Brick test was performed for this visual-polish trial.
Deliberate abbreviation of long catalog names retains full selected details.
Long translations, every mod object/error string, crowded backgrounds and the
full physical display/state matrix remain follow-up QA. The preview archive is
for visual review, not an installable device update.
