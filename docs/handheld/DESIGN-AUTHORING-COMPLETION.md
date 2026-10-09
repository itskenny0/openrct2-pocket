# Native design authoring and advanced editing — incremental checkpoint

Goal: 100% ordinary OpenRCT2 gameplay on controller-native 512×384 TrimUI Brick.
Construction remains PARTIAL. No missing ordinary feature is removed from the roadmap.

## Phase 1 — player-facing native export

Dashboard → Rides → Inspect → Save design opens the existing UTF-8-aware 32-code-point
keyboard. OK creates an explicit Cancel-first export/overwrite review. A confirms
only the selected choice; B returns to naming/inspection without writing. Existing
held-input handoff consumes one action per transition. Full-width review text shows
the destination basename. Successful export returns to inspection with filename feedback.

Eligibility preserves desktop Ride::saveToTrackDesign: tested flag AND native ratings
are required; native design creation must succeed. No incomplete-ride shortcut exists.
Ride identity retains generation, type, object, build date and location. Each review
and write recreates the native design, compares actual native T6Exporter bytes,
status and source name, and rejects changed/departed/reused sources. The payload
includes native geometry, vehicles, operating settings, appearance and statistics.

Names retain existing UTF-8 and delete whole code points through TextEntry. Empty,
long, control/path-symbol and trailing-dot/space names reject. Native user track
folder and native TD6/TD7 extension are used. Symlink/nonregular destinations reject.
Overwrite requires a fresh review with the existing file bytes unchanged. Export
writes a unique temporary file, revalidates source/destination, then renames it.
Native TrackRepositoryScan refreshes the picker. Repeat confirmation rejects; menu
close and existing load/exit tool teardown clear all transient authoring context.
No simulation/accounting state is changed and no Pocket gameplay state is serialized.

Validation: Release GUI/CLI build PASS; RideConstructionTests five-family native
query/execute/ghost/cost/refund/identity/manual-height/save regression PASS; CTest
input/launcher/save preservation 3/3 PASS. Expanded asset-backed DesignCapture PASS:
native testing produces tested status and ratings, then real export/import round-trip,
UTF-8/unsafe names, repository refresh, Cancel-first/held-A controller workflow,
repeated execute, overwrite cancellation, stale destination, native appearance-action
source change, reused identity, menu cleanup and exact unchanged cash. The existing
design placement/research/access/testing/save-reload suite also passes. No live
ratings, ownership, cash or research is patched to obtain export eligibility.

## Source preservation and publication

Restored local checkout was db96e886864a04bfbc60398493f59a221c68c55d (staff checkpoint).
49 unmodified tracked files were updated from canonical 0ff46584fe0d3689c081ad2b28c0952bba70d23a
through authenticated GitHub blob reads; each blob and the full staged tree matched.
Local synchronization commit 7ab997b preserves prior local history; its tree is
exactly e84ead2956618c94d5526db887ae5254613bfe99. Esperanto remains untouched and excluded.
Phase publications use canonical native parents, exact source-tree equality and
expected_sha/force=false guarded fast-forward updates. Phase SHA table follows.

## Current limitations / next phases

Export currently has no scenery selection. Manual individual/area scenery selection,
substitution choices, covered/inverted/alternative variants, seat rotation and other
special parameters, backward construction and selected-block replacement remain
ordinary gaps. Native maze/conventional placement, manual elevation, disconnected
forward continuation and selected speed/removal workflows are preserved.
Asset-backed export uses the privately supplied African Diamond Mine scenario,
Data/ObjData/Tracks. Captures are production native X8 viewport plus Pocket component
renders, not complete SDL gameplay frames or physical Brick captures. AArch64,
physical Brick, sustained routing and per-family operation remain unverified.
Private archives, parks and generated designs are excluded from publication/delivery.
