# Reproducing the Brick testing build

Use a private SDK directory; do not install the bundled libc into your host or device system. Host requirements: Linux x86-64, Python 3, tar, make, CMake >=3.20, Ninja, pkg-config, C/C++ compiler and standard ICU host build prerequisites. The tested compiler is GCC 16.2.0 AArch64 LP64, glibc 2.26, Cortex-A53. Official NextUI's GCC 8.3 toolchain is too old for current OpenRCT2 (GCC >=12).

Download into `$BRICK_SDK`:
- `gcc.tar.gz`: https://github.com/prepkg/gcc-toolchain/releases/download/1.0.0-beta/gcc-aarch64-linux-gnu.tar.gz (SHA256 da5113921eaeba7c9e9e7be104c610790a449ee13e98f09a5a7153ebb5dc739e).
- `trimui-sdk.tgz`: https://github.com/trimui/toolchain_sdk_smartpro/releases/download/20231018/SDK_usr_tg5040_a133p.tgz.

From the repository root, with `BRICK_SDK` an absolute private directory:

```sh
export BRICK_SDK=/absolute/path/to/brick-sdk
python3 packaging/handheld/nextui/extract-safe.py
# Compiler archive has non-executable libexec programs: normalize only this private copy.
find "$BRICK_SDK/compiler/aarch64-linux-gnu/libexec" -type f -exec chmod u+x {} +
bash packaging/handheld/nextui/setup.sh
cp packaging/handheld/nextui/target.cmake "$BRICK_SDK/target.cmake"
python3 packaging/handheld/nextui/download-deps.py
# Copy nlohmann-json 3.x headers into the private sysroot usr/include/nlohmann.
bash packaging/handheld/nextui/build-deps.sh
cmake -S . -B brick-build -G Ninja  -DCMAKE_TOOLCHAIN_FILE="$BRICK_SDK/target.cmake"  -DCMAKE_BUILD_TYPE=Release -DOPENRCT2_USE_CCACHE=OFF  -DDISABLE_NETWORK=ON -DDISABLE_HTTP=ON -DDISABLE_TTF=ON  -DDISABLE_OPENGL=ON -DDISABLE_DISCORD_RPC=ON -DDISABLE_IPO=ON  -DDISABLE_VORBIS=ON -DDISABLE_FLAC=ON -DDISABLE_GOOGLE_BENCHMARK=ON  -DBUILD_SHARED_LIBS=OFF  -DCMAKE_CXX_FLAGS='-Wno-error=stringop-overflow -Wno-error=maybe-uninitialized -Wno-error=null-dereference'
cmake --build brick-build -j6 --target openrct2 openrct2-cli
ROOT="$BRICK_SDK/compiler/aarch64-linux-gnu/aarch64-linux-gnu/sysroot"
"$BRICK_SDK/compiler/aarch64-linux-gnu/bin/aarch64-linux-gnu-gcc"  --sysroot="$ROOT" -mcpu=cortex-a53 packaging/handheld/nextui/input-map.c  -I"$ROOT/usr/include/SDL2" -L"$ROOT/usr/lib" -lSDL2 -o brick-build/input-map
```

The target cannot run the sprite generator during cross-compilation. Supply matching `g2.dat`, `fonts.dat`, `palettes.dat`, `tracks.dat` from a native build of this exact source into brick-build. Download and SHA256-check the objects/title-sequence releases pinned by assets.json; extract to an assets directory with objects/ and title-sequences/ children. Then:

```sh
python3 packaging/handheld/nextui/package.py --build brick-build --sysroot "$ROOT"  --assets /absolute/assets --stage /absolute/stage  --strip "$BRICK_SDK/compiler/aarch64-linux-gnu/bin/aarch64-linux-gnu-strip"
```

Zip the stage's Tools directory preserving Unix executable modes. The package helper rejects non-AArch64 dependencies or missing generated graphics. It includes source and dependency provenance; original RCT2 assets must be supplied separately.

## Runtime checks and limits

The packaged GUI and CLI version checks pass QEMU 7.2 using the private loader with `-B 0x100000000`. Without that guest address offset, explicit-loader QEMU execution crashes at MAP_FIXED of the non-PIE executable at 0x400000. QEMU's ordinary interpreter mode with the package as `-L` and its lib directory as LD_LIBRARY_PATH also passes. This establishes ELF/runtime closure, not gameplay or physical-device validation.

Vendor SDL 2.26.1 includes the native Mali backend but no dummy video driver. Target GUI fixtures therefore cannot initialize headlessly here. Actual device GPU/audio/display/controller compatibility, sleep/resume and performance remain unverified. Network, HTTP, OpenGL, TTF, Vorbis and FLAC are disabled; native software rendering, simulation, save formats and Pocket UI remain. Device GPU libraries are not redistributed or replaced.
