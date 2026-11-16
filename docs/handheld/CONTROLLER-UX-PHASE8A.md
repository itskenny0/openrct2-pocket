# Phase 8A — visual polish and handheld readability

Built on canonical `d4e6993db87f8f9f26a45a2b7d429071dcd5b53d`.
Phase 6 management and Phase 7 Single/Line/Fill paths are preserved. This milestone
changes presentation and layout; native simulation, transactions and controller
navigation remain authoritative.

## Shared changes

- `Panel.h`: measured two-lane controller hints, glyphs for face/shoulder/system
  buttons and D-pad, primary A emphasis, consistently rightmost B in the primary
  lane. All supplied hints remain visible. The first lane groups movement/action/
  Back, the second supplementary actions. `L/R` is rendered as `L1/R1`; `L2/R2`
  remains distinct. No timed rotation, hidden shortcut page or invented mapping.
- `Layout.h`: reserve 44 logical pixels for two 20-pixel lanes, replacing the
  28-pixel strip. Layouts, management capacity and feedback anchors use that shared
  reservation. Construction panels remain compact.
- `PocketPalette.h`: secondary text uses stable palette entry 20 instead of 19;
  disabled labels use 19 instead of 17. Slate surfaces, focus and native world
  palette are retained. Existing mint/amber/coral tokens are applied to real states.
- `ManagementPanel.h`: explicit meter directions, native attention metadata,
  optional semantic metric ink, readable full rows and wrapped thoughts. Short
  roster rows fit both title and subtitle. Destructive reviews have a coral title/
  rule and a marked destructive row, retaining Cancel-first focus.
- `Dashboard.h`, construction/placement presenters, object picker and management
  wrappers use the shared hint renderer instead of fixed x positions.

The footer is designed for 512×384. Layout assertions cover representative
Dashboard, ride, coaster, Single/Line/Fill, adjustment, patrol, guest, history and
confirmation combinations. Glyph widths use the same font advances as labels.
Longer translated labels still require a separate language audit; no claim is made
that arbitrary text fits the two lanes.

## Gameplay-specific presentation

| System | Result |
| --- | --- |
| Rides | Overview adds native reliability, customer count and profit/hour when provided by the statistics adapter. Compact action rows keep the ordinary controls together. Open is mint, Testing amber, Closed neutral, and crashed/broken-down states coral; text remains explicit. |
| Track construction | A 216-pixel panel and two-line selected name distinguish similar pieces. The 320-pixel catalog wraps selected details across 296 pixels. Native preview sprites and eligibility are preserved. |
| Paths | Single/Line/Fill retain their established explicit titles and dispatch. Multi-tile review says “Build these paths?”, “Stops at the first problem.” and “Built paths stay. You pay.” Native sequential cost/partial-success behavior is unchanged. |
| Scenery / terrain | Adjustment labels and instructions are shorter and brighter. Existing groups, catalogs, height controls and Build/Adjust transitions remain intact. |
| Staff | Pending duties compare the draft to native orders, show amber Unsaved duties and an explicit Apply/B consequence. Empty transient feedback no longer erases that summary. Four handyman duties plus Apply and Cancel fit together. |
| Guests | All six native normalized values are retained. Happiness/energy are labeled higher-is-better; hunger/thirst/nausea/toilet lower-is-better. Amber ! markers use native thought/attention predicates, not invented percentage bands. Thought rows wrap, ordinary English native typography uses the readable UI face, and A retains full native text. |
| Finance | History uses more plot space and avoids an unnecessary negative half-axis for entirely nonnegative retained samples. Signed month-net values use semantic ink without an overall park-health inference. Native series and graph renderer remain unchanged. Monthly ledgers and weekly histories stay explicitly labeled. |
| Research | Unavailable priorities remain readable with explicit / none and disabled markers. Amber Unsaved draft and its discard consequence remain visible. All seven priorities, review and discard fit the grid. |
| Marketing | Each active campaign has a type line and a target / remaining-weeks detail line. Native campaign state, targets, durations and accounting are unchanged. |

Guest attention source: `src/openrct2/entity/Guest.cpp` native tired-thought
(energy ≤70 and happiness <128), hungry (hunger ≤10 without food/drink), thirsty
(thirst ≤25 without food/drink), nausea ≥140, and toilet ≥160. The raw hunger/
thirst direction differs from the desktop-normalized displayed need percentage;
this milestone preserves the existing adapter inversion. Happiness has a continuous
native meter and no invented severity cutoff. These markers describe native
attention conditions, not recommendations or guarantees that a thought is emitted.

## Verification

Linux x86-64 GUI and CLI build and version commands pass. GCC 13.3, SDL 2.30,
ICU 74.2; Release with `-O1`, UI LTO, scripting enabled. HTTP/network/OpenGL/TTF
and optional audio codecs are disabled in this verification configuration.
Build-local warning demotions were required for existing Vehicle.Collision
null-dereference and SceneryTools optional-value warnings; no engine source was
changed to suppress them. The stringop-overflow warning demotion is retained.
This is not an AArch64 package or a physical Brick test.

All **16 native runtime drivers** pass:
PeopleUxTests, ManagementUxTests, RideFlowTests, PathBatchTests, UiRedesignTests,
ConstructionUxTests, PathActionsTests, RideAccessRelocationTests,
SceneryPathParityTests, PauseResumeTests, PresentationLifecycleTests,
ConstructionTests, AccessShortcutTests, GuestCompletionTests,
GuestInformationTests and StaffCompletionTests.

The subsequent thought typography/Overview fit refinements are revalidated with
PeopleUxTests and RideFlowTests, including updated actual captures. New checks
cover footer bounds/no omitted actions, distinct shoulder labels, visible research
controls, complete Overview actions, native guest meter directions and attention
boundaries, English thought rendering, unsaved duty display, and unchanged native
draft/cancel behavior. Existing suites cover action accounting, queued callbacks,
Line/Fill partial failure, native cost, save/reload and controller journeys.

All **5 standalone CTest checks** pass: input, NextUI launcher/config preservation,
save preservation, file browsing and named saves. They exercise the launcher
fixture; they do not establish actual NextUI hardware behavior.

### Runtime capture evidence

The delivered capture bundle contains **74 before and 76 after PNGs**, all
512×384. They use the production UI, native renderer, original installed G1 art
and official objects. They are deterministic desktop offscreen runtime fixtures;
financial/research records and guest/ride populations are seeded for assertions.
They are not mockups, mature-park histories or physical-device screenshots.
Original proprietary asset archives and DAT files are excluded from publication.
The repository includes all 76 after captures, five matched before cases, and the
comparison sheets; the complete before set is in the delivered capture bundle.

- [All 15 required visual cases](captures/controller-ux-phase8a/contact-sheet.png)
- [Five matched before/after comparisons](captures/controller-ux-phase8a/before-after.png)
- [Individual capture mapping](captures/controller-ux-phase8a/cases.json)
- [Tests and hashes](evidence/controller-ux-phase8a/verification.json)

Inspected at their native logical size: Dashboard; Ride Overview and Operations;
coaster construction and piece picker; Line and Fill previews and review; scenery
and terrain adjustment; staff roster/duties/patrol; guest needs, attention extremes,
thoughts and 1,024-guest paging; finance history; research controls; active
marketing; destructive review. The footer remains in its reserved region in these
cases. Composite sheets retain native-size images; viewing only a scaled sheet
was not used as the legibility check.

Visual review caught and corrected compact roster subtitles, a hidden duty Cancel
row, an overlong path warning, an erased pending-duty summary, English thought
fallback typography and unnecessary Overview pagination. Shared styles are kept
subdued against native park graphics.

## Remaining work / Phase 8B

`UI_POLISH_BACKLOG.md` preserves all prior IDs and unresolved observations. Its
37 entries are 16 Fixed, 2 Confirmed, 16 Unverified and 3 Deferred. Fixes are scoped
to their recorded evidence; physical/translated checks remain open.

Phase 8B should address controller consistency, Back/cancel behavior, camera
access, Build/Adjust transitions, caller/tab focus restoration and the research
B/tab discard behavior. Do not treat the current warning as prevention of draft
loss. The fixed path panel can still overlap wide batch footprints (P7-01), and
the optional bright-sand harness crash (P5-01) remains untriaged. Actual Brick
font/contrast/scaling, held-input timing, crowded parks, broader object catalogs,
long names/translations and hardware performance remain unverified. Proposed new
gameplay features remain separate in `UX_FEATURE_BACKLOG.md`.
