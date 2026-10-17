# Verified UI polish backlog

Keep observed UI defects separate from proposed features in `UX_FEATURE_BACKLOG.md`.
Do not treat historical checkpoint screenshots as the latest implementation.

Capture filenames below refer to the separately supplied runtime evidence;
PNG publication awaits explicit permission.

## Open

- **P7-01 — contextual panel occludes part of a batch footprint.** Reproduced in
  `captures/controller-ux-phase7/05-fill-preview.png`: the left part of the
  rectangle can pass behind the fixed 204-pixel tool panel. Endpoint labels,
  dimensions and visible outlines still identify the selection. Consider safe
  contextual relocation/collapse after controller testing; do not remove feedback.
- **P5-01 — optional bright-sand capture crash remains untriaged.** The Phase 5
  opt-in offscreen diagnostic failed after a successful native terrain action.
  Its cause is unknown. This milestone's grass/water desktop fixtures pass;
  neither fact establishes the missing physical-device result.

## Fixed during Phase 7

- Cancelling the directly opened path surface grid unnecessarily opened settings.
  It now restores the active tool; settings-origin Back still restores settings.
- The destination label coincided with the fixed cursor and was obscured.
  Endpoint badges now sit beside the cursor.
- The compact preview legend and long native water rejection were clipped.
  A shorter legend and content-sized, four-line selection feedback resolve the
  representative 512×384 cases. Very long localized text still needs inspection.
