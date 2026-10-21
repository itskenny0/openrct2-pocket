# Friendly tools and secondary-screen polish

Continues published `7114678ade4766b34b2b0b0ace95ee135522524a` on
`handheld/modern-ui-paths`. The accepted Daylight/Slate themes, native sprites,
continuous lists, controller bindings, catalogs, ordinary coaster construction,
quit/path/speed fixes and single-row footer remain the foundations.

## Changes

- Track connections remain beside the highlighted native track. Ordinary labels
  name the piece, open ends and station entrance/exit state. Coordinates,
  direction indices and native sequence counts are behind Technical details.
  Readouts have no action-button border/arrow. Technical-details toggling retains
  focus, and Back restores the selected action on the track-piece screen.
- Scenery's extended settings show the real native object preview, current height
  policy and an explanation of the selected control. Unsupported controls are
  visibly fixed, guarded against adjustment, and explain why. Scatter uses
  Sparse/Medium/Dense rather than internal proposal counts. Actual errors remain.
- Scenery's ordinary height field now changes elevation with Left/Right from the
  real placement proposal. A restores automatic ground height without leaving
  adjustment. Height displays normalized levels, not raw world-coordinate Z;
  path furniture explicitly follows its selected real path. Existing native
  placement, attachment, clearance and accounting rules are unchanged.
- Landscape's extended settings show a brush footprint and selected-control help.
  Brush width/depth are explicitly measured in tiles, distinct from elevation.
  Exposed land edges are labelled cliff faces. The ordinary brush editor also
  shows its footprint. Clear-area, land-rights and patrol dimension labels use
  the same distinction.
- Path settings explain ground following, fixed elevation, slopes, restarting
  from another path end and undo. Connected Continue level uses the existing
  direct native level operation. Single/Line/Fill placement behavior is preserved.
- Saved-design availability leads with readable names, compatible vehicles and
  scenery consequences; repeated object types are grouped. Exact object IDs and
  mapped-path IDs remain in Technical details. Missing scenery still explicitly
  warns that ALL design scenery will be omitted. Research states, changed train
  limits, partial placement and native failures remain visible. Confirmation
  replaces origin coordinates with the actual placement-height policy.
- Small Pocket-authored diagnostic phrases in clearing, duties and construction
  recovery are rewritten as player actions. Engine error messages are untouched.

## Review and validation

Reviewed current menu routes/presenters and tool overlays, preserving the already
accepted build catalogs, ride/staff/guest workspaces, park/map/news, finances,
research/marketing and save/load foundations. This is not a claim that every
locale, mod object or possible simulation state has been played.

Restored the published source and native QA cache; compared all 1,823 cached
build/test inputs before editing. The sole non-UI launcher difference in that
cache was resolved from the current remote; it is not part of this change.
Native Linux x86-64 Release/O1 GUI build passed using private dependencies.
Existing host GCC null-dereference/maybe-uninitialized diagnostics required local
warning-only treatment, including at LTO link; engine code was not changed for
those warnings. Optional network/HTTP/TTF/OpenGL/audio-codec features remain
omitted in this host QA configuration.

Ten native initialized runtime suites passed:
FriendlyUiTests, SelectedTrackEditingTests, DesignSubstitutionTests,
SceneryPathParityTests, EverydayControlsTests, PathCursorTests, RideFlowTests,
ManagementUxTests, PeopleUxTests and BrickAuditTests.

The new driver navigates actual production controller handlers, checks height
changes and A ground reset without spending money, caller return, track technical
show/hide focus, Back focus restoration, unchanged native track context and the
28-pixel footer. Existing suites cover real placement/ghost cleanup, connections,
lamps, query/cancel/failure/partial accounting, save/reload, native speed/pause,
quit success/failure/cancel, roster navigation and coaster construction. The
EverydayControls suite includes mapped SDL virtual-controller input. Fixtures
and scripted controller frames are not physical hardware tests.

Captures are native 512x384 engine rendering with original game graphics and
initialized test parks, not mockups. The fixtures are intentionally small; these
are not screenshots of the user's personal park.

## Acceptance boundary

This checkpoint is source and native-host verification. It does not contain a
new AArch64 package and has not been physically tested on TrimUI Brick/NextUI.
Device readability, crisp 2x presentation, input/motion timing, performance,
audio and suspend remain hardware acceptance work. The installed binary changes
only when a matching device build is delivered and installed.
