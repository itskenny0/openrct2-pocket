#!/bin/bash
# Builds the NextUI Pak for the TrimUI Brick from a clean Linux x86-64 machine, following
# BUILD.md step by step. Used by the GitHub workflows; also runs locally.
#   ci-build.sh <output.zip>
# Needs: BRICK_SDK (an empty or cached private directory), python3, cmake, ninja, curl and a
# native toolchain with OpenRCT2's host dependencies (for the generated graphics).
set -euo pipefail
OUT="$(realpath -m "${1:?output zip}")"
: "${BRICK_SDK:?Set BRICK_SDK to a private SDK directory}"
REPO="$(cd "$(dirname "$0")/../../.." && pwd)"
cd "$REPO"
NEXTUI=packaging/handheld/nextui
mkdir -p "$BRICK_SDK"

fetch() { # url sha256 destination
    [ -f "$3" ] && echo "$2  $3" | sha256sum -c --quiet && return
    curl -fsSL "$1" -o "$3"
    echo "$2  $3" | sha256sum -c --quiet
}

# 1. Toolchain, vendor SDK and target dependencies (cached between runs by the workflow).
if [ ! -f "$BRICK_SDK/.deps-built" ]; then
    fetch https://github.com/prepkg/gcc-toolchain/releases/download/1.0.0-beta/gcc-aarch64-linux-gnu.tar.gz \
        da5113921eaeba7c9e9e7be104c610790a449ee13e98f09a5a7153ebb5dc739e "$BRICK_SDK/gcc.tar.gz"
    fetch https://github.com/trimui/toolchain_sdk_smartpro/releases/download/20231018/SDK_usr_tg5040_a133p.tgz \
        b6b615d03204e9d9bb1a91c31de0c3402434f6b8fc780743fa88a5f1da6f3c79 "$BRICK_SDK/trimui-sdk.tgz"
    python3 "$NEXTUI/extract-safe.py"
    find "$BRICK_SDK/compiler/aarch64-linux-gnu/libexec" -type f -exec chmod u+x {} +
    bash "$NEXTUI/setup.sh"
    cp "$NEXTUI/target.cmake" "$BRICK_SDK/target.cmake"
    python3 "$NEXTUI/download-deps.py"
    bash "$NEXTUI/build-deps.sh"
    rm -f "$BRICK_SDK/gcc.tar.gz" "$BRICK_SDK/trimui-sdk.tgz"
    touch "$BRICK_SDK/.deps-built"
fi
ROOT="$BRICK_SDK/compiler/aarch64-linux-gnu/aarch64-linux-gnu/sysroot"
BIN="$BRICK_SDK/compiler/aarch64-linux-gnu/bin"

# nlohmann-json is header-only: copy the host's 3.x headers into the private sysroot (BUILD.md).
if [ ! -d "$ROOT/usr/include/nlohmann" ]; then
    for dir in /usr/include/nlohmann /usr/local/include/nlohmann; do
        [ -f "$dir/json_fwd.hpp" ] && cp -r "$dir" "$ROOT/usr/include/" && break
    done
    [ -d "$ROOT/usr/include/nlohmann" ] || { echo "nlohmann-json headers not found on the host" >&2; exit 1; }
fi

# 2. Generated graphics from a native build of this exact source (the target cannot run the
#    sprite generator while cross-compiling).
cmake -S . -B host-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DOPENRCT2_USE_CCACHE=OFF \
    -DDISABLE_NETWORK=ON -DDISABLE_HTTP=ON -DDISABLE_TTF=ON -DDISABLE_DISCORD_RPC=ON -DDISABLE_IPO=ON \
    -DDISABLE_GOOGLE_BENCHMARK=ON
cmake --build host-build --target graphics

# 3. Cross build.
cmake -S . -B brick-build -G Ninja -DCMAKE_TOOLCHAIN_FILE="$BRICK_SDK/target.cmake" \
    -DCMAKE_BUILD_TYPE=Release -DOPENRCT2_USE_CCACHE=OFF -DDISABLE_NETWORK=ON -DDISABLE_HTTP=ON \
    -DDISABLE_TTF=ON -DDISABLE_OPENGL=ON -DDISABLE_DISCORD_RPC=ON -DDISABLE_IPO=ON -DDISABLE_VORBIS=ON \
    -DDISABLE_FLAC=ON -DDISABLE_GOOGLE_BENCHMARK=ON -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_CXX_FLAGS='-Wno-error=stringop-overflow -Wno-error=maybe-uninitialized -Wno-error=null-dereference'
cmake --build brick-build --target openrct2 openrct2-cli
"$BIN/aarch64-linux-gnu-gcc" --sysroot="$ROOT" -mcpu=cortex-a53 "$NEXTUI/input-map.c" \
    -I"$ROOT/usr/include/SDL2" -L"$ROOT/usr/lib" -lSDL2 -o brick-build/input-map
cp host-build/g2.dat host-build/fonts.dat host-build/palettes.dat host-build/tracks.dat brick-build/

# 4. OpenRCT2's own open data (objects, title sequences), pinned by assets.json.
ASSETS="$PWD/ci-assets"
mkdir -p "$ASSETS"
for kind in objects title-sequences; do
    url=$(python3 -c "import json;print(json.load(open('assets.json'))['$kind']['url'])")
    sum=$(python3 -c "import json;print(json.load(open('assets.json'))['$kind']['sha256'])")
    fetch "$url" "$sum" "$ASSETS/$kind.zip"
    rm -rf "${ASSETS:?}/$kind" && mkdir -p "$ASSETS/$kind" && python3 -m zipfile -e "$ASSETS/$kind.zip" "$ASSETS/$kind"
done

# 5. Stage and archive the Pak. No RCT2 data is ever included.
rm -rf ci-stage
python3 "$NEXTUI/package.py" --build brick-build --sysroot "$ROOT" --assets "$ASSETS" --stage "$PWD/ci-stage" \
    --strip "$BIN/aarch64-linux-gnu-strip"
python3 "$NEXTUI/archive-package.py" --stage "$PWD/ci-stage" --output "$OUT"
echo "Built $OUT"
