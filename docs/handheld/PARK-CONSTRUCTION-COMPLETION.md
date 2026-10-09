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

## Remaining planned phases

2. Area terrain/water/paint, brushes and native smoothing/edges.
3. Native land and construction-rights inspection and Cancel-first purchasing.
4. Path furniture and sequential connected ground/elevated construction.

## Captures and publication

Genuine native component captures and the required PNG ZIP will be produced after
validated source publication. Only implemented screens will be captured. Public
metadata fixtures will be labelled; no AI world art or fabricated park backdrop.
Exact validated pushed commits are recorded in a subsequent documentation checkpoint.
