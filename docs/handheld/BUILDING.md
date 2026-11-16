> Current Brick release-candidate build and packaging: see
> `packaging/handheld/nextui/BUILD.md` and
> `docs/handheld/CONTROLLER-FOOTER-BRICK-RC1.md`. The baseline/prototype recipes
> below are retained as historical development instructions.

# Build and test instructions

Read `VALIDATION.md` before using an artifact. The x86_64 executable is for Linux development, not the TrimUI Brick or macOS. The ARM toolchain file and PortMaster launcher are unverified staging templates.

## Native Linux baseline and patched build

The inspected source baseline is `21afdbc55008335cd70e72a53d465608f9f645ac` (OpenRCT2 v0.5.5 development). CMake 3.24+, GCC 12+ or a compatible C++20 compiler, and Ninja are required. The native verification environment uses Ubuntu 24.04, GCC 13.3 and SDL 2.30.

Install dependencies (Ubuntu/Debian names; versions differ by distribution):

```sh
sudo apt-get update
sudo apt-get install build-essential cmake ninja-build pkg-config ccache \
  libsdl2-dev libicu-dev libzip-dev libpng-dev libssl-dev \
  libcurl4-openssl-dev libfontconfig1-dev libfreetype-dev \
  libflac-dev libogg-dev libvorbis-dev libgl-dev libzstd-dev \
  nlohmann-json3-dev
```

To independently build the unchanged baseline:

```sh
git clone https://github.com/OpenRCT2/OpenRCT2.git openrct2-baseline
cd openrct2-baseline
git checkout --detach 21afdbc55008335cd70e72a53d465608f9f645ac
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DDISABLE_DISCORD_RPC=ON -DDISABLE_IPO=ON
cmake --build build --parallel 4
./build/openrct2 --version
```

The delivered source archive already includes the patch. In its root:

```sh
BUILD_JOBS=4 bash scripts/handheld/build-native.sh
```

Do not apply the separate patch again to that source archive. To use the patch with a fresh checkout instead, check out the exact baseline above and use `git apply --check` followed by `git apply` on the patch.

The build generates native engine graphics. `cmake --install build-handheld --prefix "$PWD/stage"` also installs engine data and fetches the additional asset packs according to `assets.json`. That install requires network access. The native build alone does not provide original RCT2 graphics, nor does it prove gameplay has launched. Keep a manifest of the package versions, compiler, source revision, config and hashes; these instructions provide a repeatable recipe, not a claim of bit-identical builds across operating systems.

## SDL input tests (without game data)

```sh
cmake -S test/handheld -B build-input-tests -G Ninja
cmake --build build-input-tests
ctest --test-dir build-input-tests --output-on-failure
```

These tests create virtual SDL controllers and exercise the real event/state API. They need SDL 2.0.14+ for the virtual-device test API. They do not validate park rendering, native file dialogs, full game focus ordering, framebuffer output or physical key labels.

## Native park-explorer launch

After installing engine data, provide a copy of a compatible park and your own RCT2 installation. Use a separate user-data folder:

```sh
mkdir -p user-handheld
cp packaging/handheld/config.ini user-handheld/config.ini
OPENRCT2_HANDHELD=1 ./build-handheld/openrct2 /absolute/path/test.park \
  --user-data-path="$PWD/user-handheld" \
  --openrct2-data-path="$PWD/stage/share/openrct2" \
  --rct2-data-path=/absolute/path/RCT2
```

For a desktop 512×384 window, change only this profile to `window_width=512`, `window_height=384`, `window_scale=1`, `fullscreen_mode=0`. The Brick staging profile requests fullscreen desktop, 1024×768 output, scale 2, DPI inference off. Verify actual output dimensions; fullscreen desktop follows the available display mode.

The controller must have an SDL mapping. PortMaster normally supplies `SDL_GAMECONTROLLERCONFIG`; on a desktop use a known mapped controller or supply a valid mapping for the test device. Do not run a mouse/keyboard translation profile simultaneously. The first device is selected; release all buttons after connection, focus restoration or a one-shot action before the next action.

Current controls: D-pad pan; A centre selection; B dismiss/cancel/back; X rotate clockwise; L/R zoom; Start pause. Y/Select are reserved. Desktop windows, text entry and save dialogs still require fallback input. This proof does not provide a controller-only way to quit; use desktop input during host testing. A proper save/exit screen is a release gate for a usable device build.

For baseline behaviour, launch the same executable without `OPENRCT2_HANDHELD=1`, using a separate normal user-data directory. Do not overwrite a user's existing config for the comparison.

## Proposed AArch64 cross build

The checked PortMaster package is AArch64. A host Ubuntu ARM64 binary is not automatically compatible with a Brick running an older PortMaster runtime. Obtain the target runtime ABI first, using `scripts/handheld/collect-device-info.sh` inside that environment. Verify the toolchain, libc, SDL video backend and C++ runtime as a unit.

The supplied toolchain expects a **verified** SDK with tools named `aarch64-linux-gnu-gcc/g++` and a target sysroot. Adapt names to the actual SDK; do not point these variables at the native compiler or `/`.

```sh
export HANDHELD_TOOLCHAIN_BIN=/absolute/path/verified-sdk/bin
export HANDHELD_SYSROOT=/absolute/path/verified-sdk/sysroot
cmake -S . -B build-aarch64 -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/packaging/handheld/aarch64-toolchain.cmake" \
  -DCMAKE_BUILD_TYPE=Release -DPORTABLE=ON \
  -DDISABLE_DISCORD_RPC=ON -DDISABLE_IPO=ON -DDISABLE_OPENGL=ON
cmake --build build-aarch64 --parallel 4
```

Upstream skips graphics generation when cross-compiling but still installs those files. After building `graphics` on the host from exactly the same source revision:

```sh
cp build-handheld/g2.dat build-handheld/fonts.dat \
  build-handheld/palettes.dat build-handheld/tracks.dat build-aarch64/
cmake --install build-aarch64 --prefix "$PWD/stage-aarch64/engine"
```

Audit the output using `file` and `aarch64-linux-gnu-readelf -h -l -d --version-info`. Check the interpreter and every required GLIBC/GLIBCXX version against the **actual** runtime. Package compatible dependencies and notices, preserving the platform SDL/GPU integration. The intended layout is:

| Path under port directory | Contents |
|---|---|
| `openrct2` | Verified AArch64 GUI executable from this fork |
| `engine/share/openrct2/` | Matching generated graphics, languages, objects, sequences and engine assets |
| `libs/` | Audited, compatible required shared libraries |
| `RCT2/` | User-supplied original installation |
| `config.ini` | Initial profile template |
| `save-handheld/` | Separate user data; never replace existing saves |
| `test.park` | Disposable compatible park copied by the tester |
| `log.txt` | Launch/run diagnostics |

`packaging/handheld/OpenRCT2-Handheld.sh` follows the existing PortMaster launcher conventions but uses an isolated port directory and no gptokeyb mapping. It needs the real executable and full engine data; it is not a ready-to-install release. Its control discovery, exit cleanup and SDL presentation remain to be tested on the user's NextUI runtime. Preserve the user's working PortMaster setup rather than reinstalling it.

## First physical test, once ARM packaging is validated

1. Launch a copy of a small known park. Confirm the engine version and effective resolution.
2. Release every button, then test all four D-pad directions and diagonals; observe release stop and camera bounds.
3. Rotate through all orientations and pan; zoom both ways. Verify the pointer has not moved and the fixed reticle has not smeared.
4. Centre a ride/guest/entrance, tap A, then B. Confirm one action per tap, no unexpected underlying map movement, correct physical A/B labels, and clear return to the map.
5. Pause/resume; test tools with mouse fallback, then B cancellation. Confirm no placement occurs by accident.
6. Exercise focus loss and controller reconnect where possible. Hold a direction through recovery; movement must wait for release. Test actual suspend separately.
7. Save/reload using fallback input for this prototype; compare with the matching upstream version. Do not declare save round-trip verified before doing it.
8. Repeat on a larger park; record frame pacing, simulation speed, memory, sound, startup time and text comfort. No target FPS has yet been demonstrated.

Do not expand into new screens or coaster construction until the hardware explorer is usable and the input mapping is confirmed.
