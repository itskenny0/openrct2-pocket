# Persistent UI polish backlog

This is the authoritative repository backlog for final UI refinement. Historical
screenshots are observations, not proof about current source. **Unverified** items
are verification tasks or possible improvements, not confirmed defects. Preserve
IDs and update evidence/status when tested. Optional features belong in
`UX_FEATURE_BACKLOG.md`. Phase 6/7 runtime PNG publication is now explicitly
authorized; capture paths below refer to repository evidence.

Severity indicates impact if reproduced. Status is Unverified / Confirmed / Fixed /
Deferred. Physical-device tasks remain unverified despite passing desktop fixtures.

## P7-01 — Paths

- Description: Fixed contextual panel can occlude part of a wide batch footprint.
- Severity: Minor
- Status: Confirmed
- Source: Phase 7 runtime 05-fill-preview.png
- Reproduction: Preview a wide Fill rectangle through the left panel.
- Suggested solution: Consider contextual relocation/collapse after device testing.
- Relevant files/components: Paths.cpp; ConstructionPanel.h

## P5-01 — Harness

- Description: Optional bright-sand capture crashed after a successful native terrain action; cause unknown.
- Severity: Major
- Status: Confirmed
- Source: Phase 5 diagnostic; grass/water fixtures pass later
- Reproduction: Re-run the optional bright-sand diagnostic under a debugger.
- Suggested solution: Isolate harness/render lifecycle before any engine change.
- Relevant files/components: PeopleUxTests.cpp; native offscreen renderer

## S-01 — Shared

- Description: Some secondary/contextual labels may remain too muted.
- Severity: Minor
- Status: Fixed
- Source: Previous screenshot review; Phase 5 contrast improvements accepted
- Reproduction: Compare dashboard and every active construction tool on dark/bright native scenes.
- Suggested solution: Measure contrast/readability without flattening hierarchy.
- Relevant files/components: PocketPalette.h / ConceptStyle.h; ManagementPanel.h; construction presenters

- Phase 8A verification: Shared secondary ink 19→20 and disabled ink 17→19. Verified representative 512×384 native captures across management and construction; physical readability remains S-02.

## S-02 — Shared

- Description: Physical-device font readability remains untested.
- Severity: Major
- Status: Unverified
- Source: Hardware unavailable during milestones
- Reproduction: Read all core screens on Brick at 512×384 logical / crisp 2×.
- Suggested solution: Device QA; adjust only proven readability problems.
- Relevant files/components: UI-STYLE.md; NextUI launcher

## S-03 — Shared

- Description: Positive, caution, disabled and destructive treatments need full-system review.
- Severity: Minor
- Status: Unverified
- Source: Previous screenshot-review checklist
- Reproduction: Compare open/closed/testing rides, unavailable actions and destructive reviews.
- Suggested solution: Retain text/icons with semantic colors; closed does not imply failure.
- Relevant files/components: ManagementPanel.h; PocketPalette.h / ConceptStyle.h

- Phase 8A progress: Phase 8A uses neutral Closed, mint Open, amber Testing, coral crashed/broken/down and destructive review accents, signed month-net ink, and draft markers. Representative native captures pass; a full-system/device state matrix remains unverified.

## S-04 — Shared

- Description: Focus visibility over bright park graphics needs device verification.
- Severity: Minor
- Status: Unverified
- Source: Previous screenshot review
- Reproduction: Use construction and selection over sand, water and dense rides.
- Suggested solution: Tune shared focus treatment only if reproduced.
- Relevant files/components: ManagementPanel.h; construction presenters

## S-05 — Shared

- Description: Contextual controller hints need a complete journey audit.
- Severity: Minor
- Status: Deferred
- Source: Previous review; Phase 6 hints inspected in captures
- Reproduction: Exercise every Build/Adjust, tab, modifier, map and placement state.
- Suggested solution: Correct hints against actual dispatch; avoid hidden actions.
- Relevant files/components: Explorer.cpp; Menu.cpp; Dashboard.h

- Phase 8A verification: Phase 8A replaces fixed footer coordinates with shared measured two-lane glyphs; representative bounds and runtime captures pass. Full input-journey accuracy, camera access and mode transitions remain Phase 8B.

## S-06 — Shared

- Description: Back and focus restoration require remaining-system coverage.
- Severity: Major
- Status: Deferred
- Source: Previous review; representative Phase 6 automated returns pass
- Reproduction: Traverse submenus, cancellation, tool return and section/park return.
- Suggested solution: Preserve caller/tab/selection; document intentional draft discard.
- Relevant files/components: Menu.cpp; TabbedActions.h

- Phase 8A verification: Phase 8B: comprehensive Back, draft-loss prevention, caller/tab selection and focus restoration. Phase 8A retains native behavior; passing representative journey tests do not establish full-system/device coverage.

## R-01 — Ride management

- Description: Some Overview states leave considerable unused space.
- Severity: Cosmetic
- Status: Fixed
- Source: Historical screenshot observation
- Reproduction: Inspect real rides with sparse and complete native statistics.
- Suggested solution: Consider useful existing native data without clutter.
- Relevant files/components: RideTabs.h; ManagementPanel.h

- Phase 8A verification: Overview now includes native reliability, customers and profit/hour from the existing statistics adapter. RideFlowTests and ride/02-park-ride-overview.png verify the populated layout.

## R-02 — Ride construction

- Description: Some native track thumbnails can look similar.
- Severity: Minor
- Status: Unverified
- Source: Historical screenshot observation
- Reproduction: Browse adjacent complex pieces with native assets.
- Suggested solution: Keep textual identification and preview; assess at physical size.
- Relevant files/components: ConstructionAdjust.h; track construction presenter

## R-03 — Ride construction

- Description: Selected-piece labels may truncate.
- Severity: Minor
- Status: Fixed
- Source: Historical screenshot observation
- Reproduction: Select long track names and translated labels.
- Suggested solution: Improve wrapping/selected detail if reproduced.
- Relevant files/components: CustomTracks.h; Menu.cpp

- Phase 8A verification: Selected track names wrap into two lines in a 216-pixel construction panel; picker details wrap across 296 pixels. Native previews/eligibility preserved. RideFlowTests and ride/07-coaster-build.png / 09-track-pieces.png pass. Long translated names remain I-04.

## R-04 — Ride construction

- Description: Complex coaster controls and mode indicators need end-to-end device QA.
- Severity: Major
- Status: Unverified
- Source: Previous review
- Reproduction: Build turns, banking, slopes, special pieces and resume/return.
- Suggested solution: Audit displayed hints and actual mappings together.
- Relevant files/components: Explorer.cpp; CustomTracks.cpp

## C-01 — Scenery / landscaping

- Description: Adjustment panels may be information-dense.
- Severity: Minor
- Status: Unverified
- Source: Previous screenshot review
- Reproduction: Inspect all object/brush shape/paint adjustments at logical resolution.
- Suggested solution: Prioritize current operation; preserve advanced settings.
- Relevant files/components: SceneryTools.cpp; AreaTools.h / TerrainMaterials.h

- Phase 8A progress: Phase 8A shortens adjustment instructions, keeps explicit Adjust headings, and improves shared label contrast. Representative scenery/terrain panels verified; broader catalogs/settings/device coverage stays open.

## C-02 — Scenery / landscaping

- Description: Build versus Adjust distinction needs physical verification.
- Severity: Minor
- Status: Unverified
- Source: Previous review
- Reproduction: Switch modes during repeated scenery placement and terrain edits.
- Suggested solution: Keep explicit mode labels and accurate hints.
- Relevant files/components: Explorer.cpp; construction presenters

## C-03 — Scenery

- Description: Native thumbnails across large catalogs need broader asset testing.
- Severity: Minor
- Status: Unverified
- Source: Previous review
- Reproduction: Browse multiple installed groups and large/animated objects.
- Suggested solution: Investigate missing/clipped native previews case by case.
- Relevant files/components: SceneryTools.cpp; TreePicker.h / ObjectPicker.h

## P-01 — Staff

- Description: Roster density, scrolling and clipping need populated-park/device review.
- Severity: Minor
- Status: Unverified
- Source: Previous screenshot review
- Reproduction: Browse long rosters with long names and every role.
- Suggested solution: Retain dense informative rows; verify focus and viewport.
- Relevant files/components: PeopleNavigation.h / Menu.cpp; ManagementPanel.h

- Phase 8A progress: Phase 8A fixes subtitle placement within compact roster rows and verifies populated native fixture paging. Hardware/long-name coverage remains open.

## P-02 — Staff

- Description: Duties tab lacks an explicit pending/unsaved summary.
- Severity: Minor
- Status: Fixed
- Source: Phase 6 source inspection: PeopleTabs::Staff sets role header and static duty instructions
- Reproduction: Toggle a duty without applying; compare header before/after.
- Suggested solution: Show draft/current state based on native baseline, without changing save semantics.
- Relevant files/components: PeopleTabs.h; StaffManagement.cpp

- Phase 8A verification: Duties compares the draft with native staffOrders, shows amber Unsaved duties, and retains the pending summary when no transient message exists. PeopleUxTests checks no native mutation before Apply and cancellation; people/04-employee-duties.png verifies the display.

## P-03 — Staff

- Description: Confirm duty cancellation never persists unintended changes.
- Severity: Major
- Status: Unverified
- Source: Previous review; existing PeopleUx automated cancellation passes
- Reproduction: Toggle duties then B, tab away, reopen, and inspect native orders.
- Suggested solution: Broaden integration/device checks; do not call passing case a defect.
- Relevant files/components: Menu.cpp; StaffManagement.cpp; PeopleUxTests.cpp

## G-01 — Guests

- Description: Needs could communicate direction and importance more clearly.
- Severity: Minor
- Status: Fixed
- Source: Previous review; current source explicitly states high hunger/thirst/toilet means more need
- Reproduction: Compare extreme native values and happiness in Needs tab.
- Suggested solution: Validate native meaning/thresholds before any severity treatment; optional enhancement tracked separately.
- Relevant files/components: PeopleTabs.h; GuestManagement.cpp

- Phase 8A verification: Needs preserve all six native normalized values, add higher/lower-is-better labels, and use native Guest.cpp thought/attention predicates for amber ! markers. Boundary tests cover toilet 159/160 and nausea 139/140; low/high runtime captures are included. No percentage severity bands or recommendations were invented.

## G-02 — Guests

- Description: Thought layout/readability needs broader content coverage.
- Severity: Minor
- Status: Unverified
- Source: Previous screenshot review
- Reproduction: Inspect multiple long, short and translated native thoughts.
- Suggested solution: Prioritize readable native thoughts/full-text access.
- Relevant files/components: PeopleTabs.h; GuestManagement.cpp

- Phase 8A progress: Phase 8A uses 44-pixel thought rows with two-line wrapping and retains A full text. Representative English fixture verified; long/translated content remains open.

## G-03 — Guests

- Description: Guest roster selected-row visibility and activity clipping need scale testing.
- Severity: Minor
- Status: Unverified
- Source: Previous review
- Reproduction: Browse large populations and long activity/name strings.
- Suggested solution: Keep dense rows; check visible focus and rendering cost.
- Relevant files/components: PeopleNavigation.h / Menu.cpp; GuestManagement.cpp

- Phase 8A progress: Phase 8A verifies focus and complete compact rows in a 1,024-guest native fixture, restoring the activity/happiness subtitle. Device and broader long-name performance stay open.

## I-01 — Integration

- Description: Management overlays require a variety of real populated park backgrounds.
- Severity: Minor
- Status: Unverified
- Source: Milestones use deterministic native fixtures
- Reproduction: Visit crowded, dark, bright, rainy and water-heavy parks.
- Suggested solution: Capture full journeys with original installed assets.
- Relevant files/components: test/handheld; dashboard and construction presenters

## I-02 — Integration

- Description: Complete ordinary-play journeys still need human playtesting.
- Severity: Major
- Status: Unverified
- Source: Automated tests cover representative journeys only
- Reproduction: Play build→manage→finance→research→marketing→park→save/reload.
- Suggested solution: Record concrete defects with exact input/state evidence.
- Relevant files/components: VALIDATION.md; Explorer.cpp; Menu.cpp

## I-03 — Integration

- Description: Brick/NextUI timing, held-D-pad feel and physical 2× scaling are unverified.
- Severity: Major
- Status: Unverified
- Source: No physical device available
- Reproduction: Run release candidate on actual Brick/NextUI.
- Suggested solution: Profile bounded batch paths, rosters and graphs; inspect crisp scaling.
- Relevant files/components: BUILDING.md; NextUI launcher

## I-04 — Integration

- Description: Remaining clipping/footer inconsistencies need translated-screen audit.
- Severity: Minor
- Status: Unverified
- Source: Previous review; representative Phase 6 English captures inspected
- Reproduction: Use long names/locales, disabled rows, errors and all footer modes.
- Suggested solution: Fix proven clipping centrally where safe.
- Relevant files/components: Dashboard.h; ManagementPanel.h; Menu.cpp

## P7-F1 — Paths

- Description: Back from directly opened surface grid unnecessarily opened settings.
- Severity: Minor
- Status: Fixed
- Source: Phase 7 source/runtime regression
- Reproduction: Open direct surface selector then B.
- Suggested solution: Direct caller returns active tool; settings caller returns settings.
- Relevant files/components: Menu.cpp; PathBatchTests.cpp

## P7-F2 — Paths

- Description: Destination label overlapped the fixed cursor.
- Severity: Minor
- Status: Fixed
- Source: Phase 7 runtime preview inspection
- Reproduction: Move a Line/Fill destination.
- Suggested solution: Endpoint badges sit beside cursor.
- Relevant files/components: Paths.cpp

## P7-F3 — Paths

- Description: Preview legend and native water rejection clipped at 512×384.
- Severity: Minor
- Status: Fixed
- Source: Phase 7 runtime captures
- Reproduction: Preview blocked water selection.
- Suggested solution: Short legend and four-line feedback; long translations remain QA.
- Relevant files/components: Paths.cpp; PathBatchTests.cpp

## P6-F1 — Finance

- Description: Native graph currency-axis labels clipped at the left boundary.
- Severity: Minor
- Status: Fixed
- Source: Phase 6 first runtime capture
- Reproduction: Open History with five-digit currency ticks.
- Suggested solution: Reserve label gutter; retain native renderer and numerical sample.
- Relevant files/components: FinanceInformation.h; Menu.cpp

## P6-F2 — Research

- Description: Discard action fell onto a second page in grouped priorities.
- Severity: Minor
- Status: Fixed
- Source: Phase 6 first runtime capture
- Reproduction: Open plan with all seven category rows.
- Suggested solution: 26-pixel action rows keep seven categories/review/discard visible.
- Relevant files/components: ParkManagementTabs.h

## P6-F3 — Finance

- Description: Retained ledger month label omitted the year.
- Severity: Minor
- Status: Fixed
- Source: Phase 6 source/runtime inspection
- Reproduction: Browse months spanning native years.
- Suggested solution: Append native calendar year; preserve absolute period bookmark.
- Relevant files/components: FinanceInformation.h

## P6-F4 — Marketing

- Description: Inline target and duration adjustments lacked contextual hints.
- Severity: Minor
- Status: Fixed
- Source: Phase 6 first runtime capture
- Reproduction: Open targeted campaign editor.
- Suggested solution: Show Left/Right and full-list controls under relevant actions.
- Relevant files/components: MarketingManagement.cpp

## P8-F1 — Shared roster

- Description: Compact row subtitles could fall below their row or disappear after footer resizing.
- Severity: Minor
- Status: Fixed
- Source: Phase 8A source, native fixture tests and capture review
- Reproduction: Use the corresponding Phase 8A runtime fixture.
- Implementation / next step: DrawActionRow now fits title/detail baselines within a 36-pixel visible row; staff/guest populated roster captures and native scrolling tests pass.
- Evidence: `CONTROLLER-UX-PHASE8A.md`; `captures/controller-ux-phase8a/`; `evidence/controller-ux-phase8a/`.

## P8-F2 — Staff

- Description: The transient Menu status overwrote the shared pending-duty summary with an empty string.
- Severity: Minor
- Status: Fixed
- Source: Phase 8A source, native fixture tests and capture review
- Reproduction: Use the corresponding Phase 8A runtime fixture.
- Implementation / next step: Preserve the shared summary unless a transient message exists; PeopleUxTests verifies pending native duty drafts and cancellation.
- Evidence: `CONTROLLER-UX-PHASE8A.md`; `captures/controller-ux-phase8a/`; `evidence/controller-ux-phase8a/`.

## P8-F3 — Paths

- Description: The first revision of the partial-build warning extended beyond the compact panel.
- Severity: Minor
- Status: Fixed
- Source: Phase 8A source, native fixture tests and capture review
- Reproduction: Use the corresponding Phase 8A runtime fixture.
- Implementation / next step: Shortened to Built paths stay. You pay. Native Line/Fill review/cost/partial-failure tests and runtime review capture verify the result.
- Evidence: `CONTROLLER-UX-PHASE8A.md`; `captures/controller-ux-phase8a/`; `evidence/controller-ux-phase8a/`.

## P8-F4 — Staff

- Description: The taller control bar initially moved the explicit Cancel duty row onto a second page.
- Severity: Minor
- Status: Fixed
- Source: Phase 8A source, native fixture tests and capture review
- Reproduction: Use the corresponding Phase 8A runtime fixture.
- Implementation / next step: Use 26-pixel duty actions, keeping all four handyman duties plus Apply and Cancel visible. Save/cancel dispatch is unchanged.
- Evidence: `CONTROLLER-UX-PHASE8A.md`; `captures/controller-ux-phase8a/`; `evidence/controller-ux-phase8a/`.

## P8-B1 — Research

- Description: B or changing a tab discards an unapplied research plan.
- Severity: Minor
- Status: Deferred
- Source: Phase 8A source, native fixture tests and capture review
- Reproduction: Use the corresponding Phase 8A runtime fixture.
- Implementation / next step: Phase 8B must evaluate a discard review / draft preservation with consistent Back behavior. Phase 8A keeps the existing behavior and clearly labels the unsaved draft and consequence.
- Evidence: `CONTROLLER-UX-PHASE8A.md`; `captures/controller-ux-phase8a/`; `evidence/controller-ux-phase8a/`.

Inventory: **37 entries**; 2 Confirmed, 16 Unverified, 16 Fixed, 3 Deferred.
