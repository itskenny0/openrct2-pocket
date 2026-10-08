# Guests inspection workflow checkpoint

Continues GitHub checkpoint `ca481568f531cc47efddd9dddd5b3c1eaaeb9eac` on
`handheld/modern-ui-paths`. Guests is now an available Dashboard section. This is
a read-only guest workflow; no native gameplay action, guest need/money value,
simulation rule, construction tool or controller mapping is changed.

## Complete player flow

Start → Dashboard → Guests → native summary → full paged roster → guest inspection
→ needs, thoughts, spending or carried items → locate in the unobstructed park or
back through roster/summary/rail → park/save/reload.

Summary shows native in-park and heading-for-park counts plus average happiness
computed from the native happiness values of current in-park guests. Empty parks
show average happiness as Unavailable. The roster follows native desktop Guest
list membership: all guests inside the park, excluding outside/heading guests;
there is no artificial roster cap. Ordering is by native guest number, with entity
ID as tie-breaker. Shared management pagination displays seven roster rows per
page at 512×384. Roster changes preserve a still-present selected identity where
possible; returning from inspection retains roster focus.

D-pad navigates; A enters/selects; B backs out one level. Select switches rail and
content with per-section view/row resume. All Guest pages are read-only, so there
are no drafts or confirmations to strand when switching focus. Start retains the
existing pause/build/save menu transition. Existing input suspension consumes one
semantic action per frame and requires release/fresh press across transitions.
Selecting a guest cannot also Locate; Locate cannot spill an A press into park
tools. No fake mouse behavior or new physical mappings are introduced.

## Native information and locating

- Inspection shows the native guest name, formatted native action/status and rides
  ridden. It provides Locate, Needs, Thoughts, Spending, Carried items and Back.
- Needs use exactly the desktop Guest progress-bar normalization: happiness and
  energy have the desktop minimum; hunger/thirst invert their native scales;
  nausea and toilet use the native desktop ranges/clamping.
- Spending uses native cash carried, cash spent, paid entry, rides, food, drinks
  and souvenirs with the shared native currency formatter. No-money parks report
  Not applicable, matching unavailable native financial information.
- Thoughts use `PeepThoughtSetFormatArgs` and native string formatting, including
  ride/product arguments. Desktop thought-page freshness filtering is retained;
  no thoughts reports No current thoughts rather than a fabricated opinion.
- Inventory enumerates actual native item flags, with native display strings and
  the desktop photo/voucher target formatting. Nothing carried is explicit.
- Selecting a thought/item opens its complete native text. Native format-aware
  UTF-8 wrapping splits long descriptions into six-line pages with bounded
  Previous/Next navigation. Short row/name previews retain the shared ellipsis
  behavior. Native wrapped text uses the native font fallback.
- Locate resolves a fresh target and calls native `ViewportUpdateSmartFollowGuest`
  on a temporary subject window to obtain the same guest/vehicle/ride fallback
  focus as desktop. It then closes management and calls `WindowScrollToLocation`
  on the main viewport. It does not enable persistent follow, create a desktop
  Guest window, insert a watched thought, pick up the guest or mutate its needs.
  Picked/unavailable locations fail honestly without closing the panel.

## Architecture and safety

`GuestManagement.cpp/.h` owns native enumeration, presentation, text/inventory
adapters, identity resolution and locating. `Menu.cpp` owns pages, roster focus,
selected identity and read-detail state inside the existing Dashboard shell.
`Dashboard.h` enables Guests. `Explorer.h` exposes the section inspection helpers;
the semantic input dispatcher itself is unchanged.

Targets contain entity ID, native guest number, park entry time and a removal
generation. Each read or Locate resolves again and excludes guests outside the
park. The existing native peep-close/removal notification in `WindowManager.cpp`
updates Guest generations before native entity removal, preventing a reused ID
even with matching guest number/entry time from reviving a stale selection.
Unavailable inspection becomes a Back-to-roster page. Loading closes the existing
menu and discards selection/read state through the established lifecycle; no
guest target or menu state is serialized into park files.

`ManagementPanel.h` adds optional native body lines for read-only details. Empty
body lines preserve all prior section layouts. The existing rail, pane geometry,
font hierarchy, palette and bottom strip remain unchanged. Solid palette entries
remain panel 118, border/separator 119, selection 53, primary 21 and secondary 18.
No dithering or broad UI polish is included.

## Verification

Native GUI/CLI build and all three CTest suites pass. Full UI-linked integration
and engine-only integration pass, including all previous tool/ghost/shop/Staff/
Finances/Research/Marketing tests. `git diff --check` and Python capture-script
compilation pass.

New tests cover:

- 20-guest full roster and later-page selection, outside/departed filtering;
- native needs normalization, thought freshness, native product/voucher inventory,
  exact currency presentation and no-money information;
- bounded multi-page read text, empty/unavailable information;
- rail/content view resume, B hierarchy, roster return focus, pause ownership;
- duplicate semantic input and actual mapped SDL held-A/release/fresh-A handoff;
- native walking location, picked refusal and hidden-on-ride fallback;
- native removal while inspecting and an explicitly reused entity slot;
- byte-for-byte unchanged guest data after reading/locating, unchanged park cash
  and expenditure ledger, and native guest information/roster save/reload;
- menu teardown on load and restoration of the original test park.

Fixture generation/object loading and field assignments exist only in integration
tests. Production never creates guests, loads objects or adjusts their state.

## Native 512×384 scenario captures

`test/handheld/GuestsCapture.cpp` and `tools/handheld/render_guests.py` render
production Guests presentation and the Dashboard over private Mines of Africa.
That scenario starts empty and closed. Capture setup uses native actions to open
the park and eligible existing rides, starts a real two-week park-advertising
campaign, then runs ordinary native simulation until 20 guests enter (with a
bounded timeout). It never generates fixture guests or edits needs/money/items.
Rendering is asserted not to change native guest information, roster, park cash,
loan, admission or ledger. The source scenario is not overwritten. Private assets
and PNG outputs remain outside git.

Files: `guests-overview.png` (rail focus), `guests-roster.png`,
`guests-inspection.png`, `guests-needs.png`, `guests-thoughts.png`, all exactly
512×384. The native scenario currently has 20 guests and two heading for the park;
the inspected native guest is Magnus N. New guests legitimately have no current
thoughts/items or spending; no fabricated data fills those pages.

```sh
python3 tools/handheld/render_guests.py --build ../build-ui \
  --data ../recovered/proof1/.ports/openrct2-handheld/engine/share/openrct2 \
  --ninja ../host-tools/usr/bin/ninja \
  --assets /workspace/scratch/486484e47585/rct-assets \
  --scenario '/workspace/scratch/486484e47585/rct-assets/Scenarios/Africa - African Diamond Mine.SC6' \
  --output /workspace/scratch/486484e47585/guests-captures
```

## Limitations and next milestone

The shared pane ellipsizes long names/status previews. Full thought/item text is
accessible through paged detail. Read detail is a snapshot selected while the
Dashboard owns pause; returning to its list reads fresh native state. Section
resume lasts while the menu is open, not across closing or loading. The full
roster is paged but has no search/filter in this milestone. Summary average
happiness is explicitly computed, not a fabricated native metric or forecast.

Validation is on the native host and virtual controller, not physical Brick/ARM
hardware. Multiplayer/replay races remain untested. Native rendering and palette
limitations are unchanged. Guest pickup/naming, keyboard, persistent HUD and
other management/construction categories remain outside scope.

Recommended next complete workflow: existing Rides management inside the Dashboard
shell — full native roster → inspect/locate → review available native operating,
rating/reliability/financial information → native open/close/test and permitted
bounded admission-price edits → explicit review/confirm/cancel → return/save.
Do not add new ride construction or redo completed sections.
