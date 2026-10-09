# Park Construction & Scenery completion — incremental checkpoint

Goal: 100% ordinary OpenRCT2 gameplay through a controller-native 512×384 UI.
This checkpoint extends published `18a9aede00bf13efd6635c6161f1ada1d3f33773`.
Completed Ride Management and all existing construction are preserved. The
unrelated Esperanto edit remains untouched and excluded from publication.

## Phase 1: Native scenery categories — delivered foundation, partial full parity

Build → Scenery offers small scenery (including quarter/half/full-tile objects
and trees), large scenery, walls/fences and path banners/signs. Only loaded,
native available/researched objects enter the existing image picker. No object
loading or research changes occur in production. Original Trees shortcut remains.

SceneryTools uses separate SmallSceneryPlaceAction, LargeSceneryPlaceAction,
WallPlaceAction and BannerPlaceAction adapters. Native definitions/actions own
quadrants, rotated multi-tile footprints, edge obstruction, slope/ground height,
path-banner edges and height, ownership, funds, clearance and accounting. Object
rotation, quadrant and explicit height (8-unit increments) are editable. Automatic
height remains native; banners derive a real non-ghost path's base/slope height.
With multiple path levels, explicit height chooses the desired level.

A applies one action; D-pad pans; X rotates the camera; L1/R1 zoom; L2/R2 change
placement height. Y or Select opens compact options: object picker, direction/
edge, quadrant, height, supported primary/secondary/tertiary colours, and Place /
Recolour / Remove mode. Left/Right or A adjusts options. B returns/exits. Recolour
and Remove use actual native sprite hits and copied identities; no map pointer
survives actions. Unsupported colour channels retain the existing values. Native
small/large/wall/banner colour/removal actions own mutation, cost and persistence.

Native placement ghosts are separate from the desktop global scenery ghost.
Ordinary queries enforce ownership/funds/pause before previews. Ghost-only native
removal, nested save suspension, menu/focus/tool/load/shutdown cleanup and queued
real placement are integrated with existing ProvisionalElements. Large-scenery
and banner native ghost removal now exempts only ghost cleanup from changed
ownership restrictions; real removal still obeys ownership/tree restrictions.
Banner ghost lookup explicitly matches the ghost flag, protecting real banners.

Point at a banner/sign and choose Edit sign text. The reusable eight-column
32-character keyboard retains Unicode and supports UTF-8 erase; new entry uses
its existing ASCII keys. Review defaults to Cancel. BannerSetNameAction and
SignSetNameAction own text, accounting and serialization; stale text/target and
repeated drafts are rejected. Naming does not directly mutate native banners.

Validation: Release GUI/CLI; public-metadata native ConstructionTests for all four
categories, research/missing/bounds, query/execute, exact cash/refunds, nested ghosts,
ownership-loss cleanup, supported recolouring, native sign text, stale repeat
rejection and save/reload. Expanded SDL checks category/picker/duplicate-A/options
return alongside the existing lifecycle. Full existing native suite remains enabled.
No original scenery/world sprites or physical Brick behavior are claimed.

Remaining Phase 1 parity: banner no-entry/text-colour/style controls; eyedropper;
bulk scenery placement/removal; special-colour cheat palette; object-filter search
and scenery-group tabs. Ghost recreation currently favours validation correctness
over caching; physical Brick preview performance still needs measurement. Full
ordinary scenery parity is therefore PARTIAL, not inferred from visible menus.

## Phase 2: Area terrain and water — delivered requested core

Build → Terrain / Water adds a rectangular 1–64 × 1–64 tile brush. Y/Select opens
operations, width/height, native corner/edge/whole-tile selection, and loaded
surface/edge choices. Left/Right adjusts, A applies a menu change or one park edit,
B returns/exits. D-pad/camera/zoom and all original single-tile shortcuts remain.
MapRange is validated before native clamping; off-map footprints are unavailable.
Native map selection displays the rectangle and the effective selection type;
water/style operations use the whole area, regardless of terrain corner choice.

LandRaiseAction/LandLowerAction, LandSmoothAction, WaterRaiseAction/WaterLowerAction
and SurfaceSetStyleAction own all shaping, mixed-height selection, smoothing reach,
water clearance, price, ownership and pause rules. Smoothing can affect neighbouring
terrain as native desktop smoothing does. Native area tools may skip unowned tiles;
Pocket does not invent atomicity or claim those tiles were changed. Painting uses
selected loaded styles and centre-tile native colours; null styles stay unchanged.
No landscaping algorithm or map mutation is introduced. Selection has no serialized
state or placement ghost; all existing ghost/tool cleanup applies on entry/exit.

Validation: Release GUI/CLI, native 2×2 mixed-height raising/lowering, native smooth
raise/lower, corners/edges, water raise/lower, surface/edge footprint, exact cash,
funds, all-unowned rejection, invalid bounds and native save/reload; SDL brush
options, duplicate-A resize, Left adjustment and return. Original single-tile tools
remain regression contracts. Native landscaping interference tests remain in the
existing suite. Terrain colour choice controls and a drag-from-two-corners selector
are still future depth; rectangular dimension selection already covers areas.

## Phase 3: Land and construction rights — delivered requested core

Build → Land / Rights provides land/construction-rights inspection and 1–64
rectangular dimensions. A opens native cost/count review, Y/Select opens kind/size
options, B returns/exits. Review defaults to Cancel and reports available, owned
and skipped unavailable tiles. LandBuyRightsAction alone changes ownership,
fences, cash and construction permissions. Exact ownership, scenario prices,
cash/no-money and quoted cost are revalidated; stale/repeated reviews reject.
Only Pocket's owned menu pause is released for review; existing player pause stays.
Native available/unavailable, construction-versus-land, query/cancel/stale/funds,
exact accounting/save-reload and SDL Cancel-first/owned-pause/native confirmation
checks pass. No ownership or availability is invented in production.

## Phase 4: Furniture and connected paths — delivered core, partial full parity

Build → Scenery → Path furniture uses loaded/researched path additions in the
native object picker. Benches, litter bins, lamps, jumping fountains and queue
screens are available where native descriptors permit. FootpathAdditionPlaceAction
and FootpathAdditionRemoveAction own path height, edges, slope/queue restrictions,
occupancy, broken-addition replacement, costs and serialization. Automatic height
chooses a real path; explicit height selects stacked levels. These objects have
native edge orientation rather than a rotation/quadrant setting, and no supported
colour channels. Recolour reports unavailable; Remove uses the actual path hit.
Ghost removal exempts only ghost ownership checks and clears the addition ghost
flag. Ordinary removal still obeys ownership and native zero refund.

Build → Paths → Connected path / Connected queue begins endpoint selection.
A on an existing real path establishes context without construction or charge.
Each subsequent A requests exactly one adjacent native footpath action. Y turns
through four native directions, L2/R2 selects descend/flat/rise, Select opens
options (including Flat, Restart endpoint and surface). X rotates the camera;
L1/R1 zoom; D-pad pans. B ends and removes previews. Successful native callbacks
advance the endpoint; failed/pending actions do not. Source height/slope identity
is checked, with native slope direction/height validation. Flat/rising/descending
and elevated pieces reuse native incoming connection geometry, queue flags,
footpath ghosts, accounting and routing. No edges or simulation flags are patched.
Context persists through the options menu, not through serialization; restart from
a real saved path. Existing ordinary Paths and RideAccess remain intact.

Validation: Release GUI/CLI; native benches/bins/lamps/fountains through availability,
query/execute, funds, ownership-loss ghost removal, nested suspension, native zero
removal refund and save/reload. Connected native start, sequential flat/rise/elevated/
descend, edges, exact cash, pending repeat prevention, suspension and cancellation.
SDL connected choice/held-A handoff/options-return/cleanup, plus existing RideAccess,
path, construction and ride regressions. Queue screens and every expansion addition
are descriptor-backed but not individually fixture-tested. Direct entrance/exit
endpoint selection, single-step connected undo/history, automatic direction hints,
asset-backed junction/queue/live guest routing verification and physical Brick
performance remain gaps; full connected-path parity stays PARTIAL.

## Captures and publication

Genuine native component captures and the required PNG ZIP will be produced after
validated source publication. Only implemented screens will be captured. Public
metadata fixtures will be labelled; no AI world art or fabricated park backdrop.
Phase 1: `1ad489b600a872c6894a953760a0e81698213a6f`.
Phase 2: `6238d33a36bf694cf3729eac4b046115598be1df`.
Phase 3: `42a59570179f561aa097f23fbb905feb451490c9`.
Phase 4 SHA will be recorded by the subsequent capture/documentation checkpoint.
