# Native track editing and design authoring checkpoint

The goal remains 100% ordinary OpenRCT2 gameplay on the 512×384 TrimUI Brick.
This milestone is incremental; overall construction remains PARTIAL.

## Phase 1 — selected conventional section continuation

The existing track-block roster now offers **Continue section...**. Select any
real sequence-zero block of a closed, researched, compatible conventional ride.
Native `trackBlockGetNext` and `TrackIsConnectedByShape` follow that component to
its forward gap, including disconnected components without a station. Native TED
sequence-zero clearance translates the selected origin to its actual element.
No independent track geometry, map writes or serialized Pocket markers are used.

The normal forward editor retains queried costs, native placement/clearance,
removal/refunds, circuit detection and entrance/exit recovery. Selection validates
ride identity and the block's type, orientation, height, chain, inverted and speed
baselines before menu handoff. The transient anchor uses coordinates/type, never
retained element pointers. A missing anchor ends editing instead of jumping to an
unrelated station component. Undo of an extension retains context; removal of the
anchor itself ends it safely. Menu return preserves the active editor. Load/exit
uses existing tool teardown; players reselect a real block after load.

Controls: Dashboard → Rides → Inspect → Edit track blocks → select block →
Continue section. Existing A build, B endpoint undo, Select piece/camera picker,
L1/R1 piece, L2/R2 slope/height, X bank and Y chain/orientation remain unchanged.
Existing selected removal remains Cancel-first. A continuation is not destructive.

Validation: Release GUI/CLI build; RideConstructionTests five-family native
query/execute, disconnected continuation, extension/undo, missing anchor cleanup,
stale identity, costs/refunds and save/reload. Factory Capers Junior asset-backed
RideConstructionCapture exercises real menu/controller handoff, held-A Cancel,
existing speed/removal/access/design and persistence. Captures use native X8
viewport plus production Pocket components, rather than whole SDL gameplay frames.

Remaining: backward construction, replacement, broader selected parameters and
variants, player-facing design export/scenery selection, manual design elevation,
substitution choices and full special-family operation are not yet delivered by
this phase. The native block picker remains a roster; spatial selection is not
claimed. AArch64 and physical Brick are untested.

## Publication

Base canonical: 57d36683439bb192c6210c95fb5aa441cbbd6797.
Base local: 20542113fba2b5066a4ed3a1900c327b1aae4ba4.
Base source tree: 61bbf89aef9c6642c231577e4b416e86fa115977.
Canonical/local phase SHAs will be recorded after guarded publication.
Esperanto remains a separate preserved local edit.
