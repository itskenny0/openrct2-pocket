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
| Paths | PARTIAL | Paths.cpp: normal/queue surfaces, raised/sloped tiles, options, ghost, removal; RideAccess.cpp: elevated station landings and genuine connectivity. |
| Terrain / water | PARTIAL | Terrain.h, Water.h, Surface.h / AreaTools.h: original single-tile actions plus rectangular brush, native smoothing, corners/edges, water and loaded area surface/edge styles. |
| Scenery | PARTIAL full parity; four native categories delivered | SceneryTools.cpp: small/large/wall/banner picker, native geometry, heights, ghosts, colour/removal and sign text; original Trees retained. See PARK-CONSTRUCTION-COMPLETION.md. |
| Guests | COMPLETE for individual read-only inspection; PARTIAL domain | GuestManagement.cpp: summary, full roster, native action/thought groups and tracked filter, status/needs/thoughts/spending/inventory/preferences, naming/tracking and shared native pickup/placement; identity reuse protection. Ride history, persistent follow and broader filter/map entry points remain gaps; see GUEST-PLACEMENT-COMPLETION.md. |
| Staff | PARTIAL | StaffManagement.cpp: all four native roles, automatic hire, role orders/counters, locate/dismissal and loaded entertainer costumes. Native patrol area editing/clear is delivered; naming, role filtering/counts/wages and native role uniforms are delivered; shared pickup/manual hiring delivered; see STAFF-MANAGEMENT-COMPLETION.md. |
| Finances | COMPLETE for loan/admission; PARTIAL domain | Finances.cpp: native cash/loan/income/expenditure/profit, bounded loan/admission confirmation. |
| Research | COMPLETE for ordinary funding/priorities/progression | ResearchManagement.cpp and ParkSetResearchFundingAction: native current project, estimate, funding/categories, research-filtered Build objects. |
| Marketing | COMPLETE for starting/reviewing native campaigns | MarketingManagement.cpp, ParkMarketingAction: all eligible campaign/target/duration drafts, native costs/progression; no invented campaign cancellation. |
| Park/scenario management | PARTIAL | ParkOverview.cpp, Menu.cpp: objectives/status and native save/load; RightsTools.h adds native land/construction-rights purchase/inspection; broader desktop Park/Map information remains. |
| Save / load | COMPLETE for Pocket slots; PARTIAL domain | Menu.cpp native .park saves/loads, overwrite/discard review, autosave/provisional cleanup; desktop LoadSave.cpp has arbitrary file browsing. |
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
| Custom editing breadth PARTIAL | RideConstruction.cpp backward/selected-block editing, special tracks, brake speeds, design export | Descriptor-driven forward editor and selected native block removal/speed controls | architectural / high | Backward construction, disconnected endpoint choice, replacing blocks, alternative track and further parameters remain. See RIDE-CONSTRUCTION-COMPLETION.md. |
| Shop appearance/naming PARTIAL | Ride.cpp colours/music/name and item stats; appearance/name actions | Shop management/picker | medium / medium | Shared reusable naming keyboard delivered; broader shop appearance remains outside this ride milestone. |
| Connected path builder PARTIAL | Footpath.cpp bridge/tunnel next-endpoint building; FootpathPlaceAction | Existing-path endpoint selection, sequential flat/rise/descend/elevated normal/queue placement | medium / high | Native incoming geometry/ghosts/costs, pending guard and options recovery. Entrance endpoint start, undo/history and asset-backed routing validation remain. See PARK-CONSTRUCTION-COMPLETION.md. |
| Path furniture COMPLETE native placement/removal core; PARTIAL exhaustive validation | Footpath.cpp additions; FootpathAdditionPlaceAction/RemoveAction | Shared object picker/targeting | medium / medium | Delivered loaded/researched additions picker, native path geometry, placement/removal/ghosts and persistence. Named bench/bin/lamp/fountain/queue-screen fixtures, queue/slope restrictions and persistence pass; expansion exhaustive validation pending. |
| Terrain brush/edges COMPLETE requested core; PARTIAL broader depth | Land.cpp, Water.cpp, LandRights.cpp; LandRaise/Lower/SmoothAction, SurfaceSetStyleAction, LandSetRightsAction | Existing one-tile tools | medium / high | Rectangular 1–64 brushes, native corner/edge selection, smoothing, area water and surface/edge styles delivered. Colour-choice controls remain; see PARK-CONSTRUCTION-COMPLETION.md. |
| Other scenery PARTIAL | Scenery.cpp; small/large/wall/banner placement, colour/removal actions | Reusable picker and tree preview adapter | medium to architectural / high | Four native categories now have placement/ghosts, rotation/elevation, recolouring, removal and sign text; banner styles, bulk tools, filtering and remaining parity are listed in PARK-CONSTRUCTION-COMPLETION.md. |
| Guest grouping/filtering COMPLETE native action/thought core | GuestList.cpp thought/action groups and filters | Full roster and native details | medium / medium | Native action/thought argument groups, counts, tracked filter and identity-stable filtered rosters delivered; see GUEST-PLACEMENT-COMPLETION.md. |
| Guest pickup COMPLETE named core; guest domain PARTIAL | Guest.cpp; PeepPickupAction | Inspect/locate | medium / low | Shared native guest/staff placement, native cancel restoration, menu recovery and manual hiring delivered. Ride history/follow/broader GuestList filter entry points remain ordinary parity gaps. |
| Staff patrol/roles COMPLETE named core; remaining staff depth PARTIAL | Staff.cpp, StaffList.cpp; StaffSetPatrolAreaAction, StaffSetCostumeAction, StaffHireNewAction | Handyman/mechanic roster/order framework | medium / high | All roles/costumes and native patrol area Add/Remove/Cancel-first Clear delivered. Naming, filtering, counts/wages and uniforms delivered; shared pickup/manual hiring delivered; see STAFF-MANAGEMENT-COMPLETION.md. |
| Finance history/graphs DEFERRED | Finances.cpp expenditure tables and historical graphs; Park expenditureTable and history | Finance panel | medium / medium | Paged native period/category table before graphs; no fabricated history. |
| Research invention list DEFERRED | Research.cpp finished-item list, Research state | Funding/priorities complete | small / medium | Read-only paged unlock history; no manual unlocking/editor ordering. |
| Land/construction rights COMPLETE requested purchase/inspection scope | Map.cpp, LandRights.cpp; LandBuyRightsAction/LandSetRightsAction | Native tile selection/action feedback | medium / high | RightsTools.h: native area inspection and Cancel-first LandBuyRightsAction purchase review, counts, scenario availability, cost/ownership snapshots, stale guards and save/reload; no direct ownership mutation. |
| Park news/awards/history PARTIAL | Park.cpp, RecentNews.cpp, NewsItem and park histories | Overview, management panes | medium / medium | Concise news/history pages; scrolling, locate links and unavailable statistics. |
| Arbitrary save/design browsing PARTIAL | LoadSave.cpp and TrackDesignList.cpp; native platform environment | Slot UI and saved-design picker | medium / medium | Controller-native folder/file navigation; keyboard needed only for custom names. |
| General map/information MISSING | Map.cpp, Park.cpp, GuestList.cpp | Locate/navigation and Dashboard | medium / medium | Native park map and task-specific legends; no overlapping desktop windows. |
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
