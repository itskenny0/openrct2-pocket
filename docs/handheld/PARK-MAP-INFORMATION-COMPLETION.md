# Native Park Map and Information Completion

Goal: 100% ordinary OpenRCT2 gameplay through controller-native Pocket. This checkpoint is incremental; missing ordinary functionality remains on the roadmap.

## Phase 1 — native park map

Overview adds Park map after existing status/admission rows. The full-screen Pocket page shares desktop Map.cpp native tile colours, ownership dimming, ride-category colours, rotation transform and viewport bounds. A transient incremental raster refreshes at most 2048 tiles per update and rebuilds on map dimensions, mode or camera rotation changes. No map pointers or Pocket gameplay state are serialized.

D-pad moves the selected native tile, A explicitly locates and returns to park, X rotates the native camera, Y switches terrain/ownership versus ride/path colours, L/R changes zoom (whole park to 8x), B returns to Overview. Selection clamps to the playable interior of rectangular maps. A white box indicates the main viewport; gold cross indicates selection. Entry ends prior construction tools/ghosts. Page state dies with its menu on load/exit.

Release GUI/CLI rebuild PASS. ParkMapInformationTests PASS with native initialized 64x48 map, every interior tile compared to desktop native colour helpers, map bounds, four camera rotations, cache invalidation, native MapChangeSizeAction to 80x56, SDL menu/action routing, explicit locate/back and native save/reload. Esperanto checksum remains e37ba559b4d9d761e74f2bb284d1ec762b813e789655c0495a882c83651890b5.

Publication starts from canonical 7b99544ee1aaf037046fb8659cf5129e7ed1ccf3 / tree 2e7d6d92d9ba8e4e5378f26f2694442cca13ae15. This workspace had reverted to staff source; exact GitHub blob recovery produced local safety cd94db316c8ff93cca61b697a6a0de7c5cad69ea with that same tree without modifying Esperanto. Subsequent validated phase commits are recorded in the next checkpoint update to avoid self-referential commit hashes.

Remaining phases: native news/awards/history; map-linked subjects and recovery. Map currently has no peep markers/roster links. No AArch64, physical Brick performance/input, sustained routing or full SDL gameplay-frame validation is claimed. Captures use production rendering and private supplied assets; archives, scenario parks and generated private designs are excluded from publication.
