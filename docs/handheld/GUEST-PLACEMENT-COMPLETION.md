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

Phase 2 and Phase 3 delivery are recorded below.
All prior construction and staff gaps remain on the 100% roadmap.

Phase 1 validation: Release GUI/CLI build and full native/SDL PathActionsTests
with the uploaded installation pass. New checks cover native group count/membership,
first-thought groups, empty groups, held-A handoff, filtered roster focus and parent
recovery, while existing guest read-only and all construction/ride regressions pass.
The live scenario capture runner loads the uploaded assets and simulates native
visitors successfully; final captures will extend this runner for new operations.

## Phase 2 — naming, tracking and preferences

Guest Name reuses the bounded eight-column UTF-8-aware keyboard and Cancel-first
review. GuestSetNameAction retains native naming/Easter-egg behavior. Identity,
name baseline, codepoint limit, repeated/default reset are validated. Tracking uses
GuestSetFlagsAction, toggling only tracking from a revalidated full flag baseline;
other native flags are preserved. Tracking has an explicit Cancel-first review and
feeds the tracked roster. Draft/review pages cannot switch into Dashboard rail focus.

Preferences adds native preferred intensity, nausea tolerance, favourite ride and
time in park using the same values and localized formatters as desktop Guest.
Existing needs, spending, thoughts/items and full-text inspection remain intact.
Ride history list and persistent viewport following are still ordinary guest gaps.
Phase 3 delivery is recorded below.

Phase 1 published: `c2ac887c8af13496c523c35d5d74eb7fbf4a350a`.
Phase 2 validation: Release GUI build, focused native GuestCompletionTests and
full asset-backed native/SDL PathActionsTests pass. Native checks cover Unicode,
invalid length, stale/default names, tracking queries/other flags, filtered counts,
preferences, native accounting and save/reload. SDL checks exercise naming keyboard,
held-A prevention, modal rail exclusion, Cancel-first review/draft recovery, native
name/tracking apply and preserved roster focus.


Phase 2 published: `6f75764192280015e84865e9b43338d0e2af3ece`.

## Phase 3 — shared native peep placement

Guest and all four staff inspectors offer Pick up. PeepPickupAction alone owns
pickup, placement and cancellation. A compact park overlay and native hanging
sprite preserve the visible park. D-pad pans, A places, B cancels and returns to
inspection, Y or Select opens options, X rotates and L1/R1 zoom. Options can resume,
open a Cancel-first cancellation review, or return to the person's inspector.
Closing options retains pickup context. Native path/base-Z or terrain geometry,
land permission and clearance are queried at the map focus; errors and prices use
native result feedback. No map elements, positions or pickup ownership are patched.

Targets retain native identity plus frontend generation. Departing peeps cancel
before native removal invalidates their identity; stale queued requests cannot
revive the tool. Duplicate/pending place requests are rejected. Successful placement
clears native pickup ownership through cancel without moving the already placed
person. Load/exit construction cleanup and a save/autosave preparation intent cancel
pickup before serialization. The native action queue exposes its processing state
and rejects reentrant drains; removal within an action uses native nested cancel.
These are ephemeral UI controls, not new serialized gameplay state.

Staff hiring offers Automatic or Manual after the existing role rows. Manual uses
StaffHireNewAction with automatic placement disabled and immediately opens shared
placement. All role/object/costume availability and wages stay native. Cancellation
restores native old-X semantics: an already placed person returns to the native
falling position; a newly hired person whose old X is null remains unplaced and
on payroll. Pick up from their inspector resumes placement, or native dismissal
can remove them. Cancellation does not silently fire or move a new employee.

## Remaining ordinary-play gaps and validation limits

Ride history browsing, persistent viewport following, ride-specific GuestList
entry-point filters and the desktop map display of a whole filtered guest group
remain ordinary guest parity gaps. Grouping covers native action/first-thought
keys and tracked guests, not every desktop filter entry point. Returning from the
park placement tool opens a fresh inspector; prior Dashboard group filter context
is not retained across closing the menu. The native staff pickup/manual-hire gap is
closed; staff patrol execution/animation and every costume still need physical play.
The 100% goal remains explicit; these omissions stay on the roadmap.

Validation uses both legal metadata fixtures and the user-supplied RCT installation.
Public-metadata GuestCompletionTests exercises native Unicode names, tracking,
query/execute, invalid/stale input, guest and all four staff placement roles,
unowned/out-of-bounds placement, pending guards, native restoration, manual hiring
without automatic sources, unchanged role orders, accounting and save/reload.
The full asset-backed native/SDL suite exercises actual controller naming/tracking,
held-A handoff, modal focus, Cancel-first placement recovery, autosave/removal cleanup,
identity reuse and existing construction/RideAccess/Ride Management regressions.
AArch64 execution, physical Brick controls, live patrol effectiveness and sustained
routing/animation are not claimed. Capture evidence and final publication identifiers
are appended after validation/publication.

Release GUI/CLI and the full native/SDL PathActionsTests pass. StaffCompletionTests,
ConstructionTests, SDL PresentationLifecycleTests and all three CTest checks
(input, launcher, save preservation) also pass. The SDL suite additionally fires a
picked-up employee through StaffFireAction, proving removal cleanup does not
reenter the native action queue. Invalid native queries log expected diagnostics.
