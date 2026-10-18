# First-play UI refinements and audit

Continues canonical RC1 `b14df36` on `handheld/modern-ui-paths`. The user reports
that RC1 with the complete runtime replacement starts and plays well on the Brick.
This is useful ordinary-play evidence, not exhaustive hardware certification.

## Startup defects and fixes

Three distinct defects were verified:

- `package.py` installed title parks in `game/data/title/`; the engine's
  `DirId::sequences` resolves to `game/data/sequence/`. The native title test found
  zero sequences before the directory correction and plays a native title park
  after it. Package verification now checks the real lookup path, default script,
  ZIP CRC and an included park. This explains the blank background.
- The native resize pass reapplied desktop `height-182` placement after the
  Pocket menu grew to 210 pixels, pushing rows/footer off-screen. Title menus
  now receive their virtual resize callback; the Pocket title owns the full
  logical bounds. Desktop positioning remains unchanged.
- `DrawPanelCommands` used a dirty render target's dimensions as screen dimensions.
  It now draws a shared footer relative to explicit window bounds, independent
  of the current clipping rectangle. The title has its own accurate single-row
  Navigate / Select / Exit prompts, with native OpenRCT2 branding and animation.

No title parks or commercial graphics were replaced by mockups.

## Appearance and shop readability

A shared eight-column swatch grid renders the actual native palette ordering.
It shows the selected name, the currently applied name, a two-tone applied marker
and a distinct focus border. D-pad movement is spatial and stops at grid edges,
including the two-entry final row when special colors are enabled. Color changes
remain drafts until the existing review/apply action. B never applies them.

Both the dedicated shop editor and ride appearance editor use this component.
Discrete appearance options use named choices instead of Increase/Decrease.
Numeric prices and mechanical settings retain their existing controls. Native
object eligibility, available channels, cheat-gated special colors, stale-context
checks, callbacks, costs and serialization are unchanged.

Shop inspection uses the existing management component: compact native metric
cards, product/price/sales detail and five clearly focused actions. Missing
income/profit stays Unavailable; it is not presented as a negative monetary value.
All five actions fit at 512x384. Compact metric text is contained in its card.

Player-facing appearance/design/load/style messages now use labels such as
Apply change, Appearance updated, Cost, Refund and Track connections rather than
repeating implementation terms such as native action. Warnings about changed
objects, station renumbering, irreversible work and placement failure remain.

## Text-heavy UI audit and next refinements

These are focused proposals, not implemented navigation changes or gameplay features.
The priority order follows ordinary first-play frequency.

| Surface | Observation | Focused solution | State |
| --- | --- | --- | --- |
| Title | Blank background, clipped menu, misplaced hints | Correct sequence resource path, resize ownership and clipping-safe footer | Fixed in native test |
| Shop appearance | Color selection represented as numeric adjustment or a long text palette | Shared native swatches and named discrete choices | Fixed in native tests |
| Shop inspection | Several unstructured metric lines above actions | Shared metric cards and ordinary action rows | Implemented; native capture reviewed |
| Build menu | Fourteen legacy entries, an eight-row viewport, and overlapping routes to terrain/scenery | Present existing categories with shared cards; keep advanced/legacy operations in their existing option panels. Plan focus/Back migration first; do not silently remove routes | Confirmed density; proposal |
| Path placement | Long surface names truncate in the compact panel; repeated controls compete with geometry | Two-line selected surface name or a surface thumbnail plus full name in Adjust; keep mode and height visible, put extra controls in the existing guide | Confirmed capture; proposal |
| Path Line/Fill | Several lines explain geometry, markers and actions | Keep footprint count and total quote prominent; use the existing mode badge and one concise consequence block. Preserve explicit partial-success warning | Existing warning retained; proposal |
| Flat ride placement | Text-only stage, duplicated Start/B return instruction | Use a compact Build / Entrance / Exit step indicator and existing ride preview; put return controls in the footer once | Confirmed source/capture; proposal |
| Track block / design details | Technical geometry and compatibility strings remain dense | Summary first; expose exact native geometry in the existing connection/details view. No loss of eligibility information | Partial wording cleanup; proposal |
| Price editing | Numeric adjustment is appropriate but product scope still needs explanation | Preserve Left/Right prices and one short shared-price label; avoid adding confirmation text to every row | Existing behavior retained |
| Research / staff drafts | Meaningful Apply/Discard consequences must remain explicit | Keep Unsaved draft, compact choices and safe B return; these are useful decisions, not decorative text | Regression retained |
| Finance / guests | Native units, periods and need interpretation are essential | Keep native values and shared legends; reduce repeated labels only after focused captures | Existing Phase 8B hierarchy retained |
| Home game tools | Desktop toolbox uses a dropdown and some tools expect desktop interaction | Separate controller-complete ordinary play from advanced desktop tools; requires a bounded controller/accessibility follow-up | Unverified controller coverage |

Build-menu grouping and step-strip changes are deferred to a small follow-up so
this release does not turn a final refinement into another navigation redesign.
The screenshots preserve evidence rather than marking these observations fixed.

## Native first-park flow

`FirstParkFlowTests.cpp` uses the supplied commercial assets locally, the actual
software compositor, production menu handlers, native catalogs and GameActions.
It plays a native title sequence, compares full and clipped dirty redraws, opens
New park, returns with B, opens the selector again, switches native source tabs,
and starts Electric Fields. It enters Build -> Paths through the menus, places
three legal paths with exact quoted charges, places two researched vendors,
opens a color-capable shop, navigates swatches, reviews and cancels, places a
researched Merry-Go-Round, completes its entrance and exit, returns to ride
management, and saves/reloads a native park, then returns to the animated title without gameplay hints.

The test searches legal world tiles through the same queries; it does not unlock
research, waive costs, change ownership, clear obstructions, or bypass eligibility.
Direct tile targeting is a test driver convenience, not a physical D-pad playtest.
This is not proof that the resulting isolated ride has a complete operational
queue network. `RideFlowTests` separately covers native operational/access workflows.

The title fixture must scan installed sequences explicitly because the test
initializes headlessly before entering the real title scene. A missing sequence
is a hard failure. Native captures are 512x384; no desktop enlargement is used
as a substitute for logical-resolution inspection.

## Verification and delivery

See `evidence/first-play-refinements/` and `captures/first-play-refinements/`.
Existing unrelated backlog observations remain open. The new screens and the
updated package still need physical Brick testing. RC1's working runtime repair
does not establish the cause of the invalid files previously installed on the SD.
Updates use a fresh `game` folder and preserve original assets and `.userdata`.

Native GUI/CLI and Cortex-A53 AArch64 GUI/CLI builds pass. Six standalone tests
pass, including footer bounds and 72/74-color grid navigation. ShopAppearanceTests,
RideFlowTests, ConstructionUxTests, ManagementUxTests and PauseResumeTests pass.
The FirstParkFlowTests title, dirty-redraw, scenario, path, vendor, palette,
entrance/exit and native save/reload checks pass with the supplied local assets.

The release uses a manifest-driven archive helper. It hashes bytes as they are
archived and excludes unlisted staging/transfer debris; the verifier rejects
unlisted/duplicate files and checks ELF64 segment bounds as well as architecture.
The game checksum list supports verification of the SD-card copy. These measures
are distinct from establishing why the user's earlier installed copies differed.

The binary version stamp identifies local build snapshot `dcc5f17`. Publication
provenance separately identifies the canonical source commit; C++ gameplay/UI
sources are unchanged by the later capture/test/packaging documentation checkpoint.
