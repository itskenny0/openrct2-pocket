# Native handheld UI integration

Integrated into `handheld/modern-ui-paths` following acceptance on 2026-10-20.

Accepted source: `5746fa4cf472666912b837e018622b946303abce` from
`experiment/native-menu-feel`. Previous development head:
`14d08a737119f81d01d656d585a49226d44fd68f`.
The accepted UI was five commits ahead with no divergence. Promotion preserves
the complete development history and all existing quit, path, speed, crash
recovery, coaster construction and actionable-news fixes.

## Current UI baseline

- Daylight is the default; Slate remains selectable through the pause menu.
- Native sprites, section-aware tabs, selected animation, raised buttons,
  focus/press feedback and full-label space checks are accepted foundations.
- The layout follow-up includes bounded two-line Build labels, separate guest
  warning lines, readable graph/currency text, full-width search/save keyboards,
  and contained construction hints.
- Routes, controller bindings, focus restoration and the single-row footer stay
  intact. The abandoned purple palette is historical, not an active theme.

## Verification and acceptance boundary

The exact accepted source passed seven native initialized runtime suites,
three additional saved-Slate runs, and all nine standalone checks. Actual
512x384 screenshots, sprite-frame loading and pixel-level card/label checks
are documented in [NATIVE-SPRITE-COVERAGE-PASS.md](NATIVE-SPRITE-COVERAGE-PASS.md)
and [DAYLIGHT-LAYOUT-POLISH.md](DAYLIGHT-LAYOUT-POLISH.md).

This integration adds documentation to the tested source; executable source is
unchanged from `5746fa4`. No further test rerun is necessary for that note.

Status: accepted development baseline, ready for an AArch64 package build and
physical Brick/NextUI acceptance testing. The latest UI has not been physically
tested, and this promotion does not itself update an installed device. Hardware
readability, timing/performance, longer locales/mod assets and the full state
matrix remain QA work. Earlier experiment documents describe their historical
scope; they are not instructions to keep the accepted UI out of development.
