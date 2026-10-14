# Native shop appearance and preview lifecycle — incremental checkpoint

## Baseline integrity

Canonical baseline `615906d9ab009679c2ad9bff0d4e205caea696ee`, local safety
`ee99b273fcdfdec785b1f49b03a8f95959b29fc4`, complete identical tree
`ca76d19079118a5f41ad781a8596f8ca3091660b` verified against the current branch.
No recovery required. Only the existing protected Esperanto edit was present;
its SHA256 remains e37ba559b4d9d761e74f2bb284d1ec762b813e789655c0495a882c83651890b5.
No clone/reset/clean/restore/history rewrite; that file is excluded from commits.

## Phase 1 — paused shop preview discrepancy resolved

The original PathActionsTests driver reproduced PASS with OPENRCT2_HANDHELD unset
and FAIL with it forced to 1. Diagnostic native state at the failing paused frame:
SDL focus false before/after, zero ghost, zero real track at the preview tile,
one real ride instead of real+borrowed, unchanged cash 997000, selected shop tool
still active. InputManager::process calls UpdateExplorer only in Pocket mode;
UpdateExplorer intentionally calls Shops::ClearPreview on lost focus. This clears
the requested preview during the native ShopPreviewScope suspension, so its
restoration correctly does not resurrect a cleared request. No production leak,
cash mutation or stranded borrowed ride was demonstrated by that failure.

The fixture now explicitly distinguishes actual Pocket focus-loss frames from
focused/no-shell native tick checks. Both must preserve real track/cash. Focus
loss must remove ghost/borrowed ride, retain tool selection and never resurrect;
focused/no-shell tick must retain the valid ghost/borrowed ride. No assertion was
simply skipped or removed, and production focus cleanup is unchanged. Existing
nested provisional/save/autosave/tool/menu/load/shutdown assertions remain enabled.

Phase 1 PASS: entire asset-backed native/SDL PathActionsTests under both documented
and forced Pocket environments, Release GUI/CLI, CTest 5/5. The SDL dummy window
is actually unfocused. A separate real focused-display run was attempted using a
locally extracted Xvfb, but this workspace cannot establish its Unix or TCP display
sockets. No focused-display/physical Brick result is claimed. Native no-shell
paused preservation and explicit focus-loss branches are both exercised; focused
full-shell display validation remains a host/hardware follow-up.

Shop appearance implementation and final preservation/capture closeout follow.

## Phase 2 — native shop/facility appearance

Desktop audit: Ride.cpp excludes station flags from shop building colour support;
RideTypeDescriptor hasTrackColourMain/Additional/Supports and the loaded entry's
disableColourTab govern building controls. Ride::hasRecolourableShopItems governs
the random merchandise checkbox. Pocket uses exactly these predicates, the
existing native colour dropdown ordering/names and RideSetAppearanceAction.
No shop vehicle/station/music controls are invented. Inspection always offers
Appearance; objects without native controls show an explicit empty state.

Controller: shop inspection fifth row Appearance; D-pad selects native channel
and paged colour/value, A opens Cancel-first review, A on Cancel/B returns to the
unchanged editor, Apply runs the native action then returns to the same channel.
The review shows original/proposed native names and the queried native cost.
Shared naming, prices/common prices, status, statistics and demolition remain.

The optional native action guard serializes the exact reviewed shop context and
checks both Query and Execute. It includes native allocation generation (process
local, never park-save data), ID/type/subtype/build date, location/real block,
object descriptor/version/flags/items, supported channels, palette cheat state,
all building colour schemes and merchandise random-colour baseline. Invalid or
unchanged proposed values are rejected. Unguarded desktop actions retain their
existing behavior. Native action execution owns appearance and accounting; no
UI writes live ride fields. Pending UI callbacks use window lifetime/request
checks; busy input and shared edge-triggered controller handling prevent repeats.

Focused ShopAppearanceTests covers five real native objects: Burger Bar, Balloon
Stall, Toilets, Cash Machine and Information Kiosk. Native capabilities differ:
Burger/Toilets/ATM expose zero controls for these loaded objects; Balloon and
Information expose building main plus random merchandise colours. It exercises
all supported channels, unsupported station/invalid colour rejection, exact zero
native cost and unchanged map/cash/prices/status/name/common-price state,
serialized guards, changed baseline/object flags, demolition/reuse of the same
native slot, Cancel-first cancellation, repeated A, actual SDL virtual held A,
stale UI recovery and native .park round-trip. The fixture temporarily changes
object flags solely to demonstrate stale loaded-definition rejection and restores
them; production never edits object definitions.

Phase 2 validation: Release GUI/CLI builds, ShopAppearanceTests native/SDL with
original assets, and CTest 5/5 pass. Final preservation/capture closeout follows.
