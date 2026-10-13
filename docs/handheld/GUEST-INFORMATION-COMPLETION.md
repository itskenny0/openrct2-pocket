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

Persistent native main-viewport follow/stop and controller recovery are delivered below. Native ride-specific GuestList filters and
whole-filter map entry/recovery remain pending. Existing groups/tracked filtering,
name/tracking/preferences and shared guest/staff pickup/placement are preserved.
All broader ordinary gameplay gaps remain on FEATURE-DEPTH-MATRIX.md.

## Phase 2: native persistent follow

Phase 1 canonical `784244aa2db522cae00e758505aa0332ce0fdd40`, local `35b8098`,
matching tree `8f58c4c11dd430bb1de032aba29b2d40bb77a05b`.

Guest inspector adds Follow guest / Stop following after the existing actions.
Follow returns to the unobstructed park; B recovers the same inspector, retaining
follow. D-pad explicitly stops native follow before panning. The compact native
two-line context box shows B Guest / D-pad Stop. Zoom and rotation use existing
native viewport operations. No peep position, flags, metrics, simulation or native
serialized gameplay data is changed. GuestFollow.h retains only a transient copied
guest identity; WindowFollowSprite/WindowUnfollowSprite and native smart-follow
resolution own guest/vehicle/ride camera behavior.

Picked, departed and reused guests are rejected. Native peep removal stops follow
before entity reuse; native map/load intent and window-manager cleanup stop it.
If native manual locate/pan or another camera owner replaces follow, the transient
Pocket guard is discarded. Ordinary menus can be entered/recovered without losing
follow; no new camera simulation or desktop mouse mapping is introduced.

GUI/CLI build PASS. Expanded asset-backed GuestInformationTests PASS for controller
Follow/Stop, B inspector recovery, native zoom/rotation with unchanged peep position,
controller D-pad stopping, native pickup rejection/cancel, load cleanup, actual
native removal, reused entity slot with deliberately matching fixture identity
fields, fresh replacement identity and native window-manager shutdown. Native
fixture setup is explicit and production never generates or edits peep state.
SDL PresentationLifecycleTests and asset-backed ParkMapInformationTests PASS;
CTest input/launcher/save-preservation 3/3 PASS. Existing history persistence and
inspection tests remain enabled and pass. Hardware/AArch64/sustained-follow and
hardware-frame validation remain unclaimed.

Phase 3 native ride-specific GuestList filters and filtered map workflows remain
pending. Canonical Phase 2 SHA is recorded externally after verified fast-forward
publication; Esperanto remains excluded with its unchanged recorded checksum.
