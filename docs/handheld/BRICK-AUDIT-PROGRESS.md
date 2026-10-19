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
