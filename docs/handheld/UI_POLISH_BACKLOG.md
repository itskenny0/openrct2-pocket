> 2026-10-20 update: [EVERYDAY-CONTROLS.md](EVERYDAY-CONTROLS.md) records the native-tested ground-height/Line/Fill correction, selected-path additions, controller quit routing and direct simulation controls. Historical deferred path findings below remain as original evidence; physical Brick retesting remains open.

Follow-up: [Compact handheld pass](COMPACT-HANDHELD-PASS.md) replaces the remaining report-style connection/design views and groups extended tool settings.

Current secondary-tool polish and verification: [FRIENDLY-TOOLS-PASS.md](FRIENDLY-TOOLS-PASS.md). Daylight/Slate and the existing ordinary-play fixes are preserved; physical Brick acceptance and a matching AArch64 package remain separate delivery work.


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
- Status: Unverified
- Source: Previous review; Phase 6 hints inspected in captures
- Reproduction: Exercise every Build/Adjust, tab, modifier, map and placement state.
- Suggested solution: Correct hints against actual dispatch; avoid hidden actions.
- Relevant files/components: Explorer.cpp; Menu.cpp; Dashboard.h

- Phase 8A verification: Phase 8A replaces fixed footer coordinates with shared measured two-lane glyphs; representative bounds and runtime captures pass. Full input-journey accuracy, camera access and mode transitions remain Phase 8B.

- Phase 8B verification: Desktop journey audit completed: shared stable footer zones, correct catalog Page/Camera and placement Zoom hints, existing tool-specific bindings retained. Sixteen native runtime drivers and logical-resolution captures cover representative journeys. Full translated/controller hardware matrix remains unverified. See `CONTROLLER-UX-PHASE8B.md`.

- RC1 verification: RC1 keeps one readable footer row in all captured ordinary cases, with secondary shortcuts in the park Controls guide. Bindings are unchanged; hardware/locale coverage remains open. See `CONTROLLER-FOOTER-BRICK-RC1.md`.

## S-06 — Shared

- Description: Back and focus restoration require remaining-system coverage.
- Severity: Major
- Status: Unverified
- Source: Previous review; representative Phase 6 automated returns pass
- Reproduction: Traverse submenus, cancellation, tool return and section/park return.
- Suggested solution: Preserve caller/tab/selection; review meaningful draft changes before leaving.
- Relevant files/components: Menu.cpp; TabbedActions.h

- Phase 8A verification: Phase 8B: comprehensive Back, draft-loss prevention, caller/tab selection and focus restoration. Phase 8A retains native behavior; passing representative journey tests do not establish full-system/device coverage.

- Phase 8B verification: Desktop nested returns, roster/tab focus, guarded research/duty exits, camera return and repeated cancellation verified in runtime drivers. Existing caller bookmarks retained. Complete ordinary-play/device coverage remains unverified. See `CONTROLLER-UX-PHASE8B.md`.

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

- Phase 8B verification: Coaster catalog X Camera shortcut and compact mode badge verified. A/B/Select return from Camera without building; Y/triggers cannot edit geometry there. Existing catalog Camera option retained. Ergonomics and complex-piece device QA remain open. See `CONTROLLER-UX-PHASE8B.md`.

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

- Phase 8B verification: PeopleUxTests verifies duty B -> Continue, tab -> Discard (native orders unchanged), queued Apply -> requested tab, explicit Apply/Cancel, patrol return and employee roster focus. Physical-device cancellation remains unverified. See `CONTROLLER-UX-PHASE8B.md`.

## G-01 — Guests

- Description: Needs could communicate direction and importance more clearly.
- Severity: Minor
- Status: Fixed
- Source: Previous review; current source explicitly states high hunger/thirst/toilet means more need
- Reproduction: Compare extreme native values and happiness in Needs tab.
- Suggested solution: Validate native meaning/thresholds before any severity treatment; optional enhancement tracked separately.
- Relevant files/components: PeopleTabs.h; GuestManagement.cpp

- Phase 8A verification: Needs preserve all six native normalized values, add higher/lower-is-better labels, and use native Guest.cpp thought/attention predicates for amber ! markers. Boundary tests cover toilet 159/160 and nausea 139/140; low/high runtime captures are included. No percentage severity bands or recommendations were invented.

- Phase 8B verification: Removed repeated per-card higher/lower captions in favor of one shared explanation. Six native values/meters and native attention predicates remain. Good/attention boundary captures and tests pass; no new severity thresholds. See `CONTROLLER-UX-PHASE8B.md`.

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
- Status: Fixed
- Source: Phase 8A source, native fixture tests and capture review
- Reproduction: Use the corresponding Phase 8A runtime fixture.
- Implementation / next step: Phase 8B must evaluate a discard review / draft preservation with consistent Back behavior. Phase 8A keeps the existing behavior and clearly labels the unsaved draft and consequence.
- Evidence: `CONTROLLER-UX-PHASE8A.md`; `captures/controller-ux-phase8a/`; `evidence/controller-ux-phase8a/`.



- Phase 8B verification: Fixed: research Back/Select/Start/tab changes review pending edits; Continue/B retains draft and focus, Discard leaves native values untouched, Apply waits for native callback. Review Cancel now preserves the plan. ManagementUxTests verifies navigation, native queue, repeated confirmation and recurring research accounting. See `CONTROLLER-UX-PHASE8B.md`.

## P8B-F1 — Dashboard

- Description: Unchanged Park actions paginated after visual-polish layout changes.
- Severity: Minor
- Status: Fixed
- Source: Phase 8B source audit and native regression fixtures
- Implementation / verification: The rail preview uses compact metrics/actions; the footer reserves its measured 28/44-pixel height. Native 00-dashboard.png shows all six actions with no page indicator. No actions removed.
- Evidence: `CONTROLLER-UX-PHASE8B.md`; `captures/controller-ux-phase8b/`; `evidence/controller-ux-phase8b/`.

- RC1 verification: RC1 retains all six dashboard Park actions with the shared 28-pixel strip; native 24-dashboard.png verifies no pagination. See `CONTROLLER-FOOTER-BRICK-RC1.md`.

## P8B-F2 — Shared footer

- Description: A/B prompt placement and two-row distribution varied between contexts.
- Severity: Minor
- Status: Fixed
- Source: Phase 8B source audit and native regression fixtures
- Implementation / verification: MeasureFooter anchors navigation left, A center and B right; secondary controls pack contiguously. Bounds tests verify no lost hints or overlaps at 512x384; native management, construction and confirmation captures reviewed.
- Evidence: `CONTROLLER-UX-PHASE8B.md`; `captures/controller-ux-phase8b/`; `evidence/controller-ux-phase8b/`.

- RC1 verification: RC1 supersedes the two-row distribution with grouped navigation/action/Back prompts and 28-pixel ordinary footers. Native capture assertions and shared bounds tests pass; long-label fallback is documented. See `CONTROLLER-FOOTER-BRICK-RC1.md`.

## P8B-F3 — Staff

- Description: Tab/Back navigation could silently reset changed duty drafts.
- Severity: Minor
- Status: Fixed
- Source: Phase 8B source audit and native regression fixtures
- Implementation / verification: Shared guarded draft exit preserves Continue focus, Apply native queue and Discard semantics; PeopleUxTests covers native orders and subsequent patrol/tab return.
- Evidence: `CONTROLLER-UX-PHASE8B.md`; `captures/controller-ux-phase8b/`; `evidence/controller-ux-phase8b/`.

## P8B-F4 — Construction camera

- Description: Camera mode could still build or change construction geometry.
- Severity: Minor
- Status: Fixed
- Source: Phase 8B source audit and native regression fixtures
- Implementation / verification: Coaster and maze A return to Build; Y/trigger editing suppressed in Camera. Coaster Select -> X Camera is explicit in catalog footer. RideFlowTests verifies draft/cash preservation and existing camera return.
- Evidence: `CONTROLLER-UX-PHASE8B.md`; `captures/controller-ux-phase8b/`; `evidence/controller-ux-phase8b/`.

## P8B-F5 — Guest Needs

- Description: Direction explanations repeated across cards and crowded meters.
- Severity: Minor
- Status: Fixed
- Source: Phase 8B source audit and native regression fixtures
- Implementation / verification: One shared two-line legend, compact native attention markers and 56-pixel cards preserve all native values. PeopleUxTests boundary cases and native logical-resolution captures reviewed.
- Evidence: `CONTROLLER-UX-PHASE8B.md`; `captures/controller-ux-phase8b/`; `evidence/controller-ux-phase8b/`.

## P8B-F6 — Visual catalogs

- Description: Long visual catalogs lacked shoulder paging despite management roster paging.
- Severity: Minor
- Status: Fixed
- Source: Phase 8B source audit and native regression fixtures
- Implementation / verification: L1/R1 moves one six-item page, clamps at boundaries and preserves selected identity during refresh. Shared picker tests and native track catalog capture verify controls.
- Evidence: `CONTROLLER-UX-PHASE8B.md`; `captures/controller-ux-phase8b/`; `evidence/controller-ux-phase8b/`.



## RC1-F1 — Controller footer

- Description: Ordinary screens still used an awkward two-row prompt layout.
- Severity: Minor
- Status: Fixed
- Implementation: Shared 28-pixel strip, stable navigation/action/Back grouping, explicit secondary/guide priority, full menu-entry hints in Start -> Controls. No remapping or font shrinking.
- Verification: Six standalone tests, 16 native drivers, per-capture single-row assertions, logical-resolution native screenshots and focused guide Back/return checks.
- Evidence: `CONTROLLER-FOOTER-BRICK-RC1.md`; `captures/footer-brick-rc1/`; `evidence/footer-brick-rc1/`.

## RC1-F2 — Build infrastructure

- Description: Vendor libpng12.pc referenced an include directory absent after setup.sh.
- Severity: Major
- Status: Fixed
- Implementation: Private SDK setup now creates libpng12/ with the matching vendor headers.
- Verification: Clean AArch64 configure/build, ELF architecture inspection and QEMU GUI/CLI version checks pass. Package CRC/modes/hashes/runtime closure have their own verification script.
- Physical status: Device graphics, controls, audio, sleep and SD-card saves remain unverified; this build fix does not close device QA.

## FP-F1 — Title resource lookup

- Description: Home background remained blank because the release installed title parks in data/title instead of the engine's data/sequence directory.
- Severity: Major
- Status: Fixed
- Implementation: package.py uses sequence; verify-package.py checks the actual path and validates the default park sequence ZIP, script and included parks.
- Verification: FirstParkFlowTests fails with zero installed sequences against RC1 layout, then plays the real animated title park after correcting the directory. Native 01-title.png reviewed at 512x384.
- Evidence: FIRST-PLAY-REFINEMENTS.md; captures/first-play-refinements/; evidence/first-play-refinements/.

## FP-F2 — Title layout and dirty redraw

- Description: Desktop resize positioning pushed the taller Pocket menu below the display; footer drawing depended on clipped render-target dimensions.
- Severity: Major
- Status: Fixed
- Implementation: Title resize callback owns Pocket bounds; shared embedded footer uses explicit window origin/size. Native logo/animation and slate actions have a dedicated startup footer.
- Verification: FirstParkFlowTests checks 512x384 title bounds, title-specific controls, pixel equality between full and tiled dirty redraw, scenario entry and repeated Back.
- Physical status: Changed title still needs a Brick retest.

## FP-F3 — Appearance choices

- Description: Colors exposed through numeric adjustment or a long text-only palette feel like debug settings.
- Severity: Minor
- Status: Fixed
- Implementation: Shared native swatch grid, applied marker/name and selected name; D-pad spatial movement. Other discrete appearance settings use named choices. Native review/cancel/apply and stale-context validation retained.
- Verification: FirstParkFlowTests exercises a researched Balloon Stall in Electric Fields; ShopAppearanceTests covers native apply/cancel, stale objects, held input and save/reload. Footer tests check grid edges and special-color short rows. Native captures reviewed.

## FP-F4 — Shop information hierarchy and wording

- Description: Shop inspection stacked small text and showed an unavailable financial sentinel as a monetary value; several screens exposed implementation terminology.
- Severity: Minor
- Status: Fixed
- Implementation: Shared compact metric cards, separate product details, five action rows and Open/Closed treatment; unavailable income/profit is labelled. Player-facing appearance/style/design/load text is shorter and avoids redundant native-action terminology.
- Verification: ShopAppearanceTests retains management/actions/accounting; FirstParkFlowTests captures all five shop actions without pagination. Native 06-shop.png and appearance/review captures checked at logical resolution.

## FP-01 — First-play density follow-up

- Description: The Build menu still exposes fourteen entries through eight visible rows; long selected path-surface names truncate, and flat-ride instructions repeat the footer.
- Severity: Minor
- Status: Fixed
- Source: FirstParkFlowTests native captures 04-build-menu.png, 05-paths.png and 10-flat-entrance.png; source audit.
- Suggested solution: Preserve existing routes while grouping Build categories into shared cards; full selected surface names in Adjust; compact Build/Entrance/Exit indicator. Plan focus and Back behavior before changing menu structure.
- RC3 implementation / verification: Shared icon cards expose all fourteen Build routes and spatial Path/Scenery/track-entry choices; wrapped material names/native previews, path mode strip, compact ride progress/thumbnail and wider catalog cells. FirstParkFlowTests verifies nested Back/focus, native construction/accounting and save/reload; PathBatchTests and RideFlowTests retain preview/cancellation/access behavior. Native 512x384 before/after captures reviewed in captures/handheld-flow-rc3/. The specific observed eight-row/truncation/repeated-stage defects are addressed; advanced More editors, physical fit and wide Fill occlusion remain open below and in existing entries.

## FP-F5 — Scenery / terrain color choices

- Description: Supported tool colors were textual cyclic adjustments rather than visible choices; scenery modulo-72 enum cycling could select special entries and omit normal colors whose enum values are 72/73.
- Severity: Minor
- Status: Fixed
- Implementation: Supported rows display swatches and open the shared native-order palette with A. D-pad only changes the local choice, A selects the tool setting, B/Y cancels to Adjust. Scenery cycling uses the actual ordered normal palette, excluding the special invisible/void choices.
- Verification: ConstructionUxTests checks draft isolation, cancel/use, all 72 normal choices, native costs, camera and held input. FirstParkFlowTests captures palette/mode on actual scenario scenery and verifies cancellation/accounting. Existing native actions, eligibility and queues remain authoritative.
- Evidence: HANDHELD-FLOW-RC3.md; captures/handheld-flow-rc3/; evidence/handheld-flow-rc3/.

## FP-02 — Advanced and device density review

- Description: Advanced track/design/connection and legacy More editors still contain technical text; catalog tile labels can abbreviate despite full selected detail. Build intentionally retains overlapping quick/full tools.
- Severity: Minor
- Status: Unverified
- Reproduction: Read long entries and advanced options during real Brick play; assess whether grouping the retained Build routes actually improves speed.
- Suggested solution: Target only observed friction, keeping full geometry/details and all routes discoverable. Do not equate a visual grid with verified physical comfort.
- Evidence: RC3 native captures and HANDHELD-FLOW-RC3.md scope/limits. Device readability/comfort remains S-02/S-04; wide Fill remains P7-01.

- Device evidence update: User reports RC1 launches and plays well after replacing the complete runtime folder. That confirms ordinary startup/gameplay on one Brick installation; audio, every button, suspend, performance and save-round-trip matrices remain unverified. The cause of the earlier invalid installed library files is unknown.

## RC4-01 — Line/Fill real-device construction failure

- Severity: Critical
- Status: Deferred by user request; reported device defect and root cause remain unresolved.
- Report: On flat ground, Line and Fill consume money without visible paths on Brick/NextUI RC3.
- Investigation: Existing native batch tests, full-live-tick variant and actual cursor-targeting probe pass against unchanged RC3. Native map elements, exact cash, rendering and persistence were checked. This does not close the reported hardware failure.
- Next step: Reproduce with the affected Brick park save when path work resumes. The user explicitly excluded paths from this continuation; this candidate does not claim a path fix.
- Evidence: RC4-PATH-INVESTIGATION.md; captures/rc4-investigation/; evidence/rc4-investigation/.

## RC4-02 — Park guest selection opens legacy inspection

- Severity: Major
- Status: Fixed in native runtime; Brick retest pending
- Report/source trace: Park input falls through ViewportInteractionLeftClick/GuestOpen, whereas the dashboard uses the tabbed guest workspace.
- Verification: Native GuestOpen now reaches the modern workspace with exact guest identity and park return. Dashboard roster return, needs/thoughts, tracking, follow and pickup pass PeopleUxTests. Staff and ride native entry points also use their modern workspaces. See RC4-REAL-PLAY.md.

## RC4-03 — Coaster construction preview/selection friction

- Severity: Major
- Status: Fixed in native runtime; Brick usability retest pending
- Verification: Restored native ghosts across provisional scopes/full simulation ticks and save operations; native thumbnail catalog, compatible piece groups, enlarged active thumbnail and spatial built-track selection. Native geometry, costs, undo and save/reload pass Rc4IntegrationTests; advanced editing passes SelectedTrackEditingTests. See RC4-REAL-PLAY.md.

## RC4-04 — Park events disconnected from ordinary play

- Severity: Major
- Status: Fixed in native runtime; Brick retest pending
- Source finding: Native news history and target-aware management routes already exist; ordinary-play discoverability and handoff still need improvement.
- Verification: Park Y and dashboard X open native news; current event ribbon hides during construction. Native subjects/category targets reach ride, guest/staff, research, finance, marketing, park and awards screens with caller-aware Back. Informational/stale events remain safe. Native dismissal archives the current item. See RC4-REAL-PLAY.md.

## BF-01 — Secondary menus still expose engineering language and parameter forms

- Severity: Major usability issue (review judgment)
- Status: Fixed in reachable native workflows; physical Brick verification remains unverified. See BRICK-NATIVE-AUDIT-IMPLEMENTATION.md.
- Scope: All 139 declared Pocket menu states, shared tabbed presenters and active tool overlays audited. Not every state/value combination was interactively played on a physical Brick.
- Findings: Saved-design descriptors/policy reports, coordinate-first entrance/exit management, generic multi-screen value editors, map metadata, technical research catalog, repeated help and free-change reviews.
- Implemented: Continuous clipped scrolling for management/secondary lists and visual catalogs; adjacent roster preparation; direct numeric/price Apply; spatial access selection; role-wide uniform palette; initial copy/density cleanup.
- Completed follow-up: Direct ordinary edits, secondary-page focus, native saved-design workspace/vehicle cards, larger map, visual research, staff previews/patrol controls, saved-park previews, compact news and visual controller guide. Sixteen native drivers and seven standalone checks pass.
- Remaining: Physical Brick/NextUI testing, locale/mod-object/full state-matrix QA. Paths remain deferred.
- Evidence: Current source and existing captures/rc4/, captures/handheld-flow-rc3/, captures/footer-brick-rc1/. Historical images are not new runtime verification.

Inventory: **57 entries**; 2 Confirmed, 19 Unverified, 35 Fixed, 1 Deferred.
