# Existing ride-access relocation and removal — 2026-10-16

## Integrity and scope

Continues canonical 58f31ae04ad0c5d1e12958b744ab0e7781528dbf, complete tree
0e42e4d543de1b99773a738e735b17c5bf4ec2dd. The checkout disappeared before this
work; the canonical ref still named that checkpoint. The retained update archive
was materialized and its outer/nested ZIP CRCs and SHA256 checked. Source was
recovered into an absent checkout, not cloned or reset. Initial staging omitted
ignored-but-tracked .vscode/docs files; explicitly staging those recovered files
produced exact complete-tree equality. Recovery safety commit:
28fdf65051731be0cb8ce6548fa05f418c8601cd. Protected Esperanto bytes were then
reapplied unchanged at SHA256 e37ba559b4d9d761e74f2bb284d1ec762b813e789655c0495a882c83651890b5
and excluded from milestone commits. No other surviving checkout edits existed.

This is access editing, not a broader UI pass, advanced construction or design
parity milestone. Prior missing-access shortcuts, slate UI, slots/autosave,
atomic publication, file/design browsing and all prior construction remain.

## Native audit and execution guarantees

Desktop RideConstruction::ToolDownEntranceExit uses RideEntranceExitPlaceAction
for moving existing access. That action queries removal and destination first,
but its execution removes the original source before applying destination
clearance and inserting the replacement. It clears native construction/peep
state; removal invalidates testing, updates queue chains and path edges, restores
maze hedges and clears the selected station reference. Placement establishes the
new reference and native maze/path/queue connections. These are native effects,
not UI simulation, direct live-state repair or an invented transaction.

The existing FlatRideEntrancePlaceAction gains explicit serialized existing-edit
and removal modes. Default missing-access behavior remains unchanged. It reuses
RideEntranceExitRemoveAction or RideEntranceExitPlaceAction through nested native
GameActions. Query and Execute repeat the existing AccessPlacementContext guard,
exact quote and native geometry/status checks. The first source EntranceElement
that native removal would resolve must match the selected real ride, station,
entrance/exit type, base height and direction. Ghost or mismatched sources reject.
Context includes allocation generation, loaded descriptors/versions/checksums,
station/access references and nearby real source/destination/path/maze geometry,
research/default policies, native status and relevant protection/money flags.
No Pocket gameplay state is added to .park saves. Networking remains disabled in
the Brick build; network interoperability/reviews are not validated.

Relocation remains sequential. The test first obtains a valid quote, places a
real native path obstruction at the destination and proves the guarded edit
rejects it without removing the source or changing cash. Calling the underlying
native Execute directly, bypassing that guard/query boundary solely in the test,
demonstrates removal followed by failed construction and a real gap. The existing
native missing-access workflow rebuilds it. No atomicity, rollback or automatic
restoration is claimed; late native/script-hook failure can still leave a gap.

## Controller workflow and supported contexts

Entrances / exits is near the top of both quick ride management and Dashboard
inspection. For rides already missing access, Place entrance / exit remains first
and the existing-access list is second. The list enumerates actual station indices
and existing entrance/exit references; it retains list selection on return.

Choose a point to inspect its native tile, base Z and direction. Move access
starts the established FlatRides station-side targeting tool with the original
station/type bound. Its pure native ghost leaves the real source intact. A opens
a Cancel-first immutable review; B/Cancel returns to the relocation cursor.
Removal opens a separate Cancel-first review. Reviews show source, destination
for movement, station/type, native cost and path/queue/maze/testing consequences.
Open rides are not silently closed. Native restrictions and stale sources remain
recoverable errors. Failed reviews retain feedback; return/reselect obtains a
fresh context. Success returns to native access/path management; removal or late
failure exposes existing missing-access recovery.

Supported geometry follows the audited FlatRidePlacement::SupportsStation and
EntranceLocation adapters: supported flat/special footprints, maze cells and
supported conventional station sequences. Height and facing derive from native
station-side geometry; arbitrary free-facing/free-height or off-station placement
is not added. Multi-station enumeration/index binding is implemented, but the new
end-to-end fixtures exercise station 1; exhaustive multi-station/family operation
is not claimed. Incompatible/unsupported families remain explicit.

Native busy guards prevent duplicate Apply. Completion callbacks bind both request
identity and the owning window lifetime, so a closed window cannot update a new
review even when request numbers collide. Native pending actions remain owned by
the engine. Successful native import clears queued edits and allocation identities.

## Preview and lifecycle

AccessPreview retains its opt-in native pure-ghost insertion/removal, with an
explicit relocation flag. FlatRides carries transient source selection and uses
existing tool, focus/menu, camera and pre-unload cleanup. Nested provisional
simulation/save/autosave scopes suspend and revalidate the ghost. Menus remove
previews; cancellation does not delete real access, alter stations, paths, maze
walls/counts or cash. No independent preview geometry or rollback is used.

## Build and validation

Actual Cortex-A53/AArch64 GNU target build, GCC 16.2/glibc 2.26, matching retained
SDK SDL 2.26.1/runtime. Recovered cached CMake commands/dependency records are
used directly because the former checkout/tools were lost. All 160 GUI objects
and two affected native objects (162 total) were rebuilt initially; native archive,
GUI and CLI relinked. Follow-up Menu and exact-source validation changes were
rebuilt before relinking affected drivers. Production source stayed fixed during
compilation. Generated --version still names the older engine recovery ref;
package manifest records the actual canonical source tree and binary hashes.

RideAccessRelocationTests covers entrance AND exit editing separately for native
Carousel, one-tile Maze and three-piece Monorail station fixtures:

- Native source queue and normal exit paths; exact real map/station/cash purity
  during inspection, query, ghost, nested suspension, autosave and Cancel.
- Actual SDL virtual held-A cancellation, no repeated input and Cancel-first focus.
- Serialized guard round-trip, changed object versions/quotes and stale targets.
- Native move/remove, exact quoted accounting, retained other access/maze count,
  detached old path edge, original missing-access rebuild and access-list return.
- Open-ride rejection without implicit closing; native Carousel Open.
- Changed-destination rejection and demonstrated underlying sequential failure.
- Colliding request numbers across destroyed/new review windows; old callback isolation.
- Early failed import preserving real access after Pocket tool cleanup, successful
  native load clearing queued edits, exact .park map/cash round-trip and pre-unload.

Final native/SDL preservation drivers: AccessShortcutTests,
SceneryPathParityTests, ConstructionTests, PathActionsTests,
PresentationLifecycleTests, MazeConstructionTests, RideConstructionTests,
MazeAccessPreviewTests and PauseResumeTests. PathActions documented environment
(unset OPENRCT2_HANDHELD) and forced Pocket are reported separately. Existing
unchanged standalone input/launcher/save/file driver binaries pass CTest 5/5.
Final observed result details are included in the delivered test logs/manifest.

Fixture corrections retain assertions: AccessShortcutTests accounts for the new
management row while checking the same real Close action. PresentationLifecycle
had an old Build-to-pause-menu expectation superseded by the published slate
Build-to-park behavior; it now checks park/HUD return and no menu. Its metadata-only
bare RideAllocateAtIndex fixture left zero station coordinates, which are real
map coordinates rather than null/no-track stations; explicit null initialization
matches its stated no-track fixture. SDL background joystick events are enabled
for the offscreen test window, preserving the actual held-input assertions.

No physical Brick, original private-asset render/regression sweep, sustained
family guest operation, multiplayer or dense-park performance claim. Public object
metadata may emit missing legacy sprite warnings; captures do not pretend those
sprites are available. Headless offscreen window creation emits the known invalid
hardware-renderer warning; the native X8 component renderer is separate evidence.

## Captures, packaging and publication

Five inspected 512x384 production-window component PNGs show actual native access
list, point inspection, move review, removal review and stale native-error feedback.
They use native fixtures/public G2 and intentionally blank park background; these
are not physical screenshots or original-asset park-render evidence. No fabricated
customers, income or operating statistics are used. Completed historical capture
archives are unchanged. Dimension/SHA256/source/binary provenance is packaged.

OpenRCT2-Pocket-Brick-Access-Relocation-Fix.zip is an update for the working NextUI
installation: merge payload CONTENTS into Tools/tg5040/OpenRCT2 Pocket.pak/game,
retain all existing runtime/data/config/assets and .userdata saves. The unchanged
launch.sh belongs in the .pak root. Source, GPL, instructions, native test logs and
actual stripped AArch64 GUI/CLI are included; proprietary assets and private test
profiles/parks are excluded. Archive/source writes use atomic publication, and
outer CRC, every manifest hash and nested source CRC are verified after transfer.

Source publication uses the current canonical parent, every changed blob, complete
source-tree equality, expected_sha and force=false, followed by branch and native
parent/tree readback. Exact published identifiers and matching local safety commit
are recorded in the delivery manifest/final report, avoiding a self-referential
commit hash inside its own source tree.

## Remaining roadmap

Broader ordinary-play UI polish is next. Direct selected-seat/further parameters,
diagonal/special/exhaustive replacement and positive transitions, flat/special saved
design adapters, unsupported substitutions, scenery/network edge cases, native file
location/format/keyboard/late-import limits, sustained individual family operation
and physical/dense-park measurements remain as previously documented.
