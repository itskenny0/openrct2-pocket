#!/bin/bash
set -eu
TASK="${BRICK_SDK:?Set BRICK_SDK to your private SDK directory}"
CMAKE="${CMAKE:-cmake}"
NINJA="${NINJA:-ninja}"
ROOT=$TASK/compiler/aarch64-linux-gnu/aarch64-linux-gnu/sysroot

"$CMAKE" -S "$TASK/deps/zstd-1.5.7/build/cmake" -B "$TASK/zstd-build" -G Ninja -DCMAKE_MAKE_PROGRAM="$NINJA" -DCMAKE_TOOLCHAIN_FILE="$TASK/target.cmake" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DZSTD_BUILD_PROGRAMS=OFF -DZSTD_BUILD_SHARED=ON -DZSTD_BUILD_STATIC=OFF
"$CMAKE" --build "$TASK/zstd-build" -j6
DESTDIR="$ROOT" "$CMAKE" --install "$TASK/zstd-build"
"$CMAKE" -S "$TASK/deps/libzip-1.11.3" -B "$TASK/zip-build" -G Ninja -DCMAKE_MAKE_PROGRAM="$NINJA" -DCMAKE_TOOLCHAIN_FILE="$TASK/target.cmake" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_TOOLS=OFF -DBUILD_REGRESS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_DOC=OFF -DENABLE_OPENSSL=OFF -DENABLE_GNUTLS=OFF -DENABLE_MBEDTLS=OFF -DENABLE_BZIP2=OFF -DENABLE_LZMA=OFF
"$CMAKE" --build "$TASK/zip-build" -j6
DESTDIR="$ROOT" "$CMAKE" --install "$TASK/zip-build"
mkdir -p "$TASK/icu-host" "$TASK/icu-target"
cd "$TASK/icu-host"
"$TASK/deps/icu/source/configure" --disable-tests --disable-samples --disable-extras --disable-layout
make -j6
cd "$TASK/icu-target"
CC="$TASK/compiler/aarch64-linux-gnu/bin/aarch64-linux-gnu-gcc" CXX="$TASK/compiler/aarch64-linux-gnu/bin/aarch64-linux-gnu-g++" CFLAGS='-O2 -mcpu=cortex-a53' CXXFLAGS='-O2 -mcpu=cortex-a53' "$TASK/deps/icu/source/configure" --host=aarch64-linux-gnu --with-cross-build="$TASK/icu-host" --prefix=/usr --disable-tests --disable-samples --disable-extras --disable-layout
make -j6
make DESTDIR="$ROOT" install
