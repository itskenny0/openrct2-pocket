# Native sprite coverage and controller fit

Follow-up to accepted Daylight/layout work at `5475ab6`, on
`experiment/native-menu-feel`. Daylight, Slate, page order, action indices,
controller bindings, previews, focus restoration and single-row footer remain.

## Useful native artwork added

- Staff: patrol and details tabs; pickup, rename, uniform/costume actions; the
  six native handyman/mechanic duty icons, including both On/Off labels.
- Guests: native thought and inventory tabs; pickup, inventory, spending, name,
  tracking, preferences and ride-history actions.
- Rides: dispatch timer, maintenance wrench, operations, connected access,
  demolition confirmation and actionable-news management/locate buttons.
- Finance/research: ledger/controls/project/funding tabs, loan/admission/history/
  ledger controls, research catalog and campaign controls.
- Park: map, news, awards, objectives, history, open/close and common returns;
  native park-entrance artwork replaces terrain artwork on dashboard Overview.

Artwork follows the existing engine SpriteIds and the corresponding native
Guest/Staff/Park/Finances/Research window choices. It uses loaded native G1/G2
sprites, not substitutes or generated images. Native sequences keep their exact
frame counts and only animate when selected.

Overview now uses its section context: finance gets coins, guests get people,
staff get staff artwork, and rides retain the ride icon. Guest Details uses
inventory artwork; staff Details uses native statistics artwork. A maintenance
tab uses the native wrench rather than generic gears.

## Readability and navigation

Common actions are matched against exact existing English UI labels, including
explicit duty toggle states. There is no fuzzy/substring action guessing.
Roster cards bypass action-icon matching so custom person/ride names cannot
be mistaken for commands. Existing native object/ride/employee previews remain.

Action rows now measure their complete label before reserving icon space.
Where artwork would cause needless shortening, the label wins and the icon is
omitted. The existing narrow-tab rule does the same for tabs. Native artwork
is clipped inside its allocation; it cannot paint over neighboring buttons.
No labels are replaced with icon-only controls and no new input step is added.

Text-only numeric fields, arbitrary names/thoughts, toggles without an appropriate
native symbol and cramped controls intentionally remain textual. This is useful
sprite coverage, not a claim that every possible asset has been inserted.

## Validation

The temporary workspace was cleared between turns. The packaged source was
restored, then compared against the published branch; all 1,824 compiled/test/
build inputs matched before editing. Original game assets were restored for
native screenshots. No historical source was published over newer changes.

Native x86-64 Release/O1 application built successfully. The restored local QA
build disables optional network/HTTP/TTF/OpenGL/audio codecs/scripting and uses
private Ubuntu dependencies. Two existing GCC optional-value/null-pointer
warnings required warning-only treatment in the local build commands; engine
source was not changed for those warnings. This is not a production ARM build.

Seven native initialized suites passed: BrickAuditTests, ManagementUxTests,
PeopleUxTests, RideFlowTests, EverydayControlsTests, SceneryPathParityTests and
PathCursorTests. Three suites also passed with saved Slate preferences:
BrickAuditTests, ManagementUxTests and PeopleUxTests. Nine standalone checks
passed. These exercise actual controller input frames (including the SDL virtual
controller in EverydayControlsTests), native state/actions, money, save/reload,
held scrolling, focus/Back, draft cancellation and footer bounds.

New checks load every frame of representative new native icon families and
compare a narrow action's complete rendered label with a plain reference,
proving artwork does not consume its required text space. Existing card-boundary
and two-line-label pixel regressions remain. Staff Details is additionally
visited through controller tab navigation and returns to Overview before the
existing duty-draft tests.

Screenshots are actual 512x384 native rendering with original park graphics.
No physical Brick/NextUI or ARM/QEMU checks were performed in this pass. Long
locales, mod assets and the complete hardware/state matrix remain follow-up QA.
This does not change the installed package or `handheld/modern-ui-paths` branch.
