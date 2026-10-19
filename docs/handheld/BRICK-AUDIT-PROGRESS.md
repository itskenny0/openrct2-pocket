# Brick-native audit implementation progress

Continuation of canonical `c8c869a36457921969a1c39b1c14f8f684440b36`.
This file records incremental source checkpoints. The whole audit is not finished.

## Batch 1 — ordinary edits

- Naming Done submits the existing guarded native action for rides, shops, staff,
  guests and signs; B leaves the unsubmitted keyboard draft.
- Ride appearance selection submits the existing guarded draft directly.
- The shared keyboard labels Space, Delete, Case, Clear and Done explicitly.
- Staff duty drafts retain their unsaved indicator and Apply/discard controls,
  without repeated instructions. Guest needs retain their native meters and
  attention indicators without the persistent tutorial.
- Consequential vehicle, financial, destruction and overwrite reviews remain.

Validation pending: the native build environment is being restored after cleanup.
This is a source checkpoint, not a verified release or device-tested package.

Remaining scope: native verification and captures for batch 1; remaining common
edits/focus/disabled states, ride grouping, saved-design workspace, map, research,
staff previews/patrol, save previews, news, controller guide, audit/backlog closeout.
Physical Brick/NextUI testing is unavailable. Line/Fill remains deferred.

## Batch 2 — visual secondary workflows

Source checkpoint (runtime verification pending):

- Per-page focus restoration supplements existing tab/roster restoration.
- Guest tracking submits its guarded native action directly.
- Scenery adjustment filters unsupported colour channels and clamps changing focus.
- Ride Operations orders operating mode, vehicles, dispatch and maintenance.
- Saved-design options show the real four-orientation native preview beside direct
  height, automatic height, facing and scenery controls, vehicle choice and details.
  Native query cost/readiness and financial/compatibility rules remain authoritative.
- Saved-design vehicle choices and research reuse the existing visual picker.
  Research construction handoffs are offered only for current, researched objects
  supported by the existing shop/flat/maze/track tools.
- Map area expands to 472×304 with mode tabs, legend swatches, selected subject,
  ownership and R2 optional coordinates/dimensions/rotation.
- Native role/costume sprites accompany staff selection; patrol L2/R2 resizes a
  square brush while separate rectangular controls remain available.
- Park/Build/Menus controller guide shares the production button mapping, including
  configured A/B swaps. Save/search keyboards retain the shared full key labels.

Remaining: native verification/captures; compact saved-file inspection/quick slots,
news layout, track-action ordering, direct shop appearance, final audit/backlog.

## Batch 3 — secondary actions and previews

- Native Release GUI build and BrickFeelTests PASS after the environment restore.
- BrickAuditTests PASS for direct ride naming/Done/B discard, staff sprite screens,
  visible brush shortcuts, visual research browsing, map/details and safe return,
  actual news subjects, guide sections and configured A/B mappings. Captures are
  genuine 512×384 native viewport/component frames; no physical Brick was tested.
- News presents its actual subject and primary contextual action first, followed
  by available location/history/dismissal; paging appears only for long messages.
- Quick slots and file inspection lead with park name, real native preview when
  present, native date, rating, guests and rides. Browse retains folders/Refresh.
- Saved-design file inspection moves compatibility identifiers into Details.
- Track actions prioritize Continue/Replace/Remove while retaining advanced actions.
- Shop appearance applies its guarded native action directly; B cancels the palette.
- Ride vehicle-object choices use native visual cards and retain train-change review.
- Design export uses readable scenery names and spatially highlights selected
  scenery while restoring temporary map-selection state.

Broader regression checks and final capture publication remain in progress.
