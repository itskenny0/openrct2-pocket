# Compact handheld tools and readouts

Continues `6bf0a4575d09d730f6002dcaef3dd56c6728f8fc` on
`handheld/modern-ui-paths`. The direction is a compact handheld game interface:
read the important state immediately, move focus between actions, and use
shoulder buttons for short groups. The accepted Daylight/Slate artwork, native
catalogs, ordinary coaster construction, simulation rules and 512x384/2x target
remain. This is not a GBA port or a change to the game's resolution.

## Track connections

The ordinary screen no longer uses a scrolling report or a detached selected-line
box. Ahead and Behind are fixed, wrapped readouts beside the actual highlighted
track. Station number, entrance and exit occupy one compact strip, shown only
for station pieces. Focus belongs to two actions: Technical details and Back.
Left/Right and Up/Down both select those actions. B returns to the original track
action with focus restored. Technical geometry stays available in the existing
advanced report. Removal consequences remain in native-backed destructive
reviews; they are not repeated in an ordinary connection readout.

Native open-end, connected-station and non-station fixtures are captured and
checked. Readouts are derived from the existing native connection/station
inspection. No synthetic readiness, path connectivity or opening eligibility is
inferred from these cards. Construction and money are unaffected by inspection.

## Settings groups

Scenery's twenty-row extended list is split into Place, Height, Colour, Tools and
Scatter. Landscape uses Brush and Paint. L1/R1 switches groups and remembers the
last selected field in each. D-pad up/down stays inside the visible group;
Left/Right retains direct value adjustment. All existing native action indices,
search/sign/catalog/pick flows, precise heights, disabled-control guards and
material-picker caller returns are preserved.

The tab bar spans the screen so labels fit without abbreviation. The native
object preview or brush footprint remains beside the short group. Each group
has at most six rows, so ordinary settings navigation needs no list scrolling.
Unsupported settings remain clearly fixed. The ordinary Y adjustment panels and
visual colour/material catalogs remain the primary fast building controls.

## Saved-design check

The ordinary warnings report is now one screen: selected vehicle/state, scenery
outcome and the relevant placement consequence. Missing objects explicitly say
ALL design scenery is left out. Known ride-type/station-style blockers remain
visible, and actual placement failures are retained. Train-limit changes remain
explained. The possibility of leftover scenery is only shown when scenery is
included and not omitted wholesale. Full native errors, individual object
states, IDs, mappings and partial-result information remain in Technical details.
Redundant copies of the all-scenery warning are removed from the shared warning
list. Confirmations, actual native queries/actions, partial accounting and save
formats are unchanged.

## Validation boundary

Native Linux x86-64 Release/O1 build and real 512x384 engine captures use original
park graphics and initialized native fixtures. Existing host GCC warnings in
unchanged engine/optional-value code retain the build-local warning-only
workaround; no engine validators were relaxed. Optional host QA features remain
disabled as documented in FRIENDLY-TOOLS-PASS.md.

FriendlyUiTests exercises actual controller dispatch for group switching,
per-group field restoration, group boundaries, disabled settings, material
caller returns, two-action connection navigation, technical show/hide, unchanged
inspection state/money and the single-row footer. It additionally builds native
connected station/straight fixtures. SceneryPathParityTests' explicit fixture
jumps now use native action-index selection; real group navigation is exercised
separately rather than treating a large synthetic D-pad delta as a group change.

Final verification results and exact screenshots accompany the delivery bundle.
Physical Brick readability, motion/input timing, frame rate, audio and suspend
remain unverified. This pass does not produce an AArch64 package or perform
ARM/QEMU checks. Dense real parks, translations/mod objects and every simulation
state have not all been played; this report does not certify every page as
fully polished.
