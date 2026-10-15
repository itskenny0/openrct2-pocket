# Desktop versus Pocket feature depth

Living roadmap, source audit 2026-10-09. Baseline: published Pocket b21dd2d.
COMPLETE means the named player task is effectively represented, not literal desktop parity.
PARTIAL domains list their gaps below. MISSING means no Pocket player flow. DEFERRED is a deliberate scope choice.
Native references are relative to src/: desktop windows under openrct2-ui/windows,
Pocket modules under openrct2-ui/handheld, actions under openrct2/actions.
No editor/debug features are counted as ordinary park-play gaps.

## Represented player tasks

| Domain | Classification | Represented tasks / source evidence |
| --- | --- | --- |
| Park / overview | COMPLETE for summary/status; PARTIAL domain | ParkOverview.cpp: native objective, rating, value, guests, open/close review; ParkSetParameterAction. |
| Rides | PARTIAL | RideManagement.cpp: full non-shop roster, ratings/reliability/customers/income/profit, locate, status and admission drafts; RideSetStatusAction, RideSetPriceAction. |
| Ride construction | PARTIAL | FlatRides.cpp and Designs.cpp: eligible non-track flats and conventional descriptor-compatible native saved designs, optional native scenery, previews/actions and entrance/exit recovery. |
| Custom track | PARTIAL | CustomTracks.cpp: descriptor-driven regular-station forward editor, loaded/researched object/type pairs, native curves/slopes/banks/chain/brakes and compatible special/inversion pieces, native track actions, endpoint undo, coordinate block selection, guarded removal/refund and brake/booster speed reviews, gap and circuit/access recovery. |
| Shops / facilities | COMPLETE for placement/basic management; PARTIAL domain | Shops.cpp, ShopManagement.cpp: eligible native objects, rotation, ghost, status, product/common prices, statistics and demolition. |
| Paths | PARTIAL | Paths.cpp: normal/queue surfaces, raised/sloped sequential sections, guarded native undo, ghosts; RideAccess.cpp: connected entrance/exit starts, elevated landings and native connectivity. See SCENERY-PATH-PARITY-COMPLETION.md. |
| Terrain / water | PARTIAL | Terrain.h, Water.h, Surface.h / AreaTools.h: original single-tile actions plus rectangular brush, native smoothing, corners/edges, water and loaded area surface/edge styles. |
| Scenery | PARTIAL full parity; four native categories delivered | SceneryTools.cpp: small/large/wall/banner picker, researched native groups, geometry, heights, ghosts, colour/removal and sign text/styles. SceneryClear.h: guarded native area clearing. Original Trees retained; see SCENERY-PATH-PARITY-COMPLETION.md. |
| Guests | COMPLETE for individual read-only inspection; PARTIAL domain | GuestManagement.cpp: summary, full roster, native action/thought groups and tracked filter, status/needs/thoughts/spending/inventory/preferences, naming/tracking and shared native pickup/placement; identity reuse protection. Native retained ride history with inspection/return is delivered; native persistent follow/stop and camera/menu recovery are delivered; broader filter/map entry points remain gaps; see GUEST-PLACEMENT-COMPLETION.md. |
| Staff | PARTIAL | StaffManagement.cpp: all four native roles, automatic hire, role orders/counters, locate/dismissal and loaded entertainer costumes. Native patrol area editing/clear is delivered; naming, role filtering/counts/wages and native role uniforms are delivered; shared pickup/manual hiring delivered; see STAFF-MANAGEMENT-COMPLETION.md. |
| Finances | COMPLETE for loan/admission; PARTIAL domain | Finances.cpp: native cash/loan/income/expenditure/profit, bounded loan/admission confirmation. |
| Research | COMPLETE for ordinary funding/priorities/progression | ResearchManagement.cpp and ParkSetResearchFundingAction: native current project, estimate, funding/categories, research-filtered Build objects. |
| Marketing | COMPLETE for starting/reviewing native campaigns | MarketingManagement.cpp, ParkMarketingAction: all eligible campaign/target/duration drafts, native costs/progression; no invented campaign cancellation. |
| Park/scenario management | PARTIAL | ParkOverview.cpp, Menu.cpp: objectives/status and native save/load; RightsTools.h adds native land/construction-rights purchase/inspection; broader desktop Park/Map information remains. |
| Save / load | COMPLETE delivered native controller workflows; PARTIAL desktop parity | Menu.cpp: retained .park slots/autosave, real park/autosave folders, preview-only stored metadata, explicit load/discard recovery and named Save As with guarded publication. Native late-import rollback, 32-code-point ASCII entry and desktop file-management/location gaps remain; see SAVE-LOAD-BROWSING-COMPLETION.md. |
| Global information | PARTIAL | Hud.h native cash/date/guests/pause/speed; Dashboard sections provide selected native summaries. |

## Missing depth and recommended treatment

Each row is a meaningful gap, with native authority and existing Pocket foundation.
Complexity refers to implementation, not the number of desktop controls.

| Gap / classification | Desktop capability and native source | Pocket foundation | Complexity / handheld value | Presentation / dependencies / risks |
| --- | --- | --- | --- | --- |
| Ride operations PARTIAL | Ride.cpp operating page; RideSetSettingAction; Ride::getAvailableModes, type flags, LiftData | Existing ride identity, drafts, confirmation, Dashboard | medium / high | Operations list, one bounded native setting draft at a time; mode changes can reset test/vehicle state, waits couple minimum/maximum. First depth package. |
| Ride maintenance PARTIAL | Ride.cpp maintenance; Ride inspectionInterval, lastInspection, downtime, dueInspection, availableBreakdowns | Existing stats and native setting action | small / high | Maintenance page with interval draft, native refurbishment review and assigned/nearest mechanic locate; distinguish rides without breakdowns. See Ride Management Phase 3. |
| Ride demolition COMPLETE | DemolishRidePrompt.cpp; RideDemolishAction; RideGetRefundPrice uses native track query calculations | Shop confirmation pattern and ride stale identity | medium / high | Delivered named destructive Cancel-first review, native prompt refund calculator and stale quote validation; see RIDE-DEPTH-CHECKPOINT.md. |
| Vehicles PARTIAL | Ride.cpp vehicle page; RideSetVehicleAction; Ride::updateMaxVehicles, loaded vehicle object limits, proposedNumTrains/proposedNumCarsPerTrain | Ride drafts/identity available | medium / high | Train/car counts and requested/effective review delivered in Vehicles/Dispatch checkpoint. Native station/object limits, closure and clamping apply. Loaded researched alternative vehicle selection and descriptor-supported reversed trains delivered in RIDE-MANAGEMENT-COMPLETION.md; native query/review/effective bounds apply. |
| Mode-specific options PARTIAL | Ride.cpp launch/time/laps/swings/rotations/circuits; load mask/synchronization now represented; RideSetSettingAction | Operations setting model | medium / high | Delivered duration/laps/swings/rotations/launch/speed/circuits and native dispatch controls with typed labels/units and descriptor visibility. No-vehicle maximum-rider fallback is delivered in Ride Management Phase 2. |
| Ride finance/customer depth COMPLETE within summaries/station scope | Ride.cpp income/customer pages; Ride totalProfit, popularity, satisfaction, queue/station information | Existing read-only stats | small / medium | Delivered separate Finance/Customers pages and per-station queue/access summaries; native undefined values, scaling and rolling windows are respected. |
| Ride music/appearance/naming COMPLETE within milestone scope | Ride.cpp music/colour/name; RideSetSettingAction, RideSetAppearanceAction, RideSetNameAction | Picker/draft architecture | medium / medium | Delivered native music toggle/style, indexed appearance/entrances, track painting, random vehicle colours and reusable ride/shop naming keyboard. See RIDE-MANAGEMENT-COMPLETION.md. |
| Ride graphs COMPLETE within native sample-view scope | Ride.cpp graph page; RideMeasurement sampling and scrolling | Native bounded controller graph view | medium / low | Delivered bounded native speed/altitude/G-force graphs, time ranges and scrolling; waiting/unsupported sampling states remain explicit. |
| Other ride types / special construction PARTIAL | NewRide.cpp, RideConstruction.cpp; RideCreateAction, TrackPlaceAction, MazeSetTrackAction, TrackDesignAction | Flat/custom/saved adapters | architectural / high | Conventional descriptor-compatible families and native maze half-cell build/move/fill/access/recovery delivered. Maze segment previews and further special geometry remain partial; see MAZE-SPECIAL-RIDE-COMPLETION.md. |
| Custom editing breadth PARTIAL | RideConstruction.cpp backward/selected-block editing, special tracks, brake speeds, design export | Descriptor-driven forward editor and selected native block removal/speed controls | architectural / high | Disconnected forward/rear endpoints, native backward core, guarded two-step replacement, covered/inverted core and seat placement are delivered. Complex/diagonal coverage, positive transitions, special parameters and atomic/station/inverted replacement remain; see ADVANCED-RIDE-EDITING-COMPLETION.md. |
| Shop appearance/naming COMPLETE within native object channels | Ride.cpp shop colour flags/random merchandise/name; RideSetAppearanceAction/SetNameAction | Shop management/picker and shared keyboard | medium / medium | Actual object-supported building colours and random merchandise colours, native palette, Cancel-first cost review, allocation/object/channel/baseline guards and .park persistence delivered. Unsupported loaded objects are explicit; no invented shop vehicle/station/music controls. See SHOP-APPEARANCE-COMPLETION.md. |
| Connected path builder PARTIAL | Footpath.cpp bridge/tunnel next-endpoint building; FootpathPlaceAction | Existing-path endpoint selection, sequential flat/rise/descend/elevated normal/queue placement | medium / high | Native incoming geometry/ghosts/costs, pending guard and options recovery. Entrance endpoint start, undo/history and asset-backed routing validation remain. See PARK-CONSTRUCTION-COMPLETION.md. |
| Path furniture COMPLETE native placement/removal core; PARTIAL exhaustive validation | Footpath.cpp additions; FootpathAdditionPlaceAction/RemoveAction | Shared object picker/targeting | medium / medium | Delivered loaded/researched additions picker, native path geometry, placement/removal/ghosts and persistence. Named bench/bin/lamp/fountain/queue-screen fixtures, queue/slope restrictions and persistence pass; expansion exhaustive validation pending. |
| Terrain brush/edges COMPLETE requested core; PARTIAL broader depth | Land.cpp, Water.cpp, LandRights.cpp; LandRaise/Lower/SmoothAction, SurfaceSetStyleAction, LandSetRightsAction | Existing one-tile tools | medium / high | Rectangular 1–64 brushes, native corner/edge selection, smoothing, area water and surface/edge styles delivered. Colour-choice controls remain; see PARK-CONSTRUCTION-COMPLETION.md. |
| Other scenery PARTIAL | Scenery.cpp; small/large/wall/banner placement, colour/removal actions | Reusable picker and tree preview adapter | medium to architectural / high | Four native categories now have placement/ghosts, rotation/elevation, recolouring, removal and sign text; banner styles, bulk tools, filtering and remaining parity are listed in PARK-CONSTRUCTION-COMPLETION.md. |
| Guest grouping/filtering COMPLETE requested native core; PARTIAL dedicated global thought-type entry | GuestList.cpp thought/action groups and filters | Full roster and native details | medium / medium | Native action/thought argument groups, counts, tracked/name intersections, ride rider/queue/thought filters and identity-stable whole-roster map/inspection/return delivered; see GUEST-INFORMATION-COMPLETION.md. |
| Guest pickup COMPLETE named core; guest domain PARTIAL | Guest.cpp; PeepPickupAction | Inspect/locate | medium / low | Shared native guest/staff placement, native cancel restoration, menu recovery and manual hiring delivered. Native retained ride history and inspection/return are delivered. Native follow/stop and camera/menu/load cleanup are delivered. Dedicated global guestsThinkingX thought-type entry remains an ordinary parity gap; native group, name, tracking and ride-specific filters/map handoffs are delivered. |
| Staff patrol/roles COMPLETE named core; remaining staff depth PARTIAL | Staff.cpp, StaffList.cpp; StaffSetPatrolAreaAction, StaffSetCostumeAction, StaffHireNewAction | Handyman/mechanic roster/order framework | medium / high | All roles/costumes and native patrol area Add/Remove/Cancel-first Clear delivered. Naming, filtering, counts/wages and uniforms delivered; shared pickup/manual hiring delivered; see STAFF-MANAGEMENT-COMPLETION.md. |
| Finance history/graphs PARTIAL | Finances.cpp expenditure tables and historical graphs; Park expenditureTable and history | Finance panel | medium / medium | Native retained-period/category ledger and desktop 64-sample cash-less-loan/profit/park-value graphs delivered; company-value history has no native array. See FINANCE-RESEARCH-INFORMATION-COMPLETION.md. |
| Research invention list COMPLETE native catalog core; PARTIAL broader information | Research.cpp finished-item list, Research state | Funding/priorities complete | small / medium | Native invention-record catalog, seven category filters, real availability, object previews and stale descriptor guards delivered; native order retained, no invented dates or manual unlocking. See FINANCE-RESEARCH-INFORMATION-COMPLETION.md. |
| Land/construction rights COMPLETE requested purchase/inspection scope | Map.cpp, LandRights.cpp; LandBuyRightsAction/LandSetRightsAction | Native tile selection/action feedback | medium / high | RightsTools.h: native area inspection and Cancel-first LandBuyRightsAction purchase review, counts, scenario availability, cost/ownership snapshots, stale guards and save/reload; no direct ownership mutation. |
| Park news/awards/history PARTIAL | Park.cpp, RecentNews.cpp, NewsItem and park histories | Overview, management panes | medium / medium | Native recent/archive pages with full wrapped text, expiry/identity guards and locate; read-only active awards and native rating/guest-history graphs with undefined samples. Guest/staff/ride subject inspection and recovery are delivered. Notification settings and broader research/finance/campaign news subject links remain; see PARK-MAP-INFORMATION-COMPLETION.md. |
| Arbitrary save/design browsing COMPLETE named workflow; native format/family limits retained | LoadSave.cpp, TrackDesignList.cpp; native platform environment/importers/repository | Native park/autosave and design folder/metadata/preview/placement flow, reviewed named Save As | delivered / high | Stable selection, UTF-8 path detail, file/destination guards and recoverable native errors. No new flat-design/substitution support or desktop install/delete/rename/new-folder/Windows-drive flow; see SAVE-LOAD-BROWSING-COMPLETION.md. |
| General map/information PARTIAL | Map.cpp, Park.cpp, GuestList.cpp | Locate/navigation and Dashboard | medium / medium | Native Pocket map now shares desktop colours/rotation, bounded tile navigation, viewport box, zoom, native ownership labels and explicit locate. Native guest/tracked/staff-role overlays, actual subject rosters and linked inspection/recovery plus news/awards/history are delivered. Native guest follow and filtered guest-map rosters are delivered. Train overlays, non-guest finer map filters and editor-only map operations remain; see PARK-MAP-INFORMATION-COMPLETION.md. |
| Desktop-only pointer affordances DESKTOP-ONLY | Resizable overlapping windows, hover tooltips, drag widgets | Semantic controller architecture | architectural / low | Replace with bounded pages/context details; literal desktop arrangement inappropriate at 512×384. |

## Order and maintenance

First: Ride Details / Operations / Maintenance / demolition. Next: vehicle and
mode-specific dispatch depth as one complete ride configuration workflow, unless
hardware feedback identifies a blocker. Terrain/rights, scenery category and path furniture core are now delivered;
Staff roles/patrol and shared guest/staff pickup are now delivered. Next: ride construction breadth and native track editing while retaining documented construction/guest parity gaps. Keyboard and arbitrary browsing
are cross-cutting completeness tasks, not reasons to redesign the shell.
Update this document in each depth checkpoint. Treat COMPLETE tasks as regression
contracts. Public component captures do not prove asset-backed coaster operation.

## First depth package delivered

RideManagement.cpp now supplies descriptor-filtered mode, wait limits/toggles and
lift-speed drafts, maintenance interval and native information, total queue length
and confirmed native demolition. These named tasks are COMPLETE within their
current supported native setting scope; at this earlier checkpoint Rides remained PARTIAL for vehicle, full
dispatch/mode-specific options, appearance/music, refurbishment and graphs. See
RIDE-DEPTH-CHECKPOINT.md for exact tests and missing asset-backed validation.
The next package at that checkpoint was vehicle/dispatch configuration, not a new UI
shell. Refurbishment and mechanic locating use RideDemolishAction renew and native
RideGetAssignedMechanic/RideFindClosestMechanic; medium complexity, medium player
value, reuse Maintenance with closure/empty-ride checks and staff identity safety.

## Vehicles / Dispatch package delivered

Vehicles provides descriptor/object-filtered train and car-count drafts through
RideSetVehicleAction. The native updateMaxVehicles calculation supplies station,
mode/block and loaded-object limits and the effective count shown in review.
Dispatch adds native load threshold, wait-for-load, supported adjacent-station
synchronization and conditional leave-on-another-arrival. Unrelated departure
bits are preserved. Existing wait-time controls remain in Operations.
Counts/dispatch are COMPLETE within this named scope; Named train/car, loaded vehicle-object and reversed-train controls are COMPLETE within the current native configuration scope. Mode-specific options are extended by the next package documented below. No new construction, UI
shell or save format. See VEHICLES-DISPATCH-CHECKPOINT.md for validation and
public-fixture limitations.

The next package at that checkpoint was descriptor-driven mode-specific operating parameters
within Operations, preserving the vehicle/dispatch workflows. Vehicle object
selection, appearance/music and graphs followed in the completion milestone.

## Mode-specific Operations package delivered

COMPLETE within the desktop mode-tweak vocabulary requested for Pocket: native
Dodgems duration, race laps, swings, forward/backward/plain rotations, supported
launch modes and station-to-station speed. Descriptor bounds, native multipliers,
velocity formatting and the Dodgems ten-second spinner increment apply.
Circuit counts use Ride::canHaveMultipleCircuits; native cable-lift multi-circuit
restrictions are respected. Typed drafts use RideSetSettingAction and revalidate
mode/operation/vehicle state. No raw generic operationOption is exposed.

At that checkpoint, Mode-specific Operations remained PARTIAL for no-vehicle maximum-rider capacity
(the desktop fallback), other niche settings and track-piece brake parameters.
Refurbishment/mechanic assistance, appearance/music and richer finance/customer
information were still open at that checkpoint; they are delivered below.
See MODE-OPERATIONS-CHECKPOINT.md for scope, native sources, tests and limits.

The next workflow at that checkpoint was Maintenance → review native breakdown/
inspection state → locate assigned/nearest available mechanic → native queried
refurbishment review → Cancel-first confirmation → return/save/reload. Reuse
Maintenance, Staff identities and existing demolition/renew action machinery;
no patrol editing or new staff roles.

## Ride Management completion: delivered milestone

Vehicle-object selection and reversed trains delivered through native typed drafts;
loaded/research filtering, closure and effective counts remain native. All eight
requested phases are now delivered: music, maintenance actions, appearance,
ride/shop naming, customer/financial summaries, graphs, scoped track painting,
random vehicle colours and station detail. See RIDE-MANAGEMENT-COMPLETION.md
for exact scope and validation limits. The domain remains PARTIAL for broader
construction and desktop parity; no prior gameplay phase was redone.

## Park construction checkpoint

See `PARK-CONSTRUCTION-COMPLETION.md` for four incremental native construction
phases, controller mappings, exact published phase SHAs, regression evidence and
eleven genuine 512×384 component captures. Broader scenery/terrain/connected-path
parity remains PARTIAL; land/rights purchase and furniture placement/removal core
are complete within the named scope. The 100% ordinary-gameplay goal is unchanged.


## Guest / placement checkpoint

See GUEST-PLACEMENT-COMPLETION.md: native action/first-thought groups and tracked
filter, stable filtered rosters, guest naming/tracking/preferences, shared
PeepPickupAction placement/cancel and manual staff hiring are delivered. Native,
asset-backed SDL and actual 512×384 park-rendered captures cover the milestone.
Guest domain remains PARTIAL for ride history, persistent follow, broader desktop
filter entry points/map display and filter restoration after closing into placement.
Staff pickup/manual-hire gap is closed; hardware/routing/patrol validation remains
explicitly limited. The 100% ordinary-gameplay goal and construction gaps remain.


## Ride construction breadth follow-up

RIDE-CONSTRUCTION-COMPLETION.md Phase 1 replaces Wooden-only forward construction
with native descriptor-backed conventional track families and compatible special
pieces. Five native families plus original Wooden gameplay regressions pass.
The domain remains PARTIAL for editing/backward/selected-block controls, design
breadth, covered/inverted variants and distinct maze/special geometry; no ordinary
parity gap is removed from the 100% roadmap.

Saved-design breadth checkpoint: compatible conventional native families and optional native design scenery are delivered in [RIDE-CONSTRUCTION-COMPLETION.md](RIDE-CONSTRUCTION-COMPLETION.md). Native maze designs are now delivered in MAZE-SPECIAL-RIDE-COMPLETION.md; flat design adapters, object substitution choice and broader editing remain ordinary parity gaps. Goal remains 100% ordinary gameplay.

Ride construction editing checkpoint: selected native block removal/refunds and brake/booster speed drafts, Cancel-first reviews, gap recovery and a Select-based Brick camera route are delivered. [RIDE-CONSTRUCTION-COMPLETION.md](RIDE-CONSTRUCTION-COMPLETION.md) records three independently validated published phases and 29 asset-backed captures. Overall ride construction remains PARTIAL: backward/disconnected-endpoint editing, maze/special adapters, additional parameters, variant controls and design export/manual elevation remain on the 100% ordinary-gameplay roadmap.

Maze checkpoint: native researched object selection, half-cell build/move/fill, Cancel-first removal, access, existing-ride recovery and native persistence are delivered. Construction remains PARTIAL; [MAZE-SPECIAL-RIDE-COMPLETION.md](MAZE-SPECIAL-RIDE-COMPLETION.md) preserves exact remaining preview, geometry, design and operation-validation gaps. The goal remains 100% ordinary gameplay.

Maze saved-design checkpoint: native import, exact loaded/researched object eligibility, four orientations, native height, ghosts/cleanup, scenery options, Cancel-first native cost review, execution and access/recovery are delivered. Export, manual elevation and substitutions remain partial.

Maze recovery: native existing tile/quarter selection now reaches disconnected maze sections. Mini Golf all five holes/four orientations, multi-tile actions, costs/refunds, undo/recovery and persistence pass public native fixtures with the existing adapter. Full Mini Golf operation and transport/water family asset-backed validation remain PARTIAL.

Maze/special final checkpoint: three independently validated publications culminate
in implementation bf05719e3457fca5831a4aad2f5df48b7bc839a8; see
[MAZE-SPECIAL-RIDE-COMPLETION.md](MAZE-SPECIAL-RIDE-COMPLETION.md) for canonical/local
tree equality, 14 native asset-backed captures, all regression results and precise
gaps. Overall ride construction and special-family parity remain PARTIAL toward
100% ordinary gameplay, including ghosts, further editing/design authoring and
individual family operation validation.

Conventional selected-section continuation: the native block roster can resume the
forward editor at the gap of a selected disconnected component. Native traversal,
costs and undo remain authoritative; deleting its anchor ends context safely.
Backward/replacement/variant/export depth remains PARTIAL; see
[TRACK-EDITING-DESIGN-COMPLETION.md](TRACK-EDITING-DESIGN-COMPLETION.md).

Native brake/booster placement now has bounded X/Y speed controls, matching ghost,
query and execute parameters; existing selected-block speed reviews remain.
Full seat/covered/inverted variant parity remains PARTIAL.

Design placement: exact manual elevation, automatic-height recovery and compact X
options are delivered for supported conventional/maze designs. Native query/ghost/
cost review/execute still own placement. Player-facing export/scenery selection and
substitution choices remain PARTIAL; ordinary special-family operation is pending.

Operation evidence follow-up: Mini Golf now passes a public-fixture action-built
closed circuit, native access/Open and save/reload. Maze passes native opening in
public tests and Factory Capers asset-backed capture. Sustained guest operation,
asset-backed Mini Golf and transport/water-family operation remain unvalidated.
Overall construction stays PARTIAL toward 100% ordinary gameplay; export,
backward/replacement and variant parity remain on the roadmap.

Track editing/design follow-up comprises four validated implementation publications
through 0a915988a0264402ff50a161430efd9ed5b1b715 with canonical/local tree equality.
The new checkpoint preserves exact remaining ordinary gaps and 14 genuine
asset-backed viewport/component captures. No full gameplay-completeness claim.

Native design authoring follow-up: player-facing tested/rated export now uses the
UTF-8 keyboard, Cancel-first export/overwrite, native T6Exporter round-trip and
repository refresh with source/file stale guards. See DESIGN-AUTHORING-COMPLETION.md.
Scenery authoring, substitution choices, variants/seat parameters and backward/
replacement editing remain PARTIAL; the 100% ordinary-gameplay goal is unchanged.

Native scenery authoring now offers native nearby selection plus individual descriptor
inclusion/exclusion, no-scenery reset, counts and locked cost-free export review.
Native multi-tile grouping, bounds/conversion and object-format rules remain authoritative;
changed source/scenery rejects. Arbitrary outside-nearby/spatial selection and vehicle
substitution remain PARTIAL; see DESIGN-AUTHORING-COMPLETION.md.

Seat rotation placement is delivered for native RTD-eligible pieces through bounded
Select-picker controls and matching query/ghost/execute; native shared-bit speed
restrictions are retained. Native save persistence and selected seat baseline guards
pass. Covered/inverted variants, selected seat editing and other parameters remain
PARTIAL. See DESIGN-AUTHORING-COMPLETION.md for exact native dependencies/validation.

Selected conventional blocks with native connected predecessors now offer guarded
Cancel-first removal/rebuild. Native refund is explicit; the original supported draft
resumes at the gap and A builds only by player intent. Multi-sequence predecessor
and stale/quote guards pass native tests. Backward, atomic/station/inverted replacement,
covered/alternative variants and broader operation remain PARTIAL on the 100% roadmap.

Final authoring validation: Release GUI/CLI, nine native/SDL suites, CTest 3/3 and
four private-asset capture/regression workflows PASS. Twelve genuine 512×384
viewport/component captures accompany DESIGN-AUTHORING-COMPLETION.md. Stand-up
construction and original Wooden natural queue use are evidenced; sustained special
ride throughput, AArch64 and physical Brick remain unverified. Overall construction
and design authoring remain PARTIAL with precise ordinary gaps retained.

Advanced editing Phase 1: native rear-gap selection and backward construction are
delivered through block rosters, TED/native traversal and strict query/execute source
baselines. Straight, multi-tile turn, slope, held-A, native cost/refund/undo and save
coverage pass. Complex/diagonal/inverted family coverage and full editing parity remain
PARTIAL; see ADVANCED-RIDE-EDITING-COMPLETION.md. No ordinary gap is removed.

Advanced variants: native covered mappings/base groups and inverted paint/vehicle
eligibility, actual-bank connections, inverted-start Flying station defaults and
forward/backward source-variant recovery are delivered. Native query/ghost/execute,
controller covered utility, unsupported inversion, undo and save persistence pass.
Positive transition and exhaustive family/variant coverage, direct selected-seat
adjustment and other special parameters remain PARTIAL; see advanced checkpoint.

Representative family validation now passes action-built Monorail, Dinghy (covered
segment) and Flying (inverted) closed circuits, native access/queue-normal landings,
Open and save/reload using initialized native fixtures with real sprites. Empty
Monorail movement is observed for 4096 ticks. No sustained guest use, full park-network
routing, Dinghy/Flying motion or exhaustive family operation is claimed. Overall
construction remains PARTIAL; see ADVANCED-RIDE-EDITING-COMPLETION.md.

Advanced milestone final validation: Release GUI/CLI, ten native/SDL suites, CTest
3/3 and four asset-backed regressions PASS. Seventeen real production-rendered
512×384 fixture captures document the new workflows. Overall construction remains
PARTIAL. Next highest-value subsystem: scenery/path construction parity and connected
path recovery, preserving all ordinary gaps in the matrix and checkpoints.

## Scenery/path parity follow-up

SCENERY-PATH-PARITY-COMPLETION.md Phase 1 delivers researched loaded native scenery
group filtering and guarded Cancel-first banner no-entry/text/primary style and
scrolling sign channel controls. Four-category placement and trees are preserved.
Scenery and Paths remain PARTIAL; later phases/gaps are recorded in that checkpoint.

Scenery parity Phase 2: native category-selective 1–64 tile area clearing with
Cancel-first cost/refund review, copied full-object stale guards, native whole-large
removal and path preservation/opt-in removal is delivered. Native multi-tile clear
query accounting is corrected and regression-tested against execution. Connected
access starts/undo, eyedropper, bulk placement and terrain surface colour remain
PARTIAL; see SCENERY-PATH-PARITY-COMPLETION.md.

Scenery/path parity Phase 3 adds actual native ride entrance/exit connected starts,
fixed landing height/direction, guarded session undo/refunds with Cancel-first review,
new-section-only history and existing-junction protection. Eight-row path options
remain controller-visible. Native action-built queue association, elevated extension,
menu recovery and persistence are tested; sustained new-route guest throughput and
physical Brick remain unproven. Full parity remains PARTIAL; see
SCENERY-PATH-PARITY-COMPLETION.md for precise gaps.

## Native park map / information (2026-10-10)

Pocket Overview now links a native park map, recent/archive news, active awards and native park rating/guest graphs. Map colours, rotation and viewport geometry are shared with desktop Map.cpp; peep overlays use native entity positions/colours. Bounded camera-relative tile axes, whole-park/1–8x presentation, explicit locate, actual guest/tracked/staff-role/ride rosters, generation guards and inspection return context are delivered. Native graph undefined samples/date order are preserved and dark-panel labels are opt-in without changing desktop defaults. See PARK-MAP-INFORMATION-COMPLETION.md.

General map/information remains PARTIAL for moving-train overlay, finer task filters, persistent follow and editor-only map operations. News settings/acknowledgement and research/finance/campaign subject handoffs remain ordinary gaps. Active awards use the only native current-award state; no invented historical award log. Finance historical ledgers/graphs and research finished-item catalog remain the recommended substantial information milestone. Every previous ordinary construction, design, guest/staff and file-management gap is retained.

## Finance/research validation closeout (2026-10-10)

Native ledger/history/catalog and management news handoffs are delivered and
validated; see FINANCE-RESEARCH-INFORMATION-COMPLETION.md. Thirteen native/SDL
suites, CTest 3/3 and all four supplied-asset construction/design regressions PASS.
Twenty-five reviewed production 512x384 scenario renders cover negative/undefined
history, native catalog filters/previews and campaign/research news return flows.
Money-news capture is unavailable in that natural scenario; native handoff tests
pass. Hardware/AArch64/performance and broader ordinary parity remain unvalidated.
The next guest phases are still pending at this closeout.

## Guest information Phase 1 (2026-10-13)

Native retained ride-history browsing and existing ride inspection/return are
delivered. Native RideManager order and Guest::hasRidden membership are respected;
no chronology, visit dates or counts are invented. Empty/stale/missing/reused targets
and save/load are validated. GUI/CLI build, focused native/SDL GuestInformationTests,
GuestCompletionTests, PresentationLifecycleTests, full asset-backed PathActionsTests
and CTest 3/3 PASS. Follow and broader filters/map entry points remain pending. See
GUEST-INFORMATION-COMPLETION.md for scope and publication/preservation records.

## Guest information Phase 2 (2026-10-13)

Native main-viewport persistent follow/stop is delivered, with B inspector recovery,
D-pad stop/pan, native zoom/rotation and departed/reused/picked/load/exit guards.
Expanded native/SDL GuestInformationTests, PresentationLifecycleTests, asset-backed
ParkMapInformationTests and CTest 3/3 PASS. Follow state is transient UI identity,
not additional serialized gameplay state. Guest filter/map entry workflows remain
Phase 3; broader ordinary and hardware validation gaps remain explicit.


## Guest information Phase 3 (2026-10-13)

Native rider/queue/thinking-about-ride filters link Ride Customers to stable guest
rosters, inspection and original ride return. Existing action/thought groups and
tracking remain; controller keyboard name search and tracked intersections apply
to every roster. Whole-filter map overlays and subject lists use actual native
membership, shared marker positions/colours and guarded IDs. Inspection/Back keeps
map/filter/roster/ride-or-group context. No native statistics or serialized Pocket
gameplay state is added. GUI/CLI build, expanded native/SDL GuestInformationTests,
asset-backed ParkMapInformationTests, GuestCompletionTests, PresentationLifecycleTests,
full asset-backed PathActionsTests and CTest 3/3 PASS; see the incremental checkpoint.
Dedicated global guestsThinkingX entry, moving-train overlay, editor operations,
physical Brick/AArch64 and sustained dense-park performance remain exact gaps.


Publication: native filters/map Phase 3 `8d318517ae3f05d4a5496746e25d252bb4795844`,
local validated `e3593c2`, exact tree `1400355d771ac8fdc3173e2ea34b9abb3caebe66`.
Previous canonical phases: finance closeout `3cb124cf9e7014c1685a61d819db8598ce7dbf08`,
guest history `784244aa2db522cae00e758505aa0332ce0fdd40`, follow
`b242e5c7d9aa2624486ae1aae980f3ccd703dc64`. Guards and actual validation/gaps
are recorded in GUEST-INFORMATION-COMPLETION.md and the finance checkpoint.


## Scenery selection and landscaping Phase 1 (2026-10-13)

Native scenery eyedropper now selects eligible real small/large/wall/banner/furniture
objects and appropriate copied placement settings, with stale/ghost/descriptor guards,
non-origin large height and native banner/path offset handling. Held A selects without
placing, changing the source element or charging money. Existing groups, styles,
clearing and placement are preserved. GUI/CLI, expanded ConstructionTests and asset-
backed native/SDL SceneryPathParityTests, PresentationLifecycleTests and CTest 3/3 PASS.
Three inspected production 512×384 fixture captures are retained for the final ZIP.
Search, bulk placement and terrain colours are the next independently published phases;
all broader ordinary and hardware gaps remain. See SCENERY-LANDSCAPING-COMPLETION.md.


## Scenery selection and landscaping Phase 2 (2026-10-13)

Native scenery/furniture search now intersects loaded/researched objects and existing
groups, using desktop native names/identifiers/authors/filenames. Controller keyboard
apply/cancel/clear, explicit empty results, X search, stable descriptor identities,
previews and placement handoff are delivered without changing native accounting or
save format. GUI/CLI, expanded asset-backed native/SDL SceneryPathParityTests,
ConstructionTests and PresentationLifecycleTests PASS; four inspected 512×384 native
fixture captures are retained. Native bulk placement and terrain colours follow;
remaining ordinary and hardware gaps stay explicit. See SCENERY-LANDSCAPING-COMPLETION.md.

## Scenery selection and landscaping Phase 3 — 2026-10-13

Phase 2 published f20751d (tree 6a38c38), preserving native eyedropper/name search.
Shared desktop/controller native small-scenery scatter now adds bounded size/density,
native area/ghost preview, released-A review/apply, frozen proposals/identity/cost
checks, pending callback guards, partial native results/actual cost and save cleanup.
GUI/CLI, expanded ConstructionTests, asset-backed SDL SceneryPathParityTests and
PresentationLifecycleTests PASS; captures 08–13 inspected. See SCENERY-LANDSCAPING-
COMPLETION.md for exact recovery/publication/validation provenance. Scenery remains
PARTIAL: continuous wall drag/repaint/pick equivalents, Shift Z search, network scatter
and next terrain surface/edge colour phase. Final regression/capture ZIP pending.

## Scenery selection and landscaping Phase 4 — 2026-10-13

Phase 3 native scatter published ee2cbd3, complete tree ef54f2c. Area paint now exposes
actual loaded-object surface/edge primary-colour channels and desktop palette/defaults,
selected-colour native icon previews, Cancel-first native quote, copied object/terrain/
flags/cost stale guards, pending callback/repeat guards and load/exit cleanup. Quick
surface selection hands off into shared one-tile colour/area options. Native
SurfaceSetStyleAction now preserves unsupported primary edge encoding consistently
with its Query/surface-channel flag policy. Expanded native/SDL construction/scenery
and presentation checks PASS; captures 14–19 inspected. See SCENERY-LANDSCAPING-
COMPLETION.md. Colour capability is complete for supported loaded native definitions;
network review remains unvalidated/unavailable. Scenery/landscaping overall remains
PARTIAL for the exact drag, network, modifier and broader ordinary roadmap gaps.
Final preservation sweep and capture ZIP remain next.

## Scenery/landscaping validation handoff — 2026-10-13

All four implementation phases are published through 3882849 (source tree 92c2acc):
native placed-object eyedropper; eligible native name/descriptor/author search;
shared desktop/native small-scenery scatter with review, callbacks and honest partial
results; supported native surface/edge colours, native previews and stale/pending
paint review. Native fixed-edge colour preservation is corrected in the native action.
Fourteen named fixed-source native/SDL regression drivers PASS, including finance/
research, guests/staff/map, paths and ride/construction preservation; CTest 3/3 PASS.
Asset-backed RideConstructionCapture, MazeConstructionCapture and original Wooden/
natural-guest CustomCoasterCapture returned confirmed exit 0. DesignCapture is
unconfirmed after workspace disappearance and requires rerun.

The environment is offline again. The nineteen capture files were generated but final
all-file inspection/checksum/provenance ZIP and download remain UNFINISHED. Source is
published; no missing test/package/hardware result is claimed. See the exact
interrupted closeout and standalone resume instructions in
SCENERY-LANDSCAPING-COMPLETION.md. Preserve delivered source; complete this capture
closeout before a new substantial subsystem. Remaining drag/modifier/network and
broader ordinary gaps remain PARTIAL on the 100% roadmap.


## Scenery/landscaping final capture closeout — 2026-10-14 UTC

The interrupted handoff above is superseded. All four implementation phases remain
published at unchanged production source 3882849 / code tree 92c2acc. Native Release
GUI/CLI build and expanded asset-backed native/SDL SceneryPathParityTests rerun PASS;
DesignCapture on African Diamond Mine returned confirmed exit 0; CTest rerun 3/3 PASS.
Previous fourteen-driver preservation sweep and confirmed asset-backed ride, maze and
original Wooden/access/natural-guest regressions retain their recorded results.

All nineteen final production native viewport/Pocket component fixture PNGs were
individually inspected at 512×384. Actual dimension/SHA256/provenance manifest and
README are saved with OpenRCT2-Pocket-Scenery-Landscaping-Captures.zip: 133606 bytes,
SHA256 e95c7afb7952d2464964ee09e6fe162b81fc3b71130500e6fcc2bdc21f29c5b8.
The saved ZIP survived another local reversion; its exact checksum and every image
hash were verified after recovery. No private asset/archive/park/design is included.
See SCENERY-LANDSCAPING-COMPLETION.md for completed results, preservation/publication
history and precise provenance. This requested implementation/capture milestone is
closed. Scenery/landscaping remains PARTIAL for continuous wall/repaint/eyedropper drag,
Shift automatic Z-search, asynchronous native network scatter/paint review and all
broader ordinary roadmap gaps. Physical Brick/AArch64, sustained dense-park performance
and sustained operation of every ride family remain unestablished.


Verified final closeout publication: canonical 6033832f91c7050c3ec35c92b70922b784913244,
local safety 03a966a3e3e2f0d938827ffe8360828268586f9c, complete matching tree
5cdc86afb52088f6ee36f097e307b364f5607e09. Native parent b2db7e0, changed blobs/full
tree and expected_sha/force=false fast-forward were verified; final ref read back.
This publication-history record changes docs only, preserving validated source.

## Save/load browsing Phase 1 — 2026-10-14 UTC

Native park/autosave folder browsing extends the retained three Pocket slots.
Desktop park filter, configured saves directory, parent navigation, paged real entries,
long-path detail, explicit empty/error and Cancel-first active-park load review are
implemented. File byte/size/time identity guards reject stale selections; native
importer failure recovery retains its documented early-preservation/late-title limits.
Named Save As and design folder browsing remain pending. See SAVE-LOAD-BROWSING-COMPLETION.md.

## Named Save As Phase 2 — 2026-10-14 UTC

Controller folder destination and existing-file/name entry now feed native .park
serialization with Cancel-first Save/Replace review, immutable destination/folder
identity and prepublication revalidation. Existing slots/autosave/current-save identity
and success-only save-and-exit retained. CTest 5/5 and asset-backed native controller
round-trip/stale/failure recovery pass. Design file browsing remains pending; keyboard
retains UTF-8 but types only labelled ASCII and bounds names to 32 code points.

## Save/load/design browsing completion — 2026-10-14 UTC

Native park/autosave browsing, preview-only stored .park metadata, explicit load and
Cancel-first active-park review, named folder/filename Save As and immutable overwrite
review/publication are delivered. Native design repository picker X opens folder/file
inspection with native metadata/preview/eligibility and existing placement/height/
rotation/scenery/access handoff; cancel returns to selected file. Full-byte stale file
and folder/destination guards, corrupt/unreadable recovery, current-save identity,
failed-save/no-quit and native successful-save/quit-request integration pass.
See SAVE-LOAD-BROWSING-COMPLETION.md for exact API/format/location limits. CTest 5/5,
all fourteen preservation drivers and four asset-backed regressions pass. This closes
the named browsing workflow; flat/special design/substitution gaps, 32-code-point ASCII
keyboard entry, native late-load rollback limits and overall ordinary gameplay PARTIAL
remain explicit. No physical Brick/AArch64 results are claimed.

Save/load/design browsing implementation published as 2f0f7caad66b901b35bee646a06ff41ebd8f8014,
matching local c82ebd09 / full tree 1112565763e0fb5e07dd21bcbea6ced3e4f8694e.
Completed inspected 24-PNG capture ZIP saved (SHA256 812dec5dcff5d0297b38bd9de7885afb38bd86e74343a0e6987dae4487d975a6).
The next substantial ordinary gameplay gap is broader native selected-track editing/
replacement, especially station/inverted/no-predecessor cases and actual failure/recovery
semantics in ADVANCED-RIDE-EDITING-COMPLETION.md. Do not rebuild this browser milestone.

## Selected-track inspection Phase 1 — 2026-10-14 UTC

Native block context now exposes no-predecessor/station geometry, native shape
connections and actual access references, with stable block-roster return. Exact
track/station/access/object review guards run inside the native GameAction wrapper.
See SELECTED-TRACK-EDITING-COMPLETION.md. Replacement breadth remains pending in this
incremental phase; overall construction remains PARTIAL.

## Selected-track replacement Phase 2 — 2026-10-14 UTC

Orthogonal no-predecessor/station/inverted/covered selected blocks now have guarded
native removal and explicit exact-origin rebuild with retained parameters and native
accounting. Cancel-first source/geometry/descriptor reviews, fixed-origin draft and
B return avoid accidentally removing a predecessor. No atomicity/restoration claim.
Focused native/SDL and save/reload evidence is in SELECTED-TRACK-EDITING-COMPLETION.md;
connected access/circuit closeout and exact remaining family gaps remain Phase 3.

## Selected-track editing Phase 3 closeout — 2026-10-14 UTC

Ordinary orthogonal selected-block inspection and explicit native remove/rebuild now
include no-predecessor, station, supported inverted/covered contexts, retained seat/
speed/chain/colour parameters and counted native access deletion/recovery. Four native
families pass begin/middle/end station circuit/access/Open/save workflows. Final
Release GUI/CLI, focused native/SDL/browsing, CTest 5/5, 14 preservation drivers and 4
original-asset regressions pass after exact Phase 2 workspace recovery and rebuilding.
Forty final production 512×384 PNGs are individually inspected. See
SELECTED-TRACK-EDITING-COMPLETION.md for publication, captures and guarantees.

Construction remains PARTIAL: no atomic rollback/restoration, direct selected-seat
editing, diagonal/special/full-family or positive-transition parity claim. Design
substitution/warnings and arbitrary scenery selection, maze/access ghosts, sustained
family guests, scenery drag/network, shop appearance, hardware/performance and other
existing ordinary matrix gaps remain. Next: native design substitution and
placement-warning recovery; preserve the completed browsing/selected-track workflows.

Selected-track implementation published `f45ef4660eaffeb81061d81766ebf0b5aebdf9fd`, complete tree
`554ddbacce7ed8590bf2516ef1dac9b6a86312e9`, matching local safety `56c0339a8e2c728124e609d48d1effc59d090c6c`.
Capture ZIP: 40 individually inspected production 512×384 PNGs; SHA256
`c34bd20e510404fd1125335766187751364d8195f7571c81482481676e93bfd7`. Final documentation publication follows that fixed
validated source. Exact remaining ordinary and hardware limits remain above.

## Design availability inspection Phase 1 — 2026-10-14 UTC

Native file inspection now reports required vehicle loaded/research state, ride
research, compatible loaded/researched vehicle count, station default fallback and
optional scenery resolution, including native all-scenery omission and legacy path
mapping/default policy. Read-only native scenery resolution is shared with actual
placement. Explicit alternatives and review/execute binding follow in Phase 2.
See DESIGN-SUBSTITUTION-COMPLETION.md; broader ordinary gameplay remains PARTIAL.

## Explicit design vehicle/scenery policy Phase 2 — 2026-10-14 UTC

Native file inspection and placement options now offer explicit loaded/researched
native track-compatible vehicle choices. Missing originals never silently substitute.
Cancel-first reviews bind source identity, copied choices/origin, native object and
research/default context, actual quote and money mode through native action Query/
Execute/serialization. Native all-scenery omission/path fallback warnings are visible;
height/rotation/scenery and source bytes preserved. Native/SDL/failure/save roundtrip
and CTest 5/5 pass. DESIGN-SUBSTITUTION-COMPLETION.md records exact limits. Overall
ordinary construction remains PARTIAL; no arbitrary scenery or rollback promise.

## Native design substitution/warning completion — 2026-10-14 UTC

Explicit loaded/researched exact native ride-type/track-group vehicle choices,
optional scenery policy, original-file preservation and bound Cancel-first native
reviews are delivered. Native failed optional scenery subactions now expose actual
query/result omission counts; changed counts, files, descriptors, research and
quotes reject. Native browser → choice → preview → cancel/apply → access/Open →
save/reload and provisional autosave cleanup pass. Twenty-four genuine production
512×384 captures are inspected; see DESIGN-SUBSTITUTION-COMPLETION.md for APIs,
publication/package, test environments and exact limits. Release GUI/CLI, CTest5/5,
15 preservation drivers under their documented contracts, 5 asset-backed construction/
design drivers and native/SDL save/load browsing pass. The extra forced-shell
PathActions paused-shop fixture failure is explicitly retained there.

Overall ordinary gameplay stays PARTIAL: arbitrary scenery/cross-family replacement,
flat/special design adapters, native rollback guarantees, direct selected-seat and
exhaustive geometry/positive variants, maze/access ghosts, continuous scenery/network
review, broader shop appearance, sustained family guests and Brick/AArch64/performance
remain. Next: broader native shop/facility appearance, preserving delivered naming.

Design substitution implementation published `2f364a1213996a5fde66349423e7a0a7f7fbcd38`,
matching local `c167a945291ef9f86d96e2f5bb7398f604c37b5b`, complete identical tree
`832d344cf04c4d2c96f54de7ab1902cecb67faab`. Guarded native parent/blob/tree/ref
verification succeeded. Saved inspected 24-PNG capture ZIP SHA256
`bcf4bb8818213e040ad4bf0b6b152d4bc9a932dfc6726cdb50abd1de834e4d2b`.
Final documentation closeout follows this fixed production source; see the handoff
for that final ref. Exact remaining ordinary gaps and test-environment caveat remain.

## Shop preview lifecycle audit Phase 1 — 2026-10-14 UTC

SHOP-APPEARANCE-COMPLETION.md resolves the extra forced-shell PathActions discrepancy:
actual dummy-window focus loss correctly clears ghost/borrowed ride with unchanged
real map/cash and retained tool selection. The fixture now asserts that cleanup,
while retaining focused/no-shell preview-restoration assertions. Entire driver passes
both documented and forced Pocket environments; Release GUI/CLI and CTest 5/5 pass.
No production focus-loss behavior changed; a real focused display remains unverified.
Broader native shop appearance follows in Phase 2.

### Shop appearance — native controls implemented (Phase 2)

Shop inspection now offers the actual loaded object's native building colour and
random merchandise channels, paged native palette names and Cancel-first native
cost review. Exact allocation/object/channel/appearance guards survive action
serialization and execute-time revalidation. Unsupported objects are explicit;
no vehicle/station/music controls are invented. Naming/prices/common prices and
status/stats/demolition remain. Focused native/SDL tests, same-slot reuse rejection,
held-A cancellation and .park round-trip pass; final preservation/captures follow.
See SHOP-APPEARANCE-COMPLETION.md for capability and preview-discrepancy evidence.

### Native shop appearance and preview lifecycle — final closeout

Actual shop/facility object channels and existing shared naming are delivered.
Native colour/merchandise choices, Cancel-first cost review, allocation/object/
channel/baseline guards, stale recovery and .park persistence pass. The paused
shop discrepancy was reproduced and resolved as native focus/reset/modal fixture
expectations; no production ghost leak/cash defect was demonstrated. Seventeen
final native/SDL drivers, six original-asset regressions and CTest 5/5 pass. The
28 inspected production 512x384 capture ZIP and exact limitations are recorded in
SHOP-APPEARANCE-COMPLETION.md. Native absence of colour support stays explicit.
Maze/access previews, further selected/special geometry/parameters, flat/special
design adapters, continuous scenery/network, file/keyboard/import, sustained
family guests and physical/performance gaps remain. Next: maze/access previews.

Shop appearance final source publication: e7abc11c4aaa82f2085a14102a560f6df7cf498f,
complete tree 16e03999cba4bc6a0aedfa1db56a36a6990ebfd4, local safety
090a60939897385021b372c6f8134301bc271d92. Every blob/tree/native parent and
leased fast-forward ref/readback verified. Subsequent history documentation
changes no validated production/test/capture files. Whole-game parity remains
partial; native maze/access placement previews are the next substantial gap.

### Native maze/access previews — Phase 1

Native missing entrance/exit previews are integrated with the existing supported
station resolver and stages. Opt-in native ghost-only insertion/removal preserves
real station/queue/maze/path state; serialized allocation/object/station/tile/quote
guards and nested provisional cleanup are tested. Release GUI/CLI, focused
native/SDL access tests, documented and forced-shell PathActions, CTest 5/5 pass.
Maze build/fill preview isolation and final asset-backed closeout follow; see
MAZE-ACCESS-PREVIEW-COMPLETION.md. Existing completed capture packages remain.

Maze/access preview Phase 2: isolated native maze build/fill proposals now reuse
MazeSetTrackAction geometry without changing real walls/stations/count/cash. Native
cursor Move, Cancel-first Fill, exact serialized context/quote guards and nested
provisional/save suspension are covered by focused native/SDL tests. See
MAZE-ACCESS-PREVIEW-COMPLETION.md; final broad/capture closeout is pending.

### Native maze/access preview completion — final closeout

Supersedes the incremental Phase 1/2 pending closeout notes above. Native missing
entrance/exit previews and maze build/fill wall proposals now preserve real map,
stations, paths, cash and counts; native action context/quote guards, nested
save/autosave and paused-tick suspension, menu/camera/load/title/exit cleanup and
held-input cancellation are validated. Native Carousel/Maze/Monorail access
fixtures, maze Open/.park round-trip, 18 drivers, six original-asset regressions,
documented/forced PathActions and CTest 5/5 pass. 29 inspected 512x384 PNGs are
packaged. See MAZE-ACCESS-PREVIEW-COMPLETION.md for APIs, exact evidence and ZIP hash.
Move remains native cursor navigation; a fully removed maze cell uses the native
selection marker. Existing access relocation/removal, exhaustive family geometry,
network reviews and physical/display/performance validation remain gaps; no atomic
replacement/rollback or sustained family guest operation is claimed.

## First Brick release preparation

The small scenery repeat/height usability pass retains native placement sessions,
labels repeat placement accurately and starts manual elevation from native preview/
query height. Native repeated placement/accounting/save round-trip and CTest pass.
See BRICK-NEXTUI-TESTING-RELEASE.md. Hardware/package validation is pending; no
existing access relocation or advanced parity work starts in this milestone.

## First Brick playtest picker follow-up — 2026-10-15

- Paths: Select opens native-sprite surface grid; Y opens options. Normal/queue
  and existing legacy adapters are filtered and revalidated; cancel preserves
  selection and connected mode. Native placement and save/reload are tested.
- Scenario selector: all visible rows are controller-inspectable, including
  locked entries; locked Start remains guarded. X exposes the actual native
  progression setting. Native controller previews/minimaps and lock icons added.
- The broader native-sprite presentation pass remains open. See
  [FIRST-PLAYTEST-PICKERS-COMPLETION.md](FIRST-PLAYTEST-PICKERS-COMPLETION.md)
  for exact controls, fixture distinctions, target validation and hardware limits.

## Slate UI / pause follow-up — 2026-10-15

Explicit Resume clears only native normal pause, including after native reload;
menu-owned cleanup preserves other pause reasons. Shared slate palette, compact
controller prompts, native tree/folder icons, Build/Paths focus return and root
Build back-to-park are delivered. Final native component fixtures were inspected;
physical Brick and exhaustive specialised-screen polish remain unverified/open.
See [SLATE-UI-PAUSE-COMPLETION.md](SLATE-UI-PAUSE-COMPLETION.md).

## 2026-10-15 Brick feedback: meaningful icons and access discovery

See [ICONS-ACCESS-SHORTCUT-COMPLETION.md](ICONS-ACCESS-SHORTCUT-COMPLETION.md).
Generic fallback folders removed; clear-area categories use native G2 family
sprites and numeric settings plain text. Missing entrance/exit placement is the
first eligible closed-ride action in both quick management and Dashboard inspection,
and directly reachable from Access / paths. Existing native placement/ghost/cost
rules remain; existing-access relocation/removal remains outside this milestone.
