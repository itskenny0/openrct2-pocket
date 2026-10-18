# RC4 real-play continuation — guest, coaster and native events

Scope: current slate-green UI and visual catalogs preserved. The user explicitly
deferred the Line/Fill defect. This is a non-path RC4 candidate, not acceptance of
the original all-priorities milestone. RC4-PATH-INVESTIGATION.md is retained unchanged.

## Park inspection

The central native GuestOpen entry now opens Pocket's Overview / Needs / Thoughts /
Details workspace when handheld play is enabled. It resets an unrelated prior
roster caller while retaining the exact entity target; desktop inspection remains
native. Dashboard entry retains its own caller and row. StaffOpen and ride track /
vehicle entry points have equivalent routing. Tracking, following, pickup/placement,
live needs/thoughts and roster focus continue to use native state/actions.

## Coaster construction

The missing-preview cause was lifecycle handling: RemovePreviewTemporarily cleared
the native ghost and RestorePreview only reduced a nesting counter. Update also
trusted cached preview metadata without checking native ghost existence. The
preview now records intent/location, separates temporary removal from cancellation,
and recreates the native ghost at the outermost restore. Whole-tick and save
preview suspension also include CustomTracks, excluding its temporary ride record
and ghost from simulation and serialized parks. Query/preview never spends money.

Catalog thumbnails now call the native RideConstruction track renderer with the
actual ride, piece, direction, chain and inverted flags. They no longer represent
only an approximate footprint when a live ride is available. The grid remains;
a larger selected-piece preview and full wrapped name accompany it. Compatible
pieces are grouped as All / Straight / Stations / Turns / Slopes / Banking / Special.
Groups reorder/filter native eligible entries and never bypass restrictions.
Y changes groups; L1/R1 continues cycling the active compatible group in the park.
Advanced speed, seat, covered/inverted, special and backward options remain.

The active build panel gives the native thumbnail more space and keeps slope,
bank/speed, chain, height and cost visible. The endpoint camera leaves the proposed
geometry clear of the panel. Native invalid-query errors replace the valid ghost;
no fabricated invalid geometry or estimated physics is drawn. Station-height
buttons now step by the native land-height increment (16 world units). The old
8-unit adjustment alternated through heights rejected by native TrackPlaceAction;
no native height validation was relaxed.

## Built-track editing

Edit track now opens a spatial selector: the actual selected native track section
is highlighted in the park, with a native thumbnail, full piece name and counter.
L1/R1 (also horizontal D-pad) follows native previous/next connections; vertical
D-pad browses every piece, including disconnected sections. A opens piece actions;
B returns to ride management. B from actions restores the selected piece.
The primary view omits coordinates and raw sequence metadata. Connections/station
details remain explicit advanced actions. Remove, replace, continue, backward,
speed changes and existing guarded native confirmation flows are preserved.
Highlight bits are saved/restored around a synchronous viewport draw, never kept
in simulation/map state. Camera context is intentionally focused on the piece.

## Native news integration

A compact Y News ribbon shows only the current native news item during ordinary
park play. It is absent during construction; history remains native. Y opens the
current detail (or history when empty); X on the dashboard opens history. Valid
native ride/guest/staff references open their modern workspaces. Existing native
research object identifiers open the invention workspace. Native money/campaign/
peeps/graph/award types lead to finance/marketing/guests/park/awards respectively.
No text matching, guessed subject, new event queue or invented severity is used.
Informational, expired and disabled targets remain unavailable.

B returns through the caller: subject to news detail, park shortcut to park,
dashboard history to its previous section. Back to news explicitly opens history.
Dismiss current message is separate from Back and calls native CloseCurrentItem,
which retains native history. Native news timestamps/text are not rewritten.

## Validation

Evidence and 512×384 captures are under captures/rc4/ and evidence/rc4/.
All images are native offscreen runtime captures using user-supplied original
assets. Proprietary assets themselves are excluded from the package/source.

- Rc4IntegrationTests: actual native track elements for station, straight, slopes,
  left/right turns, bank and chain; direction/height preview; full-tick and nested
  suspension restoration; native ghost vs committed geometry; exact native costs;
  blocked/invalid placement; cancel; undo; save with a live ghost and reload without
  it while preserving real geometry/cash; native catalog groups; connected spatial
  editor navigation and transient highlights; ride, finance, research and category
  event handoffs; informational/stale messages; native dismissal/history;
  dashboard return and construction draft preservation.
- PeopleUxTests: central native GuestOpen/StaffOpen, exact guest/news identity,
  stale guest news, tab navigation, live needs/thoughts, tracking/follow/pickup,
  roster return, native staff management, large rosters and single-row footers.
- SelectedTrackEditingTests: advanced selected removal/replacement, guarded stale
  source/context, station access, costs, cancellation, backward/gap recovery and
  save/reload. Existing context-title expectation updated to current source.
- RideFlowTests, ManagementUxTests, ConstructionUxTests, ShopAppearanceTests,
  PauseResumeTests and FirstParkFlowTests cover modern dashboard/ride/finance/
  research/marketing/staff/construction/save journeys. Standalone CTest: 6/6.
- Two older standalone information drivers stop at Ledger entry / Pocket map
  entry: their fixed-row expectations precede RC3 tabbed finance/park workspaces.
  These are recorded failures, not passes. Current ManagementUxTests covers the
  established tabs; a full migration of those older drivers remains outstanding.

Native GUI/CLI and GCC 16.2 Cortex-A53 AArch64 GUI/CLI builds use the existing
NextUI build/package infrastructure. RC3's verified private dependency runtime
was reused byte-for-byte, with headers from matching pinned sources. Matching
public objects/title sequences were retained, and graphics tables regenerated by
the native build. Package checks cover archive CRC/hashes/modes, ARM64 ELF and
dependency closure, resources, launcher/save paths and QEMU GUI/CLI version runs.
See RC4-PACKAGE.md for archive identity and installation.

## Limits and device checklist

No physical Brick is connected here. Screenshots establish native rendering at
512×384; QEMU establishes ARM executable/runtime closure, not device gameplay.
Brick display/controller/audio/performance, ghost visibility on dense real parks,
full translated catalogs, physical suspend/resume and SD-card round trips require
retest. Verify L/R selection, Y grouping/news, advanced editing and caller returns
on the device. The path device accounting/visibility report remains unresolved.

## Reproducing native fixtures

With a configured Ninja GUI build and its dependencies available:

```sh
python3 test/handheld/link-native-test.py /absolute/native-build test/handheld/Rc4IntegrationTests.cpp /absolute/Rc4IntegrationTests
SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy POCKET_RIDE_FLOW_CAPTURES=/absolute/captures /absolute/Rc4IntegrationTests /absolute/public-game-data /absolute/test-profile /absolute/owned-RCT2
```

Use separate profiles for each driver. PeopleUxTests accepts the same arguments
and `POCKET_PEOPLE_UX_CAPTURES`. The output directory contains actual runtime PNGs;
512×384 dimensions and single-row footer are asserted during capture.
