# Scenery selection and landscaping completion — incremental checkpoint

Goal: 100% ordinary OpenRCT2 gameplay through native controller Pocket at 512×384.
Base canonical `ae9d40723073248ff0c5c41c138e134b52d97ee0`, source tree
`166eb36e43f41106b85548005223f768f51f911c`. The workspace again reverted to Staff
`db96e886864a04bfbc60398493f59a221c68c55d`. Every existing edit/index/untracked
inventory was preserved before recovery. Ninety-one changed published files were
recovered atomically after individual blob verification. Full indexed tree equality
and an otherwise unchanged tracked working tree were verified before compilation.
Recovery safety `57320379e91750ad6b92fe265e1e7b58c8ca0533` exactly matches the
published tree. Esperanto remains unchanged with SHA256
`e37ba559b4d9d761e74f2bb284d1ec762b813e789655c0495a882c83651890b5`, excluded.
No clone/reset/clean/restore/history rewrite or force-push was used.

## Phase 1 — native scenery eyedropper

Scenery options appends Pick placed object, preserving existing indices. A closes
options into pick mode; D-pad pans, X rotates, L1/R1 zoom and Y/Select recovers
options. A over a real eligible native sprite selects its object/settings and
returns to placement; that press cannot also place. B ends the tool. The existing
Mode row also cycles through Pick. Target feedback shows actual hovered object,
or Point at scenery, rather than the previous placement object. HUD/indigo rail/
gold focus and contextual Pick/Place controls remain native production components.

Native desktop scenery eyedropper interaction flags supply small/large/wall/banner/
path-addition sprite hits. Existing Capture/Find copies category, native position,
height, direction/part, object descriptor, colours and banner metadata. Pick
re-resolves and compares the full copied identity, rejects ghosts/ambiguous/stale
identities and requires actual loaded/researched availability. No pointer survives
selection. Native map directions are retained (desktop button rotation is camera
relative). Small quadrants, wall/banner edges, native height and supported colour
channels are copied; unsupported channels retain current settings. Native large
sequence metadata normalizes child height to the placement origin; banner height
undoes the native banner/path offset. Furniture uses real path height/identity.
Group context is retained when it contains the object; otherwise it resets to
All available. Sign text/no-entry/routing are not cloned into a new object.

Selection is a read-only tool handoff, not a placement action. It neither charges
money nor alters the selected element. Placement, recolour/removal, ghosts and
serialization remain owned by existing native GameActions and provisional cleanup.
No additional Pocket gameplay state is serialized. Existing ordinary ambiguity
limits use native copied identities; no new persistent scenery IDs are invented.

Release GUI/CLI build PASS after refreshing the older cached source list to include
the already-published Mazes module. Production source was held fixed while building.
ConstructionTests PASS: all five categories, query-only tool stability, supported/
unsupported colours, unchanged native element/cash/ledger/ghosts, descriptor/research/
removed-target rejection, non-origin large sequence and elevated/rotated targets.
Expanded asset-backed native/SDL SceneryPathParityTests PASS: actual banner sprite
hit at the controller reticle, held/duplicate-A pick without placement, existing
native groups/banner styles/area clearing/cancellation and persistence. Existing
PresentationLifecycleTests PASS; CTest input/launcher/save-preservation 3/3 PASS.
The fixture camera uses native projection and original sprites; production does
not move peeps or invent hit geometry. Fixture compile corrections applied native
zoom separately to x/y and supplied the existing capture helper's explicit mode.

Three actual 512×384 native viewport/component captures were generated and inspected:
landscaping-01-native-eyedropper, landscaping-02-native-eyedropper-placement and
landscaping-03-eyedropper-options. They use original sprites over a native action-built
initialized fixture, actual tool/native HUD state and production drawing components.
They are not whole SDL/hardware frames or sustained gameplay evidence. Incorrect old
object feedback was fixed, rebuilt/retested and regenerated. Bounded ellipsis remains
intentional; no material overlap/focus/footer/HUD defect was found. Final dimension/
SHA256/provenance manifest and ZIP follow the remaining phases. Private archives,
parks and designs are excluded. No AArch64 or physical Brick result is claimed.

## Remaining requested phases

Native object search, shared native scatter/bulk placement and native surface/edge
colour controls remain pending. Delivered styles/groups/clearing/furniture/access/
undo, finance/research and guest history/follow/filter/map workflows are preserved.
All broader ordinary construction/design/save/map gaps stay on the feature matrix.
Canonical phase SHA/tree will be recorded after guarded expected_sha/force=false
publication with exact local/canonical source-tree equality.
