# Resumption checkpoint — 2026-10-06

## Where the interrupted task stopped

The active workspace contained clean branch `handheld/park-explorer` at `b4c220d`, not the uncommitted title/scenario/pause edits described in the interrupted run. The cross toolchain and newer native build directory were also absent. Saved Proof 1.1 NextUI and the launcher hotfix were recovered. Its BUILD-INFO identifies source `b4c220db34f781d53391ea890deb0c732ec922ba`; the ARM executable SHA-256 is `c24cc0f5a3b42fa03ee14ff39d923589fa70e2b9a515b25700d6b0eb387a92d2`.

The lost narrow menu slice was restored on that foundation, not imported from a nonexistent later commit. No broad UI rewrite was started.

## Changes

- `src/openrct2-ui/handheld/ControllerInput.*`: semantic actions, discrete menu navigation with 320 ms initial delay / 95 ms repeat, retained continuous time-based map pan, optional SDL A/B swap, one-shot emergency quit chord.
- `src/openrct2-ui/handheld/ListFocus.h`: reusable presentation-only list focus with wrapping/empty-list handling.
- `src/openrct2-ui/handheld/Menu.cpp`: 300×224 pause panel, highlighted rows, contextual footer, pause ownership, direct Resume/Save/Load/Options/Exit entry points. Exit needs a second confirmation and warns that it does not save.
- `src/openrct2-ui/handheld/Explorer.cpp`: scene/context routing into direct window actions. Retains viewport movement/inspection/rotation/zoom and focus overlay. Modal back works without repeated suspension preventing rearming.
- `src/openrct2-ui/windows/TitleMenu.cpp`: visible controller focus and direct activation of existing menu handlers.
- `src/openrct2-ui/windows/ScenarioSelect.cpp`: 504×360 handheld layout, unlocked-list navigation, scroll-to-selection, source-tab cycling, direct scenario callback. Desktop size remains unchanged.
- `Windows.h`, UI-only `WindowClasses.h`, Windows project manifest: declarations/registration for the added screen.
- NextUI launcher: recovered asset/runtime discovery, both visible/hidden launcher locations, no title-screen auto-scenario shortcut, physical A/B correction, existing user configuration preserved.
- Input and launcher fixture tests; ignore rules for target assets, saves, staging and build directories.

No simulation, world artwork, game-action implementation or save serialization was changed. Save-format compatibility is structurally preserved, but no new save round-trip was performed.

## Validation scope

Native Linux Release GUI and CLI compile/link and generated engine graphics succeeded, GCC 13.3. `--version` ran for both. The native validation profile disables HTTP, networking, TTF, OpenGL, FLAC, Vorbis and scripting; this is a test profile, not a delivered target replacement. Source defaults and the previously tested ARM executable were not altered by those build flags.

CTest passed 2/2. SDL virtual-controller tests exercise transport, action edges, menu steps, A/B override, quit precedence/release gating including staggered Start/Select, focus/reconnect safety, raw/mapped event suppression, map pan and list focus. Launcher fixtures verify both NextUI locations, title arguments and preservation of an existing config. Those fixtures do not emulate PortMaster rendering or hardware.

No original RCT2 game assets or Brick are available here. New menu rendering, scenario launch and pause ownership have not been exercised in a running park or on hardware. There are no new 512×384 application screenshots. The supplied design image is a concept, not evidence of implemented screens.

The previously shipped explorer is hardware-tested according to the user's report. The new menu slice is **native compiled/input-tested, hardware untested**. A new ARM menu executable has not been built in this restored environment. Do not install the changed launcher onto the old executable: it deliberately enters the title screen and depends on the new menu code.

## Reproduce native checks

On a host with the upstream documented development dependencies installed:

```sh
cmake -S . -B build-pocket-native -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DDISABLE_IPO=ON -DDISABLE_DISCORD_RPC=ON \
  -DDISABLE_NETWORK=ON -DDISABLE_HTTP=ON -DDISABLE_TTF=ON \
  -DDISABLE_OPENGL=ON -DDISABLE_FLAC=ON -DDISABLE_VORBIS=ON \
  -DENABLE_SCRIPTING=OFF -DOPENRCT2_USE_CCACHE=OFF \
  -DCMAKE_CXX_FLAGS=-Wno-stringop-overflow
cmake --build build-pocket-native --parallel 4
build-pocket-native/openrct2 --version
cmake -S test/handheld -B build-pocket-tests -G Ninja
cmake --build build-pocket-tests
ctest --test-dir build-pocket-tests --output-on-failure
```

The warning override concerns existing upstream picosha code under this host compiler. No game engine source was changed to work around build dependencies.

## GitHub gate

Target: `keeganwhitacre/openrct2-pocket`. Read access through the connected GitHub service succeeds. It has an initial `main` commit `66bed25b24af5824aebc59e9269ef1fa2c02e454`. Do not overwrite that history.

The shell has no authenticated Git transport: normal and managed Git URLs fail requesting a username. The connected GitHub API cannot reuse the upstream tree as a base in this non-fork repository. No branch/ref was changed remotely. Uploading only patches would not satisfy the requested complete-source/history checkpoint.

An authenticated Git workspace is required to finish the full-history push. Preserve the initial remote commit by merging it with the handheld branch when importing the histories; do not force-push. Local checkpoint tags distinguish the old physical proof from this native-only menu source.

## Next gate / smallest next task

Finish the full-source GitHub checkpoint and recreate the documented ARM ABI/toolchain route; package this narrow menu slice and physically test title → scenario → park → pause → resume/quit. Keep scripting enabled in the target profile as in Proof 1.1.

Only then start the North Star vertical slice: shared panel/HUD primitives, Build → Paths, tile focus and existing `FootpathPlaceAction`, followed by Dashboard → Finances. The saved park remains the persistent artifact. Save/load/options/toolbox, inspection windows and all construction/management are still legacy presentation; centre inspection uses existing viewport hit testing, not free-moving fake-mouse input.
