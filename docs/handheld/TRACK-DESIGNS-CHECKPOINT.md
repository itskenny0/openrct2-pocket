# Saved wooden track designs checkpoint

Continues `handheld/modern-ui-paths` from
`9f2c4c8e8b8e63610dc384e1069cd2027b92dcac`.

## Playable workflow

Build → **Wooden designs** → existing paged visual picker with native track
previews → position/rotate native ghost → A cost review → Cancel-first purchase
confirmation → one native placement → finish missing entrances/exits or safely
leave and recover through Dashboard / Rides → optional existing Access / Paths
handoff → Dashboard testing/open review → save/reload.

| Mode | Controls |
| --- | --- |
| Design picker | D-pad grid/pages; A select; B cancel; Start existing pause |
| Placement | D-pad target; A review; Select turn; Y designs; B leave |
| Review | Cancel selected first; D-pad choose; A confirm/cancel; B cancel; Start pause and discard review |
| Station recovery | Existing D-pad target/A place/B leave; native adjacency determines orientation |
| Camera | Existing X rotation and L/R zoom |

Selection survives reopening the picker. Selecting/reviewing/confirming consumes
one semantic action and suspends held input across transitions. A completed
purchase offers optional access; Cancel returns to the park without placing paths.
No ride is silently demolished or refunded when unfinished construction is left.

## Supported scope and availability

Only genuinely indexed designs of native `RIDE_TYPE_WOODEN_ROLLER_COASTER`, with
an exact currently loaded vehicle object supporting that type, researched type
and vehicle, a loaded station object, ordinary station(s), nonempty track and
no maze/scenery elements are listed. Native ignore-research cheats retain their
meaning. One through the native maximum number of stations are supported.
Unsupported categories, maze/scenery-dependent designs and missing/unresearched
vehicles are excluded. No object-loading or research-unlocking fallback exists.
The repository is rescanned when the picker opens. An empty eligible set remains
honestly empty. Designs are not bundled with this checkpoint.

Native terrain fitting and the desktop's bounded seven-height search establish
placement height. There are no custom height controls. Native queries decide
terrain/support/ownership/clearance/funds/pause/validity; a listed design may still
be unplaceable at a particular target. Imported native ride settings remain native.

## Reused architecture and native actions

- `ObjectPicker` adds stable file identities and an optional native preview
  callback; existing Tree/Shop/Flat Ride object previews remain the default.
- `Designs` adapts native repository enumeration/import, `TrackDesignDrawPreview`,
  `TrackDesignGetZPlacement`, `TrackDesignPlace` and
  `TrackDesignPreviewRemoveGhosts` to existing Explorer targeting and panel styles.
  Lazy thumbnails retain native preview pixels only. Rendering runs in playing
  mode, avoiding the track-manager renderer's automatic object-loading path.
- Registered/serialized `SavedTrackDesignPlaceAction` delegates real construction
  and pricing to `TrackDesignAction`. It rechecks the exact loaded/researched
  vehicle, reviewed cost and money-rule snapshot before queued execution. This
  prevents the desktop action's vehicle fallback from bypassing Pocket selection
  restrictions after research changes. Native desktop behavior is unchanged.
- Existing borrowed preview-ride ownership restores its exact unused record.
  Queries reuse the owned preview record while a ghost exists, including when
  every other native ride slot is occupied. That transient query-only record is
  not serialized and cannot execute a real placement.
- Preview refresh replaces ghosts only for changed valid target/orientation/design;
  native validation still runs for changed park conditions. Invalid placement,
  unavailable objects, menus/picker/review, pause, focus loss, tool exit, park
  load/change and shutdown clear ghosts. Existing nested provisional simulation
  and save/autosave guards remove/restore them without saving provisional tracks.
- Existing `FlatRides` recovery / `FlatRideEntrancePlaceAction` now accept ordinary
  wooden coaster stations and an explicit native station index. Native adjacency,
  duplicate rejection, stale ride/station guards and native entrance/exit actions
  remain authoritative. Original flat placement eligibility is unchanged.
- Existing `RideAccess` checks native queue association per station and selects
  access points still needing attention. Elevated access focuses the terrain tile
  and initializes the existing Paths fixed height to the native station landing.
  All existing path options, ghost, controls, costs and native connections remain.
- Dashboard native roster, status review and save/load need no parallel systems.

Solid palette styles remain unchanged: panel 118, border 119, selected gold 53,
primary text 21, secondary text 18. No dithering or world-render replacement.

## Validation and captures

GUI and CLI builds succeed. CTest passes 3/3 (input, launcher, save preservation).
`run_native_test.py` passes the full existing engine-only suite and the real-asset
UI-linked suite, including all previous tools, pickers, management sections,
flat rides and ride access. `git diff --check` passes.

`render_designs.py` links `test/handheld/DesignCapture.cpp` against the production
GUI build and uses SDL dummy drivers plus private native scenario assets. It
exports the original Mines of Africa **Wooden Roller Coaster 1** through native
`TrackDesign::CreateTrackDesign` / RCT2 T6 exporter into an external profile.
The genuine export has 146 track pieces, two stations and four entrances/exits.
No fabricated track layout or extra object load is used.

The legally queried owned site is tile origin (704,704). A native permitted loan
provides £40,000 cash; queried and once-charged placement cost is **£32,461.10**.
The native ghost contains 305 provisional elements. Tests cover:

- eligibility / research and exact loaded object; genuine native preview rendering
  leaves real elements, cash and scenario random state unchanged;
- full paged picker navigation, selection/cancellation and virtual-controller held
  A handoff; paging-only metadata fixtures do not create fake available designs;
- unchanged ghost/no duplication, invalid targets, orientation, camera controls,
  nested provisional suspension, menu/picker/review transitions and save/reload;
- last-slot ghost query using native empty-ride capacity fixtures, then native
  demolition cleanup with unchanged real track/cash;
- insufficient funds, pause/no-money snapshots, action factory serialization,
  stale research and changed reviewed cost; one purchase and duplicate rejection;
- natural Dashboard enumeration and optional completion cancellation;
- native entrance/exit removal on the purchased copy followed by actual Dashboard
  inspection recovery across both stations, duplicate/stale rejection and exact
  native recovery costs;
- optional Paths focus at elevated landing height, native queue station association,
  normal exit landing, path ghost/no spend and exact native path charges;
- Dashboard Cancel-first testing review/cancellation/explicit confirmation, and
  constructed ride/status/cash/access save/reload with no saved ghost.

Eight captures are exactly 512×384: picker, ghost, cost confirmation, entrance,
exit, completed coaster, Dashboard inspection and Dashboard testing review.
They render genuine native scenario world graphics with production handheld
presentation. Entrance/exit captures use native selection arrows, not building
placement ghosts. The station recovery captures intentionally exercise native
removal/replacement of the newly purchased copy's access points. Paging capacity
fixtures are cleaned before capture/save. Private assets, exported TD6 and park
files stay outside git.

Reproduction (paths supplied by the local installation):

```sh
python3 tools/handheld/render_designs.py --build BUILD --data ENGINE_DATA \
  --assets PRIVATE_RCT --scenario SCENARIO --output EXTERNAL_OUTPUT --ninja NINJA
python3 tools/handheld/run_native_test.py --build BUILD --data ENGINE_DATA \
  --assets PRIVATE_RCT --ninja NINJA
python3 tools/handheld/run_native_test.py --build BUILD --data ENGINE_DATA --ninja NINJA
```

## Limitations and next workflow

This is saved-design construction, not track editing. Only the native wooden
coaster type is supported; scenery-dependent designs and unavailable objects are
excluded. Native minified thumbnails are small within the unchanged six-card
picker. The preview/render cost has not been benchmarked on physical Brick;
validation is native desktop software rendering, not an ARM/Brick test.

The original export contains an unused operation option of zero; the native
import action logs its existing rejected `RideSetSettingAction` setting while
placement and testing succeed. The Pocket adapter does not override native
operating rules to hide that diagnostic.

The scenario test proves native station landing and queue association, not a
complete elevated route to the park network or natural guest operation of this
new coaster. Existing Paths remain manual and no bridge/tunnel automation is
introduced. No guests are forced and no successful operating statistics invented.
The next substantial workflow should complete controller-native elevated access
from these coaster stations to a real park path network, then demonstrate native
opening and genuine guest queue/ride/exit, with safe cancellation/resume.

## File browsing / native inspection follow-up — 2026-10-14

SAVE-LOAD-BROWSING-COMPLETION.md adds X Browse files to the existing native repository
picker. Supported native-format folders/files open metadata/preview inspection before
placement. Same-file height/rotation/scenery handoff, stale byte/descriptor guard,
placement cancel-to-file and repository refresh are delivered. Native parsing and
current eligibility remain authoritative; flat/special design and substitution limits
are unchanged. The broader construction milestones already supersede the early
wooden-only statements above; this browser does not add unsupported ride families.


## Explicit design substitution/warnings follow-up — 2026-10-14

DESIGN-SUBSTITUTION-COMPLETION.md delivers native required/optional availability,
explicit already loaded/researched exact-type/track-group vehicle choices, retained
height/rotation/scenery and source-file identity, Cancel-first native quote/context
guards, actual optional-scenery omission counts and stale/failure/access/save recovery.
Earlier substitution-gap entries are superseded only for this scoped workflow.
Arbitrary scenery/cross-family replacement, flat/special design geometry, native
rollback, exhaustive operation and hardware limits remain. Existing completed
browsing/authoring/selected-track capture packages are preserved.
