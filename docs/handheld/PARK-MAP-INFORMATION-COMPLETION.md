# Native Park Map and Information Completion

Goal: 100% ordinary OpenRCT2 gameplay through controller-native Pocket. This checkpoint is incremental; missing ordinary functionality remains on the roadmap.

## Phase 1 — native park map

Overview adds Park map after existing status/admission rows. The full-screen Pocket page shares desktop Map.cpp native tile colours, ownership dimming, ride-category colours, rotation transform and viewport bounds. A transient incremental raster refreshes at most 2048 tiles per update and rebuilds on map dimensions, mode or camera rotation changes. No map pointers or Pocket gameplay state are serialized.

D-pad moves the selected native tile, A explicitly locates and returns to park, X rotates the native camera, Y switches terrain/ownership versus ride/path colours, L/R changes zoom (whole park to 8x), B returns to Overview. Selection clamps to the playable interior of rectangular maps. A white box indicates the main viewport; gold cross indicates selection. Entry ends prior construction tools/ghosts. Page state dies with its menu on load/exit.

Release GUI/CLI rebuild PASS. ParkMapInformationTests PASS with native initialized 64x48 map, every interior tile compared to desktop native colour helpers, map bounds, four camera rotations, cache invalidation, native MapChangeSizeAction to 80x56, SDL menu/action routing, explicit locate/back and native save/reload. Esperanto checksum remains e37ba559b4d9d761e74f2bb284d1ec762b813e789655c0495a882c83651890b5.

Publication starts from canonical 7b99544ee1aaf037046fb8659cf5129e7ed1ccf3 / tree 2e7d6d92d9ba8e4e5378f26f2694442cca13ae15. This workspace had reverted to staff source; exact GitHub blob recovery produced local safety cd94db316c8ff93cca61b697a6a0de7c5cad69ea with that same tree without modifying Esperanto. Subsequent validated phase commits are recorded in the next checkpoint update to avoid self-referential commit hashes.

At Phase 1 the planned news/history and peep/roster follow-ons were pending; Phases 2–3 below deliver them. No AArch64, physical Brick performance/input, sustained routing or full SDL gameplay-frame validation is claimed. Captures use production rendering and private supplied assets; archives, scenario parks and generated private designs are excluded from publication.

## Phase 2 — native news, awards and histories

Overview adds native recent/archive news, active awards, and rating/guests-in-park history. News uses copied native item text/date/type/flags/association baselines, native date names, six-line UTF-8-aware paging, refresh and native subject locations. Recent-to-archive movement invalidates the old review until refreshed. Ride/peep subjects carry existing generation-aware identities; disabled/expired/departed targets and out-of-map spatial news cannot locate. No desktop subject window is opened. News and awards are read-only; no acknowledgement or notification-setting control is claimed.

Awards use actual currentAwards, AwardGetText and AwardIsPositive; empty awards remain empty. Graphs call native DrawRatingGraph/DrawGuestGraph with actual history arrays, native ordering/date labels and undefined sentinels. No samples or news are invented to populate captures.

Focused native/SDL regression PASS: native message add/archive/remove APIs, exact membership/order, stale text/archive/expiry rejection, native spatial locate, empty news/awards, native award grant (test fixture only), actual Park::UpdateHistories output with remaining undefined samples, menu paging/recovery and native save/reload. Headless graphics-disabled tests emit native G1 diagnostics; asset-backed capture validation is separately reported.

| Published phase | Canonical SHA | Local safety SHA | Matching tree |
| --- | --- | --- | --- |
| Native map | 7a6f491bead20fc5af287fcfc4f46367f6fe982e | 20376377a8179fb607596c005e381f818ade3636 | ca1548cc53df0771c4f71814d7310ab21127708a |

Phase 2 publication identifiers are recorded in the following documentation checkpoint. At Phase 2 the subject/inspection follow-on was pending; Phase 3 below delivers it. Broader ordinary notification/history settings remain on the roadmap.

## Phase 3 — native subjects, inspection and recovery

Y cycles terrain/ownership, rides/paths, guests and staff. Native map transforms and native CoordsXY::rotate(-cameraRotation) keep D-pad on camera-relative isometric tile axes, with all interior tiles reachable; cardinal steps follow the native tile diamond rather than a virtual pointer. Select opens actual subjects: rides, all/tracked guests, or all/handyman/mechanic/security/entertainer staff. Native map marker colours/coordinates are shared with Map.cpp entity helpers; no entity flags/positions are written to draw overlays. Native ownership labels distinguish owned, owned rights, land/rights for sale and unavailable land.

Subject rows recheck live membership while preserving selected native identity where possible. Show on map retains zoom/mode/filter; Inspect reuses existing guest/staff/ride panels (news shops reuse shop management). Back restores the subject/news detail and map context. Explicit locate returns to park. Target identity generations and native location eligibility reject departed/reused staff/guests/rides; no raw entity/map pointers survive reviews. Existing map/news transient state is discarded on load/exit; native camera/save state remains native.

News detail now links eligible ride/guest/staff inspection with guarded recovery; non-inspectable financial/award/research/campaign subjects remain explicitly unavailable. Copied native news text is displayed through the native format-token parser, preserving text/UTF-8 and paragraph breaks while neutralising native colours/newlines in one-line roster rows. Baselines keep original raw data unchanged. Awards similarly use neutral Pocket text; graphs keep native dates and white labels on dark panels.

Expanded ParkMapInformationTests PASS: native camera-relative movement at all four rotations; native guest/peep overlay coordinates, tracking query/execute and live membership, actual staff hire/fire and role membership, missing/reused generation rejection, native ride creation and inspection, map show/inspect/back recovery, SDL virtual held-A handoff with no auto-locate, native news subject links/disable/expiry, native maximum 1001x1001 resize (2048 tile refresh budget), shrink/clamp, load cleanup and native persistence. Initial scenario metadata/guests/research are fixture setup; gameplay changes use native actions/APIs.

| Published phase | Canonical SHA | Local safety SHA | Matching tree |
| --- | --- | --- | --- |
| News/awards/history | 5de5e7e288673eb7a233ea2a21bf64b8d83cc21e | a619d62e41dfeafc5278ba7eba0a150025ec3a79 (after b313ef1f10ab1fd39155db2d2ecdb7a08b5e3f2e) | e75c311c4666b3cbbe0ff11b17eed2db422f7bfa |

Phase 3 identifiers are recorded in the final validation checkpoint; final SHA is recorded externally to avoid self-reference. Publication always verifies remote/native parent, exact source-tree equality and expected_sha/force=false fast-forward update. Esperanto is excluded from commits.

Remaining ordinary gaps: moving train overlay, richer ride-category/peep task filters, persistent follow, native scenario/editor map resizing/entry/spawn tools, notification settings/acknowledgement, research/finance/campaign news subject handoffs and finance histories/research finished-item catalog. No native historical award log exists in currentAwards; none is invented. Source parity remains PARTIAL; the 100% goal and all prior construction/management gaps remain explicit.

## Final validation and captures

Release GUI and CLI build PASS. Required native suites PASS: ParkMapInformationTests; SceneryPathParityTests (also private sprites); ConstructionTests; PathActionsTests (including existing management/RideAccess/design coverage); PresentationLifecycleTests; RideConstructionTests; SeatRotationTests; FamilyOperationTests; MazeConstructionTests; MiniGolfConstructionTests; StaffCompletionTests; GuestCompletionTests. Final PresentationLifecycle rerun PASS. Standalone CTest: handheld-input, handheld-launcher and handheld-save-preservation, 3/3 PASS.

Asset-backed DesignCapture PASS (tested/rated authoring, export round-trip, scenery, native design placement/recovery); RideConstructionCapture PASS; Factory Capers MazeConstructionCapture PASS. Original African Diamond Mine CustomCoasterCapture PASS: action-built Wooden circuit/elevated access/Open/save, with an observed natural queue participant over the test window. This is not sustained throughput or evidence for all other ride families.

Final ParkMapInformationTests asset run PASS. Native fixtures initialise object/research/guest data only before gameplay actions. The 1001x1001 map refresh test performs native resize/shrink actions. Captures load supplied African Diamond Mine as a native scenario, explicitly open through the existing native park action when permitted, then advance 16384 native simulation ticks. News, awards, guests and histories are whatever the simulation actually produces; no values, awards or messages are inserted for scenario captures. Native simulation randomness means active awards/counts vary between runs; the final archive records its actual state.

Sixteen 512x384 PNGs: 01 terrain/ownership map and viewport; 02 native ride/path colours; 03 zoomed navigation; 04 actual native news list; 05 full news text/details; 06 actual awards or native empty state; 07 native rating history; 08 native guest history; 09 dense guest mode/legend/footer; 10 actual guest roster; 11 subject detail; 12 existing guest inspection; 13 staff mode; 14 departed staff feedback after native fire; 15 selected native guest on map; 16 native staff roster/empty filter. All captures are production Pocket window + native viewport/component renders. Fifteen use the supplied scenario and private sprites; 14 uses a native initialised action-built fixture and private sprites. They are not full SDL hardware/game-loop framebuffer captures or physical Brick frames.

Every image was visually inspected. Fixed native news roster newline/colour overflow, award focus contrast, map filter/legend overlap and native graph labels/date-label contrast; rebuilt, reran and regenerated affected captures. The capture driver activates the production X8 software context before native camera updates, avoiding invalid drawing-context access under SDL dummy. Normal headless tests still emit native G1 diagnostics without private graphics; dummy SDL hardware renderer warnings are accurately retained. The ZIP includes dimensions/SHA256/provenance manifest and excludes private archives, parks and exported designs.

No AArch64 build, physical Brick controls/performance, sustained park/guest throughput or full SDL hardware gameplay validation is claimed. Map overlays and information are read-only except explicit camera/locate and existing inspection operations; no Pocket gameplay state is saved.

| Published phase | Canonical SHA | Local safety SHA | Matching tree |
| --- | --- | --- | --- |
| Map subjects/recovery and presentation fixes | a5c1a738a06b8b0d876d48c23d4d22707e7139f7 | d1f84ebb494d62b6736cf01f29f86e7636eab666 | 86dbc63652f00ca3af60a5f5d71dc81ae5320699 |

The final documentation/publication SHA is recorded in the delivered report/manifest, not inside its own source commit. Esperanto remains uncommitted and byte-identical to the preserved checksum above.

Recommended next substantial subsystem: native Finance and Research Information Completion. Deliver historical expenditure/category/period navigation and native cash/profit/park-value graphs, native researched/invented item catalog with object availability, and coherent information links from existing news where supported. Preserve existing loan/admission/funding/priorities/marketing controls and all map/construction gaps.
