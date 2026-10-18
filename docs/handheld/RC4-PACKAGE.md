# RC4 NextUI candidate — package and verification

Published implementation: `08b255f1d1fcc984abf8268e15150449d3e1a2c1` on
`handheld/modern-ui-paths`, based on the latest canonical path-investigation
checkpoint `a850b1aa78bd0652a679c4ad19656109a574b716`. Existing canonical history,
source, captures and unrelated backlog remain intact.

Archive: **OpenRCT2-Pocket-Brick-RC4-20261019.zip** (101,071,302 bytes).

SHA-256: `15881e4095f970b2dfd62f1a8626a3355b137794d4ec17f9b6d6192d67aa6104`

This is the requested non-path continuation. The original RC4 all-priorities
acceptance criteria are not claimed: the user deferred the Line/Fill defect.
That reported device bug remains unresolved and is documented in the backlog.

## Delivered changes

- Park-selected guests open the same modern Overview / Needs / Thoughts / Details
  workspace. The exact native guest, live state and park camera are preserved;
  B returns to park. Dashboard guests retain roster position. Follow, tracking,
  pickup/placement and native staff/ride inspection remain available.
- Coaster ghosts now survive full native update/provisional cycles and are
  restored after native save operations. Native placement geometry, financial
  accounting and restrictions remain authoritative. Saving with a live preview
  excludes both the ghost and its temporary native ride record on reload.
- Track catalogs use actual native track rendering, enlarged selected thumbnails,
  wrapped names and compatible groups. Active construction stays continuous,
  with a larger thumbnail, live ghost and native cost/error feedback.
- Edit track opens spatial selection with the actual piece highlighted in the
  park. A opens plain-language piece actions. Native connected traversal,
  removal/replacement/continue/backward/station/speed functions remain supported.
- Native news appears as a compact ribbon during ordinary play, with target-aware
  ride, guest/staff, invention, finance, marketing, park, guest-category and award
  handoffs. Informational/stale references fail safely. Dismissal uses native
  history; Back preserves the appropriate caller.

## Controls

| Context | Control | Action |
|---|---|---|
| Park | Y | Current native news/detail or history |
| Dashboard section rail | X | Native news history |
| Track catalog | Y | Next nonempty compatible group |
| Active track building | L1/R1 | Previous/next compatible piece |
| Station origin | L2/R2 | Native 16-unit height step |
| Built-track selector | L1/R1 or horizontal D-pad | Previous/next connected piece |
| Built-track selector | Vertical D-pad | Browse all pieces, including disconnected sections |
| Built-track selector | A / B | Piece actions / ride management |
| Piece actions | B | Selected piece, preserving selection |

Existing advanced mappings, slate-green design, catalogs and one-row footer remain.

## Verification results

**9 native runtime suites passed:** Rc4IntegrationTests, PeopleUxTests,
SelectedTrackEditingTests, RideFlowTests, ManagementUxTests, ConstructionUxTests,
ShopAppearanceTests, PauseResumeTests, FirstParkFlowTests. **6/6 standalone CTest
checks passed.** Native GUI/CLI and Cortex-A53 AArch64 GUI/CLI builds passed.

The added native tests inspect actual track elements, geometry, chain, native
costs/cash, invalid placement, undo/cancellation, preview suspension, active-ghost
save/reload, temporary ride exclusion, spatial selection and transient highlight
cleanup. Native guest entry, tabs, roster context, tracking/follow/pickup, staff
entry and guest event references pass. Ride/research/category event handoffs,
blank/stale events, history/dismissal and construction draft preservation pass.

There are **14 actual 512×384 runtime PNGs** in `captures/rc4/`; the showcase is
`08-spatial-track-selection.png`, `09-spatial-track-actions.png`,
`02-lift-ghost.png` and `05-native-catalog.png`. Original commercial asset files
were used only for local testing and are not bundled in the delivery archive.

Two older information drivers still fail at Ledger entry and Pocket map entry;
they use fixed-row navigation from before the established RC3 tabbed workspaces.
They are recorded failures, not passes. Current management/ride/people/construction
journey tests pass; migrating those two drivers remains unfinished.

Package verification passed archive CRC, SHA-256 manifest contents, safe paths,
executable permissions, AArch64 ELF and complete private dependency closure,
generated graphics, public objects/title sequences, 1024×768 / crisp 2× setup,
launcher and isolated `.userdata` save/config paths. Packaged GUI and CLI version
checks execute under QEMU 8.2.2 with the private loader. They report
`OpenRCT2, v0.5.5 (31f7b4d)`; local implementation source is
`31f7b4d2f403391871550ba775626aa17dd753ad`, with canonical implementation/tree
recorded in the manifest. Local archive reconstruction omits historical evidence
excluded by the RC3 source bundle and preserves runtime source bytes; canonical
publication retains all preexisting files and modes.

## Installation

Back up the SD card's saves. Rename the existing package's `game` folder to
`game-before-rc4`, then copy the new `game` folder and package instructions,
launcher and metadata into `Tools/tg5040/OpenRCT2 Pocket.pak/`. Keep the existing
`RCT2` folder and `.userdata/tg5040/openrct2-pocket/`. Use a fresh game directory.
For a first installation extract the ZIP to the SD root, then supply legitimately
owned RCT2 Data, ObjData and Scenarios under the package's RCT2 folder.
`RCT2/Data/g1.dat` is required. Start through NextUI Tools. The bundled INSTALL.md
contains the complete instructions and transferred-file checksum command.

## Remaining limitations

No physical Brick is connected. Device graphics/audio, real controls, dense-park
performance, suspend/resume, actual SD-card save round trips, translations and
perceived ghost/readability improvements require hardware testing. QEMU version
checks do not validate those systems. This is a device-test candidate, not a claim
that every original RC4 acceptance criterion has been satisfied.
