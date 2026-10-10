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
