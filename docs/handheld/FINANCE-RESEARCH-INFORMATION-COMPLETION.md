# Native Finance and Research Information checkpoint

Goal: 100% ordinary OpenRCT2 gameplay on controller-native 512×384 Pocket. This
checkpoint extends information workflows; no independent simulation or serialized
Pocket gameplay state is introduced. Missing ordinary functionality remains on the roadmap.

## Preservation and publication

Remote baseline: `803675aebc49718f82825ff88365dea89045fdcb`, tree
`5bdf19ffb42d4491b80d871e65a200f05506c405`. The workspace reverted to older Staff
source twice; exact published blobs were recovered without clone/reset/clean/restore.
Esperanto stays uncommitted with SHA256
`e37ba559b4d9d761e74f2bb284d1ec762b813e789655c0495a882c83651890b5`.
Recovery safety commit: `ed45789` with the exact baseline tree. Canonical publication
uses verified native parent, equal local tree and expected_sha/force=false.

## Phase 1: native expenditure ledger

Dashboard → Finances → Expenditure ledger. Existing loan/admission and Back indices
are unchanged. D-pad up/down selects all fourteen native categories; left/right
changes the native month. Explicit Older/Newer/Back rows provide the same operations.
Selected entry and period net use native currency and signed net category values,
not gross turnover. Zero, undefined and no-money values are explicit.

Uses ParkData::expenditureTable, ExpenditureType, native summary category/month strings
and GetDate. Sixteen retained periods maximum, matching desktop SummaryMaxAvailableMonth.
An absolute native month keeps selection stable when the native table shifts;
expired periods clamp to the oldest retained period. No entry pointers are retained.
Menu/load/exit destroys transient navigation; native accounting/save format is unchanged.
No simulation action is needed to read this ledger; underlying transactions remain native.

Release GUI/CLI build passes. FinanceResearchInformationTests exercises actual
ParkMarketingAction Query/Execute, ParkSetResearchFundingAction and FinancePayResearch,
exact cash/net/category accounting, native DateUpdate/month shift, retained bounds,
zero/undefined/no-money states, actual controller navigation/cancel, virtual SDL held-A
handoff and native ScenarioSave/Load persistence/teardown. Public graphics-disabled and
private-asset SDL runs pass; native headless sprite diagnostics/dummy warnings do not
establish hardware validation.

## Remaining scope at Phase 1

Financial graphs, native invented-item catalog and linked finance/research/campaign
news handoffs are not yet delivered by this phase. Existing map/train/follow/notification,
construction/design/guest/staff and arbitrary file-browsing gaps remain unchanged.
AArch64, physical Brick, hardware-frame capture and sustained routing remain unvalidated.
Exact published phase commits and capture provenance are appended after validation.

## Phase 2: native financial histories

Phase 1 canonical `1ab190383ae7fbc5f6c38bf3320eac418141c090`, local
`cd5da74381f27773408dec46a17181f67e9eb34f`, tree `cad315a575fd42a7951e8dc6051b308a54cd1948`.

Finances → Financial histories shows desktop's 64 weekly samples of native cash
less loan, weekly profit and park value. A Next graph cycles all three; left/right
selects a bounded sample; explicit Older/Newer rows do the same. No interpolation,
forecast or fictitious company-value history is introduced. Native current company
value remains on Overview; native state has no company-value history array.
Graph dates/order/currency come from Graph::DrawFinanceGraph. Native min/max range
matches desktop Finances; negative cash/profit values are preserved. Undefined
records stay blank and selected values display Unavailable; empty/no-money states
are explicit. Graph dark-label styling and breakOnMissing are opt-in, preserving
desktop defaults. Native graph raster regression proves missing records are not
joined; native Park::UpdateHistories/profit updates, sample bounds, controller
entry/cycling/return, save/reload and no-money history pass. Release GUI/CLI passes.
Older native samples beyond desktop's 64-point view are not exposed by this page.
Research catalog and linked news handoffs remain pending at this checkpoint.

## Phase 3: native invention catalog

Phase 2 canonical `c103108ba7d109f7993fdb65a8c702c360f774d6`, local
`66ac4a5c1a14c9fbb1eafb15bf1ed057f3001aef`, tree `b65b6004f62319aa734c46d33c84e8fd3a925840`.

Research → Invention catalog preserves native researchItemsInvented ordering and
actual records, including duplicates and unavailable loaded-object records. All/seven
native category filters show actual membership/counts. Native research vectors can
contain a current project before it becomes invented: details separately query real
RideType/RideEntry/SceneryGroup invention state instead of equating vector membership
with availability. No fictional invention date or unlock operation is supplied.
Native localized item/category names and Object::DrawPreview are used where loaded.
Missing/invalid descriptors remain readable unavailable records, with no unsafe preview.

Copied targets retain raw research identity, flags/category, duplicate occurrence and
full ObjectEntryDescriptor. List changes retain the selected matching identity; leaving
or reusing an object slot invalidates detail/preview. Only short-lived resolved pointers
are used while drawing. Controller left/right filters; up/down scrolls; A inspects;
B restores catalog focus or Research. Load/exit destroys menu state.

Focused tests initialize native scenario research metadata, then use ResearchUpdate
for actual invention progression (no live manual unlock). Actual membership/category/
duplicate/missing records, stale descriptor rejection on native manager unload/reuse,
controller detail/filter/recovery and native availability persistence pass. Source never
loads objects or changes research flags; loading in tests is initial fixture metadata.
Linked finance/research/campaign news remains pending at this phase.

## Phase 4 — linked management news

Money news opens Finances, campaign completion opens Marketing, and native research completion opens the exact available invention detail. B and explicit Back recover the copied news review. Research links copy the loaded object descriptor and reject expired/disabled messages, unloaded/reused objects and changed availability. Campaign news does not encode a finished campaign identity; its link correctly opens the overview instead of inventing campaign details. These links are read-only navigation and do not unlock research or execute transactions.

Focused native/SDL tests pass actual research-generated news, action-funded campaign completion, finance-message API fixtures, return context, disabled messages and expiry. Existing loan, admission, funding and marketing controls remain unchanged.

Phase 3 publication: canonical `6dd46c7ec9f7f6f3cf81e400e25ddc9109ea2cbe`, local `a119ba8819233e5636cc4ce130a7ae6a62f54501`, identical tree `737574e2837ae29b6d5de9476d7b0514ee6ebfae`.
