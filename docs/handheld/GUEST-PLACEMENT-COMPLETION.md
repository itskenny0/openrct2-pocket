# Guest Management and Peep Placement Completion

Goal: 100% ordinary OpenRCT2 gameplay through controller-native Pocket.
Continues published `46141185eab07db7ddfa907f5ef15f1e90fc7719`.
Local safety history is preserved and each canonical publication tree verified.
The unrelated Esperanto modification is untouched. Uploaded private assets are
extracted outside git for native gameplay validation and captures.

## Phase 1 — native groups and filtered rosters

Guests now offers Action groups, Thought groups and Tracked guests alongside the
existing full roster. Native GuestList's twelve-byte Formatter argument keys are
reused: formatPeepActionTo with group formatting, or the first native thought when
its freshness is at most five. Empty native groups are excluded as desktop does.
Counts include only guests inside the park and groups sort by descending count.
Pocket has no artificial group/roster cap. Labels are formatted immediately from
native arguments; retained filter keys are compared, never dereferenced as strings.

A group opens its current membership in the shared paginated roster. Live roster
updates retain selected native identities where possible; returning from inspection
preserves roster focus and returning to groups preserves the group row. D-pad
selects, A enters, B returns; existing Dashboard controls and held-input guards apply.
Filtering/grouping are read-only and introduce no serialized gameplay state.

Phase 2 naming/tracking and Phase 3 shared pickup/placement remain pending.
All prior construction and staff gaps remain on the 100% roadmap.

Phase 1 validation: Release GUI/CLI build and full native/SDL PathActionsTests
with the uploaded installation pass. New checks cover native group count/membership,
first-thought groups, empty groups, held-A handoff, filtered roster focus and parent
recovery, while existing guest read-only and all construction/ride regressions pass.
The live scenario capture runner loads the uploaded assets and simulates native
visitors successfully; final captures will extend this runner for new operations.
