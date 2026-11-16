# Phase 8B pre-change audit (canonical 390ff104)

- Dashboard rail B closed to park; content B returned through caller/rail. A fixed
  44-pixel footer reserve consumed space even for simple controls. The rail preview
  also used 48-pixel metrics and 30-pixel actions: the unchanged six Park actions
  could spill to Page 2. No actions were newly added by that pagination.
- Ride/guest/staff tab and roster bookmarks already existed. Native target identity
  and caller contexts were retained by RideNavigation/PeopleNavigation. Long rosters
  already had shoulder paging; visual catalogs did not.
- Research tab changes recreated/dropped drafts; B/Select/Start could leave without
  a decision. Staff tab changes reset duty drafts. Review Cancel also reset research,
  unlike B from the same review. These are verified source defects.
- Scenery/terrain Adjust B/Y already returned to Build without spending. Path Adjust
  B returned to Build. Line/Fill B cancelled selection or returned review to preview.
  Existing placement/pick-up cancellation reviews and native costs are retained.
- Park and direct path/scenery/terrain/patrol D-pad already pan; L1/R1 zoom. Adjust
  owns D-pad and shoulders; B to Build restores camera movement without resetting
  settings. No extra held modifier is needed for these tools.
- Coaster camera had a piece-catalog option plus Action::cameraMode / SDL
  left-stick-click (the latter unavailable on Brick). The catalog footer did not
  expose camera as a shortcut. Maze Select already toggled camera. Camera A could still place native
  construction, and coaster Y/trigger adjustments remained active while in camera.
- ControllerInput already used 320ms initial D-pad delay / 95ms repeat; face actions
  were edge-triggered and input suspension required release across transitions.
  Preserve that tested scheme, extending tests rather than inventing a new chord.
- Existing save/load/pause returns, destructive Cancel-first dialogs, native queries,
  queued callbacks and source-level eligibility remain the authority.
