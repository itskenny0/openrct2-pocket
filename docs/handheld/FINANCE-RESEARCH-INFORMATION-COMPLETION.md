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
