# Phase 6 — finance, research, marketing and park administration

Implemented after Phase 7, from canonical `cc9c291470e2edf659063eef6f0b8a6c9770a00a`
(tree `6c28bdc26bb7a2c7c06c38c8cf5d0fc61c1a6fbe`). Historical checkpoints are
context, not rollback targets. Phase 7 path geometry, Line/Fill, review semantics,
validation, previews and controls are unchanged.

## Source audit and resulting architecture

Before this milestone, Finance overview mixed loan/admission and report links;
ledger and each financial history were separate pages. Research had a project
summary followed by an eight-control vertical funding/priority editor. Marketing
separated overview, campaign types, target browser, duration and review. Park's
administrative action list mixed status/admission with maps, objectives and
records, with admission detouring through Finance.

The native adapters were already substantial: `Finances`, `FinanceInformation`,
`ResearchManagement`, `ResearchCatalog`, `MarketingManagement`, `ParkOverview`
and `ParkInformation`. They remain the authority for data, eligibility, formatting,
queries and actions. New `ParkManagementTabs.h` projects those adapters into
controller workspaces; it stores no separate simulation state.

Shared `ManagementPresentation` adds optional metric columns, row heights,
right-aligned report values, reserved plot space and contextual navigation hints.
Defaults preserve existing callers. Dashboard content uses its full logical width
when focused; the section rail remains available through B/Select. Existing
ride, staff, guest and specialized construction patterns are retained.

## Workspaces and controls

| System | Organization | Direct interaction |
|---|---|---|
| Finance | Overview / Ledger / History / Controls | L1/R1 tabs; Ledger Left/Right month; History Left/Right weekly sample, A next native graph |
| Research | Project / Funding & priorities | L1/R1 tabs; two-column seven-category grid; A toggle; X funding; Review applies together |
| Marketing | Active campaigns / New campaign | L1/R1 tabs; all six native types visible; Left/Right target or duration in editor; A full target browser or review |
| Park | Park / Objectives / Records | L1/R1 tabs; direct admission editor; existing native map, news, awards and history |

D-pad moves focused actions; A selects; B returns to the previous context, then
section rail, then park. Per-tab focus is retained across subpages. Review dialogs
start on Cancel. B from a review returns to its draft; explicit Cancel abandons
the draft. B or changing tabs from the research plan discards unapplied changes,
with that consequence stated on screen. Research X only changes the local draft.
Specialized map, catalog, construction and patrol controls are unchanged.

### Finance

Overview distinguishes native cash, loan, park value and month-to-date signed
net. Income, expense and current weekly profit have explicit periods/labels;
these values are not presented as an invented overall health score. Ledger shows
all 14 native signed expenditure categories and an absolute native month/year,
retained through monthly table shifts. Long reports scroll six rows at a time.
History retains the engine's finance renderer and existing weekly arrays for
cash less loan, profit and park value. A sample guide and numerical selected
sample supplement the native graph; no history is synthesized in production.
Loan and admission editors preserve native queries/actions and restrictions.
UI success waits for the native queued callback, avoiding stale results or
repeated submissions. Park-origin admission returns directly to Park focus.

### Research

Project shows the native project, stage, expected date, funding and monthly cost.
The progress bar explicitly describes **this stage**, using the native 0x10000
stage threshold from `Research.cpp`; it does not predict overall project completion.
Finished-all and unavailable native states remain distinct. Funding and all seven
categories are visible together. Categories with no native eligible projects are
disabled, with text. Current/unsaved state is explicit. Review uses the existing
native funding/priorities action; reserved priority bits and native recurring
costs remain intact. The existing sprite invention catalog is retained.

### Marketing

Active rows show native type, target and remaining weeks. No effectiveness or
historical cost is inferred. New campaign shows six native types with availability
reasons and weekly prices. Target and duration can be adjusted inline; A retains
the full target browser. Exactly one eligible target can be preselected by identity;
multiple targets require selection. Existing native total cost, review, query,
eligibility and queued callback remain authoritative. Successful start restores
the active campaign, including when other campaigns exist. Expired campaigns
refresh/clamp focus by native type.

### Park

Status/guests/rating/value stay with common actions. Objectives expose native
scenario text/outcome without a separate objectives menu. Records group lifetime
admissions, admission income, news, awards and history. Existing maps and native
records remain specialized screens, with return focus restored. Save/load,
pause and Build flows are preserved, not rewritten.

## Representative interaction counts

Counts start at the named root with default focus; they exclude movement within
a long report or target list. Shoulder presses count separately, not as A.

| Task | Before | After |
|---|---|---|
| Finance root → ledger | Down ×3, A (4 inputs) | R1 (1) |
| Finance root → history | Down ×4, A (5) | R1 ×2 (2), retaining workspace |
| Research root → change funding draft | A editor, A funding (2) | R1, X (2), with all priorities visible and unsaved state |
| Browse related research categories | Vertical eight-row editor, scrolling | Two-column seven-category group; all categories and review/discard fit |
| Marketing root → campaign type list | A New campaign (1) | R1 (1); no separate nested types page |
| Change campaign target in editor | A target browser, move, A confirm | Left/Right inline; full browser optional |
| Park root → admission editor | Right, A Finance detour, A editor (3) | Right, A (2), direct return to Park |
| Related report → another tab | B then select another report | L1/R1, retaining each tab's focus |

The gain is fewer report/target transitions and better visible information, not a
claim that every task takes fewer presses. Financial/research/campaign review
remains explicit.

## Verification and evidence

`ManagementUxTests` links the production UI and real native engine, dispatches
controller frames (including duplicate actions), renders at 512×384, executes
native action queues and saves/reloads. It checks signed reports, month/sample
selection, tab focus, loan/cancel/duplicate handling, no-money controls, direct
park admission and native map/history return, stage progress, research drafts,
review/cancel/funding/accounting/catalog, campaign target browsing/duration,
exact native campaign charge, restrictions and native persisted values.

The fixture deliberately seeds known native ledger/history/research/ride records
for deterministic assertions. Screenshots are genuine **desktop offscreen native
runtime fixture** output with original installed G1 art and official objects;
they are not mature-park financial histories, mocks, or physical Brick validation.
See `captures/controller-ux-phase6/README.md` and
`evidence/controller-ux-phase6/verification.json` for capture hashes and results.

Affected Linux x86-64 GUI and CLI targets build against the clean native engine
built during Phase 7. GCC 13.3/SDL 2.30/ICU 74.2, scripting enabled; HTTP, network,
OpenGL, TTF and optional codecs disabled. The existing build-local
`ScenarioPatcher.cpp` picosha2 warning demotion is retained; engine source is
unchanged. Banners retain the configuration baseline, not the eventual publication
commit. No new AArch64 package or physical scaling/input claim is made.

Preservation includes the full Phase 7 `PathBatchTests` (Line/Fill/native actions,
rejection, explicit partial failure, cost and save/reload), twelve gameplay/UI
drivers and five standalone input/launcher/save/file tests. Recorded command logs
and hashes distinguish build, automated runtime and visual inspection evidence.

## Publication, limitations and next milestone

The user explicitly authorized publication of Phase 6 and completed Phase 7
runtime screenshots. Their capture directories now include those PNGs; original
G1, objects, asset archives and private save files are not published. Phase 7
captures retain their original milestone provenance/hashes.

Physical Brick/NextUI testing, crowded real parks, long translated labels and
hardware graph/input performance remain unverified. The earlier optional bright-
sand capture crash remains untriaged, not declared fixed by these grass fixtures.
The shared palette is preserved; this milestone does not claim to resolve every
contrast or management-polish observation.

Next: full-system controller journeys, actual-device readability/performance,
real populated parks, translated clipping and consistent Back/focus semantics.
Use `UI_POLISH_BACKLOG.md` for evidence-based defects/verification tasks and
`UX_FEATURE_BACKLOG.md` for optional proposals.
