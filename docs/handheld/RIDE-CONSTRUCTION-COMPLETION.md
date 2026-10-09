# Ride Construction Breadth and Track Editing Completion

Goal: 100% ordinary OpenRCT2 gameplay in controller-native Pocket at 512×384.
Continues canonical `1818fb28daa62309710880a7e9c353ddbf6915ed`; local safety
history and equivalent canonical source trees are preserved. Esperanto is untouched.

## Phase 1 — conventional native track families

Build → Rides lists loaded/researched object/type pairs with native type names,
including conventional coasters and transport/water tracked rides whose descriptors
have regular end-station geometry. Each explicit picker identity retains the native
ride type; multiple native types on one object do not silently choose another type.
Existing Wooden picker keys, flat rides and construction shortcuts remain valid.

TrackConstructionSupport shares descriptor/entry/research eligibility between UI
and native sequential action. Maze and single-piece flat footprints stay separate.
Regular station recovery/entrances and RideAccess now accept these descriptor-backed
families. Native station, vehicle support, funds, height, ownership, clearance and
accounting remain authoritative. No objects are loaded or research unlocked in play.

The piece picker now admits all native TED groups supported by the selected loaded
vehicle and ride descriptor, with matching incoming pitch, roll and diagonal shape.
Native inversion, helix, photo and other conventional special-piece metadata replace
the former non-inverting whitelist. Maze/flat footprint types are explicitly rejected
by the UI and guarded sequential action. TrackPlaceAction/TrackRemoveAction still own
geometry and cost; Pocket supplies no independent shaping algorithm or track route.

Controls retain A Build; B native endpoint Undo; L1/R1 compatible piece; L2/R2 slope
or station height; X bank; Y chain/station turn; Select paged pieces/Return to park;
L3 camera mode then L1/R1 zoom and X camera rotation. Start preserves menu recovery.
Extended native pitch/roll pieces remain selectable through the paginated picker.

Phase 1 validation: Release GUI/CLI; new public-metadata RideConstructionTests builds
Wooden, Looping, Corkscrew, Twister and Monorail through native actions. Stations,
straights, multi-tile turns, compatible vertical-loop approach/inversion, query,
pending-repeat rejection, exact costs/refunds, unsupported maze geometry, ghost
cleanup, resume and save/reload pass. Full asset-backed native/SDL PathActionsTests
passes. Original asset-backed CustomCoasterCapture explicitly selects Wooden from
the broader picker, builds/opens its circuit/access and proves natural guest use,
accounting and save/reload without any guest teleportation or state patches.

Phase 1 published: `fd35c9b06a80cdebf057e18a9bc078b6a6621022`.

## Phase 2 — native saved designs and scenery

Saved designs now enumerate native repository files for every compatible conventional
ride family, with exact loaded/researched vehicle/type eligibility. No vehicle
substitution is invented. Maze and single-piece flat designs remain separate gaps.
Native import, thumbnail rendering, TrackDesignAction, native design placement,
orientation, height and station/access recovery remain authoritative.
L2 excludes design scenery; R2 includes it. Native scenery availability is probed
through TrackDesignPlaceVirtual; unavailable scenery is explicitly reported and
omitted according to native desktop fallback. A opens a Cancel-first exact-cost
review; scenery cannot change within that review. Ghost removal includes native
track and scenery. No new serialized park state is introduced.

Five-family tests export actual action-built rides through native T6Exporter,
import them, query scenery off/on, verify extra native cost and real scenery
placement, prevent duplicate confirmation and check ghost cleanup and save/reload.
The original asset-backed Wooden design regression explicitly selects its exported
fixture from the expanded list; picker identity assertions accept the real list size.
Phase 3 editing remains pending. Other conventional
families are offered by native descriptors but are not individually exhaustively
validated. Covered/inverted-track toggle variants, special parameter controls and
maze/special footprint adapters remain ordinary parity gaps. Physical Brick and
AArch64 performance/controls are not claimed.
