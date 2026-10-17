# Phase 5: staff, guests and shared readability

Canonical baseline inspected: `5adbfbeca421d2643e0005304b859e343a907417`, tree
`a7b3514adf79342bd45b49085255b3363f32838e`. Existing source, the Phase 2–4
design documents and native staff/guest checkpoints were inspected before edits.
1,944 present source/document blobs were verified against the canonical tree.
New work preserves that tree, including historical captures absent locally.

## Source audit

| System | Existing flow | Principal problem |
|---|---|---|
| Staff roster | Hire handyman, Hire mechanic, employee names, then other roles/filter/wages | Commands mixed with employees; role/activity invisible; other hiring and filters move farther away as staff increases |
| Employee | Ten actions including Locate, Orders, Statistics, Dismiss, Costume, Patrol, Name, Uniform, Pickup | Common duties and patrol require repeated inspector navigation; specialized tool return loses inspector focus |
| Orders | Separate native draft screen, toggle bits, Apply/Cancel | Mature validation already exists; navigation adds an avoidable screen |
| Patrol | Native painting, width/height, add/remove, clear review and employee return | Keep this tool; clarify feedback and restore management context |
| Guests | Summary → roster → twelve-action inspector → Needs/Thoughts/Spending/etc. | Names-only roster; frequently read needs and thoughts require Back and another selection |
| Guest filters | Groups/tracked/ride/name intersection, roster map, history, persistent follow | Preserve native grouping and map contracts while moving roster controls out of the long list tail |

Staff supports four native roles, automatic/manual hiring, orders where applicable,
role wages/counts, lifetime work counters, employment duration, patrol, uniforms,
entertainer costumes, rename, pickup, locate and dismissal. There is no supported
training action, individual wage editor or staff-follow command in these adapters;
none was invented. Guest follow and tracking are distinct native features.

## Delivered architecture and interaction

`PeopleTabs.h` composes native adapter data using the existing `TabFocus<4>`
framework. It is UI presentation and action-index mapping, not gameplay state.
`Menu.cpp` maps selected rows back to existing native action handlers.

**Staff: Overview / Duties / Patrol / Details.** Overview shows role, native wage,
current formatted activity and role-specific work counters. Locate and pickup
remain immediate actions. Duties is the existing orders draft directly in the tab:
A toggles, Apply submits the existing baseline-validated native action, Cancel
restores native values. Leaving the tab discards an unsubmitted draft; B leaves the
workspace without applying it. Security/entertainers explicitly show that they have
no configurable native order bits. Patrol summarizes actual coverage and enters the
existing brush editor. Details groups statistics, rename, uniform, costume and
Cancel-first dismissal. Inapplicable costume/uniform/pickup actions are visibly
unavailable. Dismissal states the employee and patrol consequences.

**Guests: Overview / Needs / Thoughts / Details.** Overview prioritizes native
activity, happiness, rides and tracking, with Locate, Follow/Stop and Pickup.
Needs presents all six existing desktop-normalized bars and percentages directly;
hunger/thirst inversion and native energy/nausea/toilet ranges are retained. Bars
are neutral measures, not invented health grades. Thoughts exposes the native
freshness-filtered list directly; A reads complete wrapped text and B restores the
Thoughts tab/action. Empty thoughts are explicit. Details shows native carried cash
and total spending, then full spending, inventory, preferences, ride history,
rename and tracking review. Existing no-money availability is retained.

**Dense rosters.** Staff has a persistent Hire / Filter / Counts toolbar, separate
from employees. The Hire selector preserves all four roles and automatic/manual
placement. Guest Map / Name filter / Tracked controls are persistent above the list.
Each roster displays five two-line entries at 512×384, with employee role/activity
or guest activity/happiness/tracking. There are no per-person decorative cards or
new population cap. The native names and targets remain authoritative. Guest
presentation formats names/details only for the visible page, while retaining the
full identity vector and action indices. Native roster changes retain an existing
selected identity. No runtime asset loading was added to production.

`ManagementPanel.h` adds reusable persistent roster controls, compact two-line
rows, neutral normalized meters, semantic header ink and destructive review marks.
`Dashboard.h` expands staff/guest content consistently with rides and renders
contextual tab/page hints. Other management adapters retain their specialized
behavior and shared shell.

`PeopleNavigation.h` holds copied native target identities, per-tab focus, roster
row, filters and origin during patrol/placement/follow handoffs. Returning through
existing staff/guest entry APIs revalidates the target and restores the workspace.
Guest group/ride roster parent context is retained across placement. Nothing is
serialized. Native target generations and action baseline checks remain in use.

## Controls and before/after sequences

| Context | Controls |
|---|---|
| Workspace | L1/R1 tabs; D-pad action grid or vertical duties/thought list; A select/toggle/read; B return to originating roster or park; Select section rail |
| Roster toolbar | Left/right chooses toolbar control; Down enters people; Up from first person reaches toolbar; A selects |
| Roster list | Up/down browse; L1/R1 jumps five entries; A inspects; B retains the existing parent, including groups/ride customers |
| Duties | A toggles draft; Apply submits native orders; Cancel/tab change discards draft; no implicit apply on Back |
| Patrol | Existing D-pad pan, A paint, Y Add/Remove, X rotate, shoulders zoom, Select brush/options; options B resumes paint, live B returns to Patrol tab |
| Placement | Existing native pickup/placement controls and cancellation; B returns to the owning workspace/filter/focus |
| Guest follow | Existing native camera follow; B recovers guest workspace, D-pad stops follow before manual pan |
| Confirmation | Existing Cancel-first focus and A/B semantics retained; firing/demolition receive destructive text/rule treatment |

No new physical binding or analog input dependency was added. Start/Select quit
chord and the accepted ride/path/scenery/terrain bindings remain intact.

Counts below start at the stated screen and count fresh D-pad/button presses.
Native confirmations are deliberately retained.

| Task | Before | After |
|---|---|---|
| First employee from initial staff roster | Down, Down, A: 3 | Down, A: 2 |
| Toggle first duty from employee Overview | Down, A, A: 3 | R1, A: 2 |
| Enter patrol from Overview | Six Downs, A: 7 | R1, R1, A: 3 |
| Re-enter patrol after live B | Navigate the inspector again | A: 1; Patrol tab/action restored |
| Guest Overview → Needs | Down, A: 2 | R1: 1 |
| Guest Needs → Thoughts | B, Down, Down, A: 4 | R1: 1 |
| Advance five people | Five Down presses; shoulders had no paging | R1: 1 |
| Map from first guest in N-person roster | N Down presses, A | Up, Left, Left, A: 4 |
| Other-role hiring | Scroll past N employees before the role selector | A on persistent Hire opens role selector |

Hiring a handyman/mechanic now includes the same explicit role selector used for
all roles, adding one selector step compared with the two old shortcuts. This is
the deliberate tradeoff for stable, separated hiring controls. Role, monthly wage,
placement and native eligibility remain visible before confirmation.

## Shared palette

The existing solid slate-green surfaces and native park palette are unchanged.
Tokens reference stable indexed colors; arbitrary RGB values are not injected into
world rendering. `ModernText.h` adds caution/destructive/disabled ink ramps.

| Token | Palette index / actual RGB | Use / measured contrast |
|---|---|---|
| Main panel | 12 / #2F4343 | Existing opaque surface |
| Raised surface | 13 / #3F5353 | Existing metrics |
| Primary | 21 / #EFF3F3 | 9.37:1 on panel; 7.30:1 on raised |
| Secondary | 19 / #B7C3C3 | 5.79:1 on panel; 4.51:1 on raised; previously 3.41/2.66 |
| Focus | 19 / #B7C3C3 | Dark selected text, strong focus edge; no color-only meaning |
| Positive | 151 / #93C7A7 | Open ride status, 5.47:1 on panel |
| Caution | 43 / #DBC787 | Testing ride status, 6.25:1 on panel |
| Destructive | 68 / #EB9F9F | Dismissal/demolition heading and rule; brighter than the first coral candidate |
| Disabled | 17 / #839797 | Deliberately dimmer than enabled secondary; dash/unavailable text retained |

Contrast uses full-coverage default-palette RGB and WCAG relative luminance. It is
not a guarantee for every anti-aliased edge pixel or native font fallback. Primary
and secondary remain distinct. Semantic labels always remain: Closed is neutral
slate, not automatically a warning; Open is mint, Testing amber. A small status mark
preserves semantic distinction when localized text uses native font fallback.
Confirmation names and explicit consequences remain readable without color.
Footer and construction instructions inherit the brighter secondary token. Opaque
surfaces keep text contrast independent of dark/bright park colors.

## Validation and reproduction

Build: affected GUI translation units rebuilt using the AArch64 GCC 16.2 / glibc
2.26 SDK; GUI and CLI linked successfully against the retained unchanged native
engine archive. This is not a clean rebuild of all engine translation units.
Python runner syntax/help and staged whitespace checks are also verified.

`PeopleUxTests` initializes a native owned-land scenario with public object
metadata and the user's original graphical assets, executes real native actions,
and renders the native viewport plus production UI at 512×384. It checks hire
Cancel-first/duplicate handling and exact charge, duties draft/cancel/apply, patrol
paint/options/return, dismissal cancellation, role roster/page/focus, staff pickup/cancel, native needs,
thought detail return, spending, native tracking, follow recovery, pickup cancel,
tracked-filter placement return, map return and a 1,024-person last-page inspection.
50 large-roster model snapshots took about 17–30 ms under QEMU across runs. This
measures presentation snapshots, not full frame rate, scans, AI or Brick performance.

The fixture explicitly loads `rct2.peep_names.original`, as native park import/default
object loading normally does. Omitting it leaves generated real names blank; that
fixture omission was corrected rather than inventing display names. The user’s
original Data/ObjData assets remain private outside the repository. No proprietary
asset binaries are committed. Setup remains the Phase 4 private RCT2 root with
`Data/g1.dat`, `ObjData`, and optional Scenarios/Tracks; public engine/object data
remains separate.

Reproduce with an existing GUI build:

```sh
python scripts/handheld/verify-ui-redesign.py --driver PeopleUxTests \
  --build /absolute/build --data /absolute/openrct2-data \
  --profile /private/test-profile --rct2-assets /private/RCT2 \
  --captures /absolute/captures
```

Use the existing `--runner-prefix` for QEMU plus matching loader/libraries.
RideFlowTests supplies asset-backed open/closed ride, coaster and destructive
confirmation regressions. ConstructionUxTests preserves Phase 3 tool controls.
The preservation suite additionally covers native all-role staff/costume/wages/
patrol/name/uniform, guest tracking/groups/placement/history/follow/save-reload,
SDL controller release/duplicate behavior, paths, rides and selected-track editing.
See the accompanying evidence JSON/logs for exact driver outcomes.

Captures are native runtime fixture renders under QEMU/offscreen SDL, not physical
Brick screenshots or generated mockups. Staff/guest layouts, five-row scrolling,
focus/disabled states, needs meters, construction panels and confirmations were
inspected at 512×384. The existing staging profile still requests 1024×768 at scale
2 with DPI inference disabled; integer scale uses nearest-neighbour. No change was
made to scaling. Actual display mode, crisp 2× output and physical controls remain
unverified on a Brick.

## Known limits

An optional **bright-sand capture diagnostic** exposed a headless fixture failure:
native SurfaceSetStyleAction executes successfully, then the subsequent offscreen
viewport capture faults. Root cause is not established; no production terrain or
renderer code was changed to hide it. The passing acceptance run uses native grass
backgrounds. Set `POCKET_PEOPLE_BRIGHT_SAND=1` with PeopleUxTests to reproduce;
its failed diagnostic log is retained separately. Bright-sand runtime capture is
explicitly incomplete. Native headless terrain validation/accounting tests pass.
The shared opaque surfaces' contrast calculation applies independently of the
background, but it is not a substitute for this missing runtime check.

Offscreen SDL reports its existing invalid hardware-renderer Resize warning; PNGs
use the software X8 native viewport. Live focus-dependent terrain brush previews
and sustained commercial-scenario play are not fully covered by this fixture.
Long localized content uses the established ellipsis/native wrapping fallback;
not every language or maximum-length name was playtested. Staff/guest rare settings
retain focused subordinate screens. Legacy unused adapters/capture drivers remain
for later cleanup; they are not the new standard entry flow.

## Next milestone

Apply this interaction architecture to finances, research, marketing and remaining
park management: prioritize actionable summaries, common loan/admission/funding/
campaign controls, native consequences and return focus. Preserve native ledgers,
research eligibility and marketing accounting. Keep park information/map/news
specialized where a list or generic action grid would obscure the task.
