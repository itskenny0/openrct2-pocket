# Solid palette polish checkpoint — 2026-10-07

Branch: `handheld/modern-ui-paths` in `keeganwhitacre/openrct2-pocket`.

This pass changes presentation only: solid deep-indigo panels and quiet single
borders, solid gold focus rows, quieter section labels and secondary state,
no per-row separator clutter, compact terrain brush information, 28-pixel action
strip, and 110 × 26 cost badge. Shared exact palette tokens live in
`src/openrct2-ui/handheld/PocketPalette.h`; see CONCEPT-UI.md for indices/RGB.
No panel dithering or texture remains in the Terrain/Path slice.

The native GUI build succeeds. CTest passes 3/3 (controller input, launcher,
save preservation). The native action integration test passes path placement,
queues, funds, fixed-height placement across tiles, raised slopes/rotation,
save/reload, removal, ownership/pause rules and research funding. Production
UI components were rendered and visually reviewed at 512 × 384 in all three
primary screens and six alternate states. Captures use sample state and the
reference park backdrop; they are not live park or device captures.

The newest physical Brick build has not been rebuilt or tested in this pass.
The user previously confirmed launch → open park → terrain edit → save and exit.
Recent path options and visual refinements still need a fresh AArch64 package
and physical verification. The indexed palette lacks the exact concept navy
and smooth RGB ramps; use solid palette entries rather than visual tricks.

## Smallest next development milestone

Implement a native single-tile path placement ghost for the current controller
path tool. Reuse the existing native path query/action and ghost lifecycle;
show the selected surface, normal/queue type, height and slope before A commits.
A must still perform exactly one real placement and charge once. Previewing
must not change park cash, persisted contents or normal simulation state.
Remove ghosts when the target becomes invalid, mode changes, options/pause/load
open, tool closes, park changes, or the session exits. Refresh on cursor and
configuration changes. Avoid duplicate ghosts and repeated unchanged updates.
Keep all existing controls and polished UI. Do not add connected bridge/tunnel
construction, new controller mappings, ride construction, management screens,
keyboard, or top HUD in this milestone. Add meaningful lifecycle/action tests
and build/test; use real park captures if the assets/runtime permit. Clearly
label any fixture capture. Checkpoint source on the existing GitHub branch.

Management, a digital naming keyboard and persistent top information HUD remain
later milestones; do not redo the completed style system before those slices.
