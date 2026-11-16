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

Persistent native main-viewport follow/stop, controller recovery, native GuestList
filters and whole-filter map entry/recovery are delivered below. Existing groups/tracked filtering,
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

Phase 3 follows below. Canonical Phase 2 was verified by guarded fast-forward
publication; Esperanto remains excluded with its unchanged recorded checksum.


## Phase 3: native filters and whole-roster map entry

Phase 2 canonical `b242e5c7d9aa2624486ae1aae980f3ccd703dc64`, local `00e24fc`,
matching tree `444e2f25d7735d6b43706f8371bb6c47b5d7640b`.

Ride Customers adds native rider, queue and thinking-about-ride entry points.
Membership uses the same 12-byte Formatter arguments as desktop GuestList,
including describeAsInside, native ride names, the first fresh thought (freshness
<=5) and its StringId wildcard. Copied ride identities are revalidated before
reading arguments; removed/reused rides produce an empty unavailable filter.
No rider count, queue membership or thought association is inferred from totals.

Every guest roster now supports controller name search, tracked intersection and
Show roster on map, retaining action/thought groups and copied native identities.
Name search uses native formatted names and String::contains with desktop's
case-insensitive mode. The existing bounded controller keyboard applies on OK;
Back discards the draft and Clr/OK removes the name constraint. Filters are
transient view state, with no gameplay or save-format additions.

The map keeps native tile colours, camera geometry and marker positions/colours.
The shared native marker helper now carries entity IDs so the Pocket overlay can
select exactly the actual filtered members. A sorted ID lookup bounds dense-map
marker lookup cost. Map subject rosters use the same membership as markers; map
tracking can be toggled independently and mode changes explicitly clear custom
filters. B returns through subject inspection/map to the original roster/filter
and its ride or group parent. The copied ride parent survives nested guest/ride
inspection. Live changes retain selected guest identities and footer controls.
Start/menu replacement and native load teardown discard transient context.

Final GUI/CLI affected-object rebuild PASS, with production source held fixed.
Expanded asset-backed native/SDL GuestInformationTests PASS for rider/queue/queue-front
membership, another ride exclusion, fresh/stale/other-ride thoughts, group/tracking/
case-insensitive name intersections, marker identity, controller search apply/cancel/
clear, tracked selection, filtered map subject inspection and Back to original roster
and ride. Existing native history, follow/stop/zoom/rotation/pickup/load/removal/reuse/
shutdown and persistence assertions remain enabled and pass.
Asset-backed ParkMapInformationTests, GuestCompletionTests, SDL
PresentationLifecycleTests and full asset-backed PathActionsTests PASS. CTest
input/launcher/save-preservation 3/3 PASS. Expected native invalid-action diagnostics,
public-fixture missing scenery warnings and dummy hardware-renderer warnings remain
reported; no physical framebuffer validation is claimed.

A fixture assumption about the default 'Guest' name failed under native real-name
mode. The fixture now uses the native naming action to assign a deterministic name;
case-insensitive and nonmatching-name assertions were retained. An inconsistent
object build caused a stale-menu filter-layout mismatch; every affected object was
rebuilt from frozen source before the final runs. Two obsolete assertions were
updated: the generic 'Filtered guests' title now checks the actual native group
label and all footer controls; stale Locate now checks exactly 19 actual native
members, departed-identity exclusion and the four map/name/tracking/Back rows.
Existing identity-focus, inspection, native action/accounting and parent-return
coverage remains enabled, with no production eligibility guard weakened.

Ordinary roadmap: the dedicated global guestsThinkingX thought-type entry point
(which is not called elsewhere in the current desktop UI) remains unsupported;
action/thought argument groups and ride-specific thoughts are delivered. Moving
train map overlay and editor-only map operations remain separate map gaps.
Hardware/AArch64 and sustained large-park performance remain unvalidated.


Validated Phase 3 canonical publication: `8d318517ae3f05d4a5496746e25d252bb4795844`,
local `e3593c261cb80ecc4f78ea928cb5c89f67567628`, exact shared source tree
`1400355d771ac8fdc3173e2ea34b9abb3caebe66`. Publication verified native remote
parent `b242e5c7d9aa2624486ae1aae980f3ccd703dc64`, every changed blob SHA and
complete source-tree equality, then used expected_sha and force=false fast-forward.
This documentation receipt follows that validated implementation without changing
production code or tests. Esperanto retains SHA256
`e37ba559b4d9d761e74f2bb284d1ec762b813e789655c0495a882c83651890b5`
and remains the sole unrelated working-tree edit, excluded from milestone commits.
