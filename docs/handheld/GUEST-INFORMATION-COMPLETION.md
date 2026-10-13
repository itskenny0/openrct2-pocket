# Guest information completion — incremental checkpoint

Baseline: canonical `3cb124cf9e7014c1685a61d819db8598ce7dbf08`, tree
`43e0904ce91e31b1969d0d6c1023c7429b480b81` (finance/research closeout).
The workspace again reverted to the Staff snapshot. Every existing edit was
preserved before exact published blob recovery; recovery safety `a26048b` has
that exact baseline tree. Esperanto retains its specified SHA256 and is excluded.

## Phase 1: native ride history

Guest inspector adds Ride history after the existing actions, retaining their
indices. The adapter uses desktop Guest::onUpdateRides membership: native
RideManager ordering, isRide and Guest::hasRidden. This is a native retained set,
not chronological visits; no dates, visit counts or missing ride names are invented.
Copied native ride identities are revalidated for inspection; live roster changes
retain a selected identity where possible. Missing/departed/reused rides and guests
cannot revive old handoffs. Empty histories remain explicit. Existing ride
inspection opens with Back recovery to the guest history; explicit menu replacement
discards this handoff. Existing map/news guest-return context is retained.
No native gameplay state or save format is changed.

Implementation safety commits: `8a4bae6` and `f97b284`. The earlier interrupted
fixture used unconstructed native rides; native save/load correctly removes those.
The revised fixture places native station track before testing persistence.
Release GUI/CLI rebuild PASS. Asset-backed native/SDL GuestInformationTests PASS:
empty/current native history, exact membership and RideManager order, controller
ride inspection/Back recovery, native station-built rides retained by .park save/load,
unchanged read-only accounting, native demolition clearing history and stale/reused
ride/guest guards. The fixture uses native history APIs for initial membership and
native construction/demolition actions; no production simulation or save change.
Existing GuestCompletionTests and SDL PresentationLifecycleTests PASS.
Full asset-backed PathActionsTests PASS, preserving existing construction, ride/design,
management, groups/tracking and guest/staff placement workflows. CTest 3/3 PASS.
No physical Brick, AArch64, sustained gameplay or hardware framebuffer validation
is claimed. Native headless G1/dummy-renderer warnings are retained accurately.

Unvalidated source was preserved on safety/guest-history-2026-10-13 at
`cfdf2546ba141b4409c53e88eb1c58e05e557ffd`, exact tree
`c609775496c1b893b03d48538f84584ae00dcf1c`. The safety branch remains labeled WIP;
validated canonical publication is recorded externally after guarded fast-forward
publication with exact local/canonical source-tree equality.

## Remaining phases

Persistent native main-viewport follow/stop, identity/load/exit cleanup and
controller menu recovery are pending. Native ride-specific GuestList filters and
whole-filter map entry/recovery remain pending. Existing groups/tracked filtering,
name/tracking/preferences and shared guest/staff pickup/placement are preserved.
All broader ordinary gameplay gaps remain on FEATURE-DEPTH-MATRIX.md.
