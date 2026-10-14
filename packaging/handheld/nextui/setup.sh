#!/bin/bash
set -eu
TASK="${BRICK_SDK:?Set BRICK_SDK to your private SDK directory}"
ROOT=$TASK/compiler/aarch64-linux-gnu/aarch64-linux-gnu/sysroot
mkdir -p "$ROOT/usr/include/SDL2" "$ROOT/usr/lib/pkgconfig"
cp -a "$TASK/trimui/usr/include/SDL2/." "$ROOT/usr/include/SDL2/"
for f in zlib.h zconf.h png.h pngconf.h; do cp "$TASK/trimui/usr/include/$f" "$ROOT/usr/include/"; done
for f in libSDL2-2.0.so.0.2600.1 libpng12.so.0.56.0 libz.so.1.2.8; do cp "$TASK/trimui/usr/lib/$f" "$ROOT/usr/lib/"; done
ln -sf libSDL2-2.0.so.0.2600.1 "$ROOT/usr/lib/libSDL2.so"
ln -sf libSDL2-2.0.so.0.2600.1 "$ROOT/usr/lib/libSDL2-2.0.so.0"
ln -sf libpng12.so.0.56.0 "$ROOT/usr/lib/libpng12.so"
ln -sf libpng12.so.0.56.0 "$ROOT/usr/lib/libpng12.so.0"
ln -sf libz.so.1.2.8 "$ROOT/usr/lib/libz.so"
ln -sf libz.so.1.2.8 "$ROOT/usr/lib/libz.so.1"
for f in sdl2.pc libpng12.pc zlib.pc; do cp "$TASK/trimui/usr/lib/pkgconfig/$f" "$ROOT/usr/lib/pkgconfig/"; done

# Keep SDL's dynamic ALSA load inside the compatible private runtime.
cp "$TASK/trimui/usr/lib/libasound.so.2.0.0" "$ROOT/usr/lib/"
ln -sf libasound.so.2.0.0 "$ROOT/usr/lib/libasound.so.2"
