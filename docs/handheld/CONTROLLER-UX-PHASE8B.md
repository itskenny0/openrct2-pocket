# Phase 8B — controller flow and final refinements

Based on canonical `390ff104` on `handheld/modern-ui-paths`. The pre-change
findings are in `CONTROLLER-UX-PHASE8B-AUDIT.md`. Existing native validation,
GameActions, eligibility, geometry, accounts and save format remain authoritative.

## Controller conventions

| Context | B / Cancel | Focus / state |
| --- | --- | --- |
| Park | Existing contextual cancel / close | No unrelated mode switch |
| Dashboard rail | Return to park | Camera retained |
| Management content | Return to meaningful caller / section rail | Existing tab, roster and native-target bookmarks retained |
| Nested inspection/editor | Parent screen | Review Cancel and B preserve the research plan |
| Changed duties or research plan | Continue Editing / Apply / Discard decision | Continue selected first; B resumes draft at its original row |
| Decision, native action pending | Wait for callback | Repeated A/B cannot submit twice or leave mid-commit |
| Destructive confirmation | Cancel | Existing Cancel-first selection retained |
| Scenery / terrain / path Adjust | Return to Build | Object, brush, height and direction retained |
| Path Line / Fill preview | Cancel selection before exiting tool | No purchase; review B returns to preview |
| Coaster piece catalog | Return to current builder | Existing piece and endpoint retained |
| Coaster or maze Camera | Return to Build | A also returns without building; geometry edits suppressed |
| Coaster Build | Existing native Undo | Intentional exception: B is not a global Exit; Start returns to ride |
| Staff patrol / peep placement | Existing return/cancel workflow | Employee/guest caller restored; patrol paint already uses native actions |

Duty/research decisions cover Back, tab change, Select and Start. Apply uses the
same native actions as explicit Apply; navigation waits for success. Rejection
keeps the decision and reports the native error. Discard only resets the local
draft. Unchanged drafts leave without a dialog. The existing explicit duty Cancel
and research Discard remain available. Research review Cancel now agrees with B:
it returns to editing, rather than silently resetting the plan.

## Camera and construction modes

- Park and active path/scenery/terrain/patrol placement retain D-pad pan and
  L1/R1 zoom. Their adjustment modes own D-pad/shoulders; B returns to placement,
  making camera movement available without resetting settings or holding a chord.
- Coaster: Select opens Pieces; X enters Camera. The existing Camera catalog item
  and optional stick-click mapping remain. Camera uses D-pad pan, L1/R1 zoom,
  X rotate; A, B or Select returns to Build. Y and L2/R2 cannot alter track geometry
  while moving the camera. A cannot accidentally purchase a piece.
- Maze: Select retains its camera toggle; A/B return to Build. Camera Y and triggers
  cannot remove cells or alter height. Native removal review retains priority.
- Single/Line/Fill and Build/Adjust headings remain. Coaster adds a compact BUILD /
  CAMERA badge. Specialized tool mechanics have not been forced into one mapping.
- Visual catalogs now support L1/R1 paging by six entries, clamped at boundaries.
  Selected identity still survives eligible-list refresh. Existing management
  roster paging and tab bookmarks remain.
- D-pad repeat stays 320 ms initial delay / 95 ms repeat. Face actions remain
  edge-triggered; transition suspension still requires release before rearming.

## Footer and layout

Shared measured hints anchor navigation left, A at 39% of logical width, and B in
the right zone. Secondary shortcuts pack contiguously into remaining space or a
second row. Simple screens use 28 pixels; complex screens use 44. The dashboard
uses the same measured height for drawing and roster paging. Button glyphs and
slate-green semantic colors are preserved. Zoom hints expose existing controls.

The dashboard Page 1/2 was a layout regression: a 44-pixel reserve plus the rail's
48-pixel metrics and 30-pixel actions paginated the unchanged six Park actions.
Only the rail preview now uses 40-pixel metrics and 26-pixel actions. No actions
or objective text were removed. Full management workspaces retain their hierarchy.

Guest Needs keeps six native meters and native attention predicates, replaces six
repeated direction captions with one shared two-line explanation, and uses the
existing `! Attention` marker only where supported. Happiness/energy rise toward
better conditions; the normalized other needs rise toward greater need. No new
thresholds or recommendations were introduced. Finance/marketing/native previews
retain their previous data and rendering paths.

## Verification and remaining risks

Verified on desktop x86-64 Release (`-O1`, GUI LTO), native software rendering,
512×384. Affected GUI target builds and the existing CLI target is executable.
Sixteen initialized native runtime drivers pass; five standalone CTest cases pass.
The build retains the existing local GCC warning exceptions for optional scenery
preview and unrelated engine diagnostics; this is not a warning-clean ARM build.

Coverage: PeopleUxTests (duty Continue/Discard/queued Apply, patrol, roster/guest
return and need boundaries), ManagementUxTests (Research Back/Select/Start/tab
protection, explicit review Cancel, native Apply/Discard, finance, marketing and
save/reload), RideFlowTests (piece camera shortcut, D-pad pan, camera geometry
isolation, A/B return, native Undo, access and caller focus), PathBatchTests
(Line/Fill transitions, cancellation, partial failure/cost), UiRedesignTests,
ConstructionUxTests, PathActionsTests, RideAccessRelocationTests,
SceneryPathParityTests, PauseResumeTests, PresentationLifecycleTests,
ConstructionTests, AccessShortcutTests, GuestCompletionTests,
GuestInformationTests and StaffCompletionTests. Shared bounds tests check all
prompts fit, single-row confirmation height and catalog paging boundaries.

Evidence: `evidence/controller-ux-phase8b/` contains logs and results.
`captures/controller-ux-phase8b/` contains 22 actual native 512×384 PNGs,
case mapping, contact sheet and a Phase 8A/8B comparison. Individual dashboard,
ride, catalog, path, adjustment, roster, need, finance, research, marketing and
confirmation captures were inspected at logical resolution. No overlapping footer
prompts were observed. Wide Fill occlusion remains P7-01; translated layouts are
not covered by these English fixtures.
Desktop native fixtures are distinct from physical-device testing. Brick button
comfort, display readability, long sessions, NextUI timing and translated layouts
still need device QA. No physical Brick verification is claimed.

Next milestone: final NextUI packaging, installation verification, and real-device
ordinary gameplay readiness. Preserve unresolved backlog entries, especially wide
Fill occlusion, optional sand harness diagnosis and translated/large-catalog QA.
