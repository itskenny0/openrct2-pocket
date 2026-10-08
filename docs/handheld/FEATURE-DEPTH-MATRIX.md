# Desktop versus Pocket feature depth

Living roadmap, source audit 2026-10-08. Baseline: published Pocket b21dd2d.
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
| Ride construction | PARTIAL | FlatRides.cpp and Designs.cpp: eligible supported non-track flats and wooden saved designs, native previews/actions, entrance/exit recovery. |
| Custom track | PARTIAL | CustomTracks.cpp: wooden forward-endpoint editor, ordinary curves/slopes/banks/chain/brakes, native track actions, undo and circuit/access recovery. |
| Shops / facilities | COMPLETE for placement/basic management; PARTIAL domain | Shops.cpp, ShopManagement.cpp: eligible native objects, rotation, ghost, status, product/common prices, statistics and demolition. |
| Paths | PARTIAL | Paths.cpp: normal/queue surfaces, raised/sloped tiles, options, ghost, removal; RideAccess.cpp: elevated station landings and genuine connectivity. |
| Terrain / water | PARTIAL | Terrain.h, Water.h, Surfaces.h: single-tile height/water/style actions with native cost/validation. |
| Scenery | PARTIAL | Trees.cpp: loaded researched full-tile trees/picker/ghost; SceneryRemoval.h: one targeted small element removal. |
| Guests | COMPLETE for individual read-only inspection; PARTIAL domain | GuestManagement.cpp: summary, full roster, status/needs/thoughts/spending/inventory and locate, entity reuse protection. |
| Staff | PARTIAL | StaffManagement.cpp: handyman/mechanic full roster, automatic hire, orders, details/locate, dismissal. |
| Finances | COMPLETE for loan/admission; PARTIAL domain | Finances.cpp: native cash/loan/income/expenditure/profit, bounded loan/admission confirmation. |
| Research | COMPLETE for ordinary funding/priorities/progression | ResearchManagement.cpp and ParkSetResearchFundingAction: native current project, estimate, funding/categories, research-filtered Build objects. |
| Marketing | COMPLETE for starting/reviewing native campaigns | MarketingManagement.cpp, ParkMarketingAction: all eligible campaign/target/duration drafts, native costs/progression; no invented campaign cancellation. |
| Park/scenario management | PARTIAL | ParkOverview.cpp, Menu.cpp: objectives/status and native save/load; desktop Park.cpp/Map.cpp offer land/construction rights and broader information. |
| Save / load | COMPLETE for Pocket slots; PARTIAL domain | Menu.cpp native .park saves/loads, overwrite/discard review, autosave/provisional cleanup; desktop LoadSave.cpp has arbitrary file browsing. |
| Global information | PARTIAL | Hud.h native cash/date/guests/pause/speed; Dashboard sections provide selected native summaries. |

## Missing depth and recommended treatment

Each row is a meaningful gap, with native authority and existing Pocket foundation.
Complexity refers to implementation, not the number of desktop controls.

| Gap / classification | Desktop capability and native source | Pocket foundation | Complexity / handheld value | Presentation / dependencies / risks |
| --- | --- | --- | --- | --- |
| Ride operations PARTIAL | Ride.cpp operating page; RideSetSettingAction; Ride::getAvailableModes, type flags, LiftData | Existing ride identity, drafts, confirmation, Dashboard | medium / high | Operations list, one bounded native setting draft at a time; mode changes can reset test/vehicle state, waits couple minimum/maximum. First depth package. |
| Ride maintenance PARTIAL | Ride.cpp maintenance; Ride inspectionInterval, lastInspection, downtime, dueInspection, availableBreakdowns | Existing stats and native setting action | small / high | Maintenance page with interval draft; distinguish rides without breakdowns. |
| Ride demolition MISSING | DemolishRidePrompt.cpp; RideDemolishAction; RideGetRefundPrice uses native track query calculations | Shop confirmation pattern and ride stale identity | medium / high | Named destructive Cancel-first review; action Query omits refund, use native prompt calculator; changed layout/quote must invalidate review. |
| Vehicles PARTIAL | Ride.cpp vehicle page; RideSetVehicleAction; Ride::updateMaxVehicles, loaded vehicle object limits, proposedNumTrains/proposedNumCarsPerTrain | Ride drafts/identity available | medium / high | Train/car counts and requested/effective review delivered in Vehicles/Dispatch checkpoint. Native station/object limits, closure and clamping apply. Vehicle-object selection and reverse trains remain deferred. |
| Mode-specific options PARTIAL | Ride.cpp launch/time/laps/swings/rotations/circuits; load mask/synchronization now represented; RideSetSettingAction | Operations setting model | medium / high | Typed labels/units and descriptor visibility; do not show raw operationOption as a universal setting. |
| Ride finance/customer depth PARTIAL | Ride.cpp income/customer pages; Ride totalProfit, popularity, satisfaction, queue/station information | Existing read-only stats | small / medium | Split Finance/Customers when enough useful content; respect undefined values and rolling windows. |
| Ride music/appearance/naming MISSING | Ride.cpp music/colour/name; RideSetSettingAction, RideSetAppearanceAction, RideSetNameAction | Picker/draft architecture | medium / medium | Native music objects and colour choices; keyboard dependency for naming. Defer in first operations package. |
| Ride graphs DEFERRED | Ride.cpp graph page; RideMeasurement sampling and scrolling | No controller graph view | medium / low | Optional fixed-size native graph later; unavailable sampling honest, cursor/time scale require additional interaction. |
| Other ride types / special construction MISSING | NewRide.cpp, RideConstruction.cpp; RideCreateAction, TrackPlaceAction, MazeSetTrackAction, TrackDesignAction | Flat/custom/saved adapters | architectural / high | Extend one complete category at a time; special geometry, vehicles and station rules differ. |
| Custom editing breadth PARTIAL | RideConstruction.cpp backward/selected-block editing, special tracks, brake speeds, design export | Forward native wooden editor | architectural / medium | Endpoint selection and recovery before other coaster types; track metadata and native removal remain authoritative. |
| Shop appearance/naming PARTIAL | Ride.cpp colours/music/name and item stats; appearance/name actions | Shop management/picker | medium / medium | Shared ride-depth framework where semantic settings apply; no generic coaster options for shops. |
| Connected path builder PARTIAL | Footpath.cpp bridge/tunnel next-endpoint building; FootpathPlaceAction | Elevated single-tile tool and geometry | medium / high | Recoverable endpoint route context; native slopes/heights/clearance, never automatic paths. |
| Path furniture MISSING | Footpath.cpp additions; FootpathAdditionPlaceAction/RemoveAction | Shared object picker/targeting | medium / medium | Loaded eligible lamps/bins/benches picker, native path hit testing and maintenance rules. |
| Terrain brush/edges MISSING | Land.cpp, Water.cpp, LandRights.cpp; LandRaise/Lower/SmoothAction, SurfaceSetStyleAction, LandSetRightsAction | Existing one-tile tools | medium / high | Bounded brush options and native selection rectangle; ownership and mixed-height pricing tests. |
| Other scenery MISSING | Scenery.cpp; small/large/wall/banner placement, colour/removal actions | Reusable picker and tree preview adapter | medium to architectural / high | One category workflow at a time; footprints, elevation, rotation and hit testing differ. |
| Guest grouping/filtering DEFERRED | GuestList.cpp thought/action groups and filters | Full roster and native details | medium / medium | Filter submenu, counts and stable selection; live membership/stale entities. |
| Guest pickup DEFERRED | Guest.cpp; PeepPickupAction | Inspect/locate | medium / low | Target placement and safe cancellation; not needed for ordinary management. |
| Staff patrol/roles MISSING | Staff.cpp, StaffList.cpp; StaffSetPatrolAreaAction, StaffSetCostumeAction, StaffHireNewAction | Handyman/mechanic roster/order framework | medium / high | Patrol tile selection first; security/entertainer role visibility and costume picker later. |
| Finance history/graphs DEFERRED | Finances.cpp expenditure tables and historical graphs; Park expenditureTable and history | Finance panel | medium / medium | Paged native period/category table before graphs; no fabricated history. |
| Research invention list DEFERRED | Research.cpp finished-item list, Research state | Funding/priorities complete | small / medium | Read-only paged unlock history; no manual unlocking/editor ordering. |
| Land/construction rights MISSING | Map.cpp, LandRights.cpp; LandBuyRightsAction/LandSetRightsAction | Native tile selection/action feedback | medium / high | Rights tool with queried native purchase cost and permitted rights; scenario restrictions. |
| Park news/awards/history PARTIAL | Park.cpp, RecentNews.cpp, NewsItem and park histories | Overview, management panes | medium / medium | Concise news/history pages; scrolling, locate links and unavailable statistics. |
| Arbitrary save/design browsing PARTIAL | LoadSave.cpp and TrackDesignList.cpp; native platform environment | Slot UI and saved-design picker | medium / medium | Controller-native folder/file navigation; keyboard needed only for custom names. |
| General map/information MISSING | Map.cpp, Park.cpp, GuestList.cpp | Locate/navigation and Dashboard | medium / medium | Native park map and task-specific legends; no overlapping desktop windows. |
| Desktop-only pointer affordances DESKTOP-ONLY | Resizable overlapping windows, hover tooltips, drag widgets | Semantic controller architecture | architectural / low | Replace with bounded pages/context details; literal desktop arrangement inappropriate at 512×384. |

## Order and maintenance

First: Ride Details / Operations / Maintenance / demolition. Next: vehicle and
mode-specific dispatch depth as one complete ride configuration workflow, unless
hardware feedback identifies a blocker. Then terrain/rights or path furniture,
followed by scenery categories and staff patrol. Keyboard and arbitrary browsing
are cross-cutting completeness tasks, not reasons to redesign the shell.
Update this document in each depth checkpoint. Treat COMPLETE tasks as regression
contracts. Public component captures do not prove asset-backed coaster operation.

## First depth package delivered

RideManagement.cpp now supplies descriptor-filtered mode, wait limits/toggles and
lift-speed drafts, maintenance interval and native information, total queue length
and confirmed native demolition. These named tasks are COMPLETE within their
current supported native setting scope; Rides remains PARTIAL for vehicle, full
dispatch/mode-specific options, appearance/music, refurbishment and graphs. See
RIDE-DEPTH-CHECKPOINT.md for exact tests and missing asset-backed validation.
Next recommended depth package is vehicle/dispatch configuration, not a new UI
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
Counts/dispatch are COMPLETE within this named scope; Vehicles remains PARTIAL
for vehicle-object selection and reversed trains. Mode-specific options still
need typed time/laps/launch/rotations/circuits support. No new construction, UI
shell or save format. See VEHICLES-DISPATCH-CHECKPOINT.md for validation and
public-fixture limitations.

Recommended next package: descriptor-driven mode-specific operating parameters
within Operations, preserving these new vehicle/dispatch workflows. Keep vehicle
object selection, appearance/music and graphs deferred until that package is
validated.
