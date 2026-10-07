# OpenRCT2 Pocket shared UI direction

Design proposal following the first successful physical Brick terrain/save test.
Reference: OpenRCT2 Brick UI Concept B, supplied by the user.

## Evidence and scope

User reports launch, scenario selection, direct park navigation, terrain raise/lower,
and save-and-quit succeed on the physical Brick. Reload/persistence verification is
pending. This establishes the tested interaction foundation; later construction and
management tools still require their own implementation and hardware verification.

## Visual rules

- Design at 512 x 384 logical pixels. Validate actual screenshots at that size;
  enlarged posters do not establish readability on the 3.2-inch display.
- Navy panel backgrounds, restrained blue borders, white primary text, pale blue
  secondary text, solid yellow selected rows with dark text.
- Use original OpenRCT2 pixel artwork and sprite icons. Keep park graphics dominant.
- Share text styles, row spacing, borders, button badges and footer rendering.
- Selected rows need a filled highlight and a clear focus marker. Disabled rows
  retain readable text and expose a reason, e.g. insufficient funds.
- Green/red communicate money and status with explicit signs or words. Yellow
  means focus, not an unrelated second status system.
- Prefer short labels and comfortable rows; page long lists. Avoid shrinking text
  to fit every setting at once. Truncate long names and show full names in Details.
- Preserve current crisp scaling and software-rendering compatibility.

## Three shared layouts

1. Park view: slim status strip (cash, guests, rating/date), existing centre reticle,
   compact contextual selection/cost card, bottom command strip. Keep the centre
   available for park viewing; never cover the reticle with an information card.
2. Build/inspection: a compact side panel with a short list of options; park or
   placement preview remains visible. Choosing a tool collapses the catalog into
   the active tool controls. Terrain, paths and scenery share this arrangement.
3. Management: a larger panel with category navigation, details and an optional
   graph/preview. Finances, staff, research and ride management share this structure.

Initial sizing guides: 8-pixel outer spacing, 32-36-pixel primary rows, no more than
six primary choices in a panel, and roughly 28-34 pixels for the command strip.
These are design targets, subject to native sprite-font and 512 x 384 screenshot
checks. Keep labels and essential monetary values readable before adding decoration.

## Interaction rules

- D-pad pans directly in the park and changes focus when a panel is active.
- A selects/confirms or applies the active construction action; B returns one level
  or leaves the tool. Leaving a tool is not undo.
- Start opens the park/pause menu; Select opens Build, preserving tested behavior.
- X retains park rotation during existing terrain editing. Y retains terrain
  raise/lower cycling. L/R retain camera zoom for park/terrain interaction.
- Menu and management contexts can use L/R for categories; every context displays
  the actual bindings. Do not silently repurpose existing park controls.
- Y can open Details in browsing contexts when not already assigned to a tool.
- Future path and coaster orientation/height controls require explicit per-tool
  mappings; do not inherit inconsistent bindings from the generated reference.
- Additional Brick buttons are supplementary shortcuts, never required to complete
  the basic loop. Test their mappings before relying on them.
- Show placement preview, prospective cost and specific action failures. A must not
  spend money when the native action query rejects the placement.
- Destructive confirmations start on Cancel. Save-and-exit quits only after success.

## Implementation sequence

1. Extend the shared Panel layer: palette, typography, spacing, selected/disabled
   rows, button badges, section headers and contextual footer. Restyle title,
   scenario picker, park/pause, terrain and save/load together. Capture native
   screens at 512 x 384 and deliver a Brick build for readability feedback.
2. Paths: choose a loaded path surface and normal/queue mode, preview a valid
   placement, place/remove and save/reload. Begin with ground-level placement;
   add slopes, bridges and connected building as a separate validated extension.
3. Scenery and shops: use the same catalog, details, preview, cost and placement
   controls. Keep native ownership, clearance and action validation.
4. Park dashboard and finances: expose real game values, operational controls and
   existing finance history; do not invent poster statistics.
5. Ride inspection and management: operating status, prices, trains and settings.
6. Coaster construction: piece selection, orientation, height, connection checks,
   preview and editing. This is a larger dedicated construction milestone.
7. Staff, research, marketing and remaining operational screens, applying the same
   layouts and navigation rules.

## Completion checks per slice

Native build; relevant controller/action tests; actual native UI captures at target
size; AArch64 build and dependency audit; physical Brick interaction test; and a
save/reload check for changes to park state. Explicitly distinguish completed checks
from pending hardware or persistence tests.

## First implementation pass (0.4)

Shared native palette surfaces, section headers, 32-pixel focus rows, explicit
black/white formatted labels, ellipsis and button badges now cover the controller
title menu and park/build/save/load/confirmation panels. The title widgets' hit
areas follow the new visible rows. The existing scenario picker gains matching
blue colors, filled yellow focus, clipped names and 32-pixel rows. Terrain and
exploration command strips share the button components and clip long action
messages. Tested action bindings and save publication logic are retained.

A component capture uses OpenRCT2's actual X8 renderer, palette and sprite fonts at
512 x 384. It is not a gameplay screenshot or a physical Brick test. This pass
has not yet converted the scenario details pane or implemented the planned park
status strip, construction sidebars, paths, dashboard or management screens.
