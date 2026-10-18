# Ordinary-play handheld flow — RC3

Continues canonical RC2 `24b114f2ce9a9ee7af7316d29e1058e884667450` on
`handheld/modern-ui-paths`. RC2 fixed the title and shop palette but left several
first-play refinements as proposals. This pass implements those ordinary-play
surfaces; it does not claim that every advanced editor is now visual or that
desktop automation establishes physical handheld comfort.

## Before / after decisions

| Surface | RC2 observation | RC3 implementation |
| --- | --- | --- |
| Build | Fourteen text actions; only eight visible | Shared spatial icon cards, all fourteen actions visible on one page; original action identities and routes retained |
| Path entry | Text list mixing normal, queue, connected and removal | Six visual cards with a single focused explanation; Extend describes endpoint construction |
| Scenery category | Text-only category menu | Native scenery icons and a spatial category grid |
| Custom track entry | Text list for custom build versus design | Shared cards for Build track, Saved design and Back |
| Object catalogs | Six narrow cells left unused space on the right | Wider cards, native object preview viewports and labels; six entries/page and native eligibility retained |
| Path placement | Truncated surface; controls repeated across several lines | Native material preview, wrapped name, Single/Line/Fill strip and current geometry/selection count |
| Line/Fill review | Repeated button instructions | Single review question and explicit stop-on-problem, completed-tile and charge consequences; footer owns Confirm/Preview |
| Flat ride placement | Text stage and repeated return instruction | Native ride thumbnail, Ride/Entry/Exit progress, full next-action label and station/orientation; existing-access editing has no misleading completion strip |
| Shop placement | Selected name in a small single-line row | Wrapped name and named facing direction; existing Select/Turn and shop catalog retained |
| Scenery mode | Text value cycling | Four visual Place/Recolour/Remove/Pick tiles representing the existing mode; existing Left/Right cycling remains |
| Scenery / terrain colors | Left/Right textual color names | Swatches in supported setting rows; A opens shared native palette with safe cancellation |

The slate-green theme, simulation, costs, native GameActions, research/object
eligibility, geometry and save format are unchanged. Numeric dimensions/prices
remain numeric controls. Nothing is unlocked or made free for ordinary gameplay.

## Controls and state

- Choice cards: D-pad moves spatially, A opens the existing action, B returns to
  the existing caller. Edges do not wrap into another row. Partial last rows are
  bounded. A card's action identity is independent of visual position (Paths
  reorders Extend and Back without changing their native handlers).
- Back from a nested card page restores its parent selection. A newly created
  Build menu retains the existing first-action default; persistent cross-session
  Build bookmarks are not added by this pass.
- Scenery Colour and supported terrain Paint color rows: A opens the palette;
  D-pad selects; A uses the color for the tool; B/Y cancels back to the same
  setting. Navigation does not alter the tool color. Neither acceptance nor
  cancellation paints, places, pans, charges money or abandons the tool.
- Outside the palette, existing Left/Right adjustment remains. Scenery now cycles
  the actual ordered normal palette, rather than modulo-72 native enum values.
  This matters because native normal colors include enum values 72 and 73 while
  the special invisible/void values are inserted earlier in the enum.
- Unsupported color channels remain disabled and do not open a picker. The
  palette exposes only normal native colors, not cheat-gated invisible colors.
- Path Adjust retains L1/R1 mode changes, D-pad geometry and B return to Build.
  Active Line/Fill B cancels the preview before leaving construction. Reviews
  retain their native action queue and partial-success behavior.
- Placement retains D-pad camera movement and existing zoom/catalog controls.
  The flat panel exposes Select/Turn; duplicate Start return is guide-only in its
  shared footer. No working camera or construction binding is remapped.
- Placement feedback shows the actual native rejection/reason without a repeated
  all-caps tool prefix. In particular, entrance placement no longer reports the
  generic PLACE RIDE label. Palette screens suppress unrelated placement prompts.

All captured ordinary footers remain one 28-pixel row. Full hints remain in the
existing park Controls guide. Dashboard layout/actions are unchanged.

## Native verification

GUI/CLI builds pass on desktop Linux and the supported Cortex-A53 AArch64 SDK.
The executable version stamp identifies local source checkpoint `6e5a8a1`.
Canonical publication identity is recorded separately in the release manifest;
later documentation/capture changes do not alter the tested C++ implementation.

Eight initialized native drivers pass:

1. FirstParkFlowTests: animated title/dirty redraw, New park/Back, Electric Fields,
   spatial Build/Paths/Shop/Ride routes, nested focus return, legal paths/exact
   charges, two researched vendors, shop swatches/review/cancel, native scenery
   mode/palette cancellation, Merry-Go-Round/entrance/exit, ride appearance and
   native save/reload/title return.
2. ConstructionUxTests: scenery rotation/modes/scatter, palette draft/cancel/use,
   all 72 normal choices in native order, supported terrain-edge palette
   cancellation/acceptance, camera/accounting isolation, terrain
   brush/material/edge operations, native costs and held-controller transitions.
3. PathBatchTests: Single/Line/Fill, queues, invalid selection cancellation,
   native costs/connectivity, queued failure reporting, preview bounds and reload.
4. RideFlowTests: native Undo/pieces/camera isolation, access completion/cancel,
   maze and roster focus restoration.
5. ManagementUxTests: finances, graphs, research Apply/Discard/Back, marketing,
   disabled controls, caller return and native save/reload.
6. ShopAppearanceTests: supported channels, apply/cancel, stale contexts,
   serialization and mapped SDL input.
7. PauseResumeTests: pause reasons, Controls return, Build/Paths Back hierarchy
   and native save/reload.
8. PeopleUxTests: staff duties/patrol, guest needs/thoughts, placement and focus;
   1,024-guest roster fixture.

Six standalone tests pass, including launcher/save protection, input, file
navigation and shared footer/card-grid bounds. Choice-card tests cover reordered
action IDs, edges and short rows. The palette test caught and verifies the
scenery enum/order correction rather than masking it with a screenshot.

Native PNGs in `captures/handheld-flow-rc3/` are 512x384. RC2 counterparts remain
in `captures/first-play-refinements/`; compare 04-build-menu, 04b-path-mode-menu,
05-paths and 10-flat-entrance. These are actual native software rendering with
the supplied assets. Legal-tile searches/direct world targeting are test-driver
conveniences, not a physical D-pad playthrough. The first-park fixture does not
certify an operational connected queue network; RideFlowTests separately covers
native operation/access workflows. ConstructionUxTests' generated fixture is
separate from the asset-backed scenario captures.

## Remaining limits

- All advanced track/design/connection editors and legacy More settings have not
  been converted to icon grids. Exact technical geometry stays available.
- Build has fourteen cards, including intentional overlapping quick/full tools;
  no legitimate routes were deleted. Whether further grouping is worthwhile is
  a device-play observation, not a claimed completed navigation redesign.
- Catalog cell names may still abbreviate; the full selected name wraps below.
- The 216-pixel path panel prioritizes material and state but can still occlude
  a wide Fill footprint (P7-01). No placement transaction behavior changed.
- English logical-resolution captures do not establish translated-label fit or
  physical Brick readability, button comfort, GPU/audio, suspend or performance.
- RC1's reported working hardware session is useful evidence, but does not
  certify this changed RC3 build. Retain the physical-device backlog.

## Install

Use `OpenRCT2-Pocket-Brick-RC3-20261018.zip`. Replace the existing package's
`game` directory with a fresh copy, keeping the prior directory as a backup.
Keep your `RCT2` assets and `.userdata/tg5040/openrct2-pocket/` saves/config.
The launcher and its working device mapping are unchanged. The archive contains
the real ARM GUI/CLI, input-map, private runtime, public engine resources, source,
instructions and checksums; it contains no commercial RCT2 assets.

Run the package verifier for CRC, architecture/ELF segment bounds, dependencies,
resource lookup, launcher paths/modes and hashes; QEMU `--version` checks test
loader closure, not physical graphics/gameplay. On the copied SD payload,
`shasum -a 256 -c SHA256SUMS.txt` from `game` verifies the transferred bytes.
