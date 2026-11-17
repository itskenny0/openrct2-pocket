#!/bin/sh
# NextUI Tools/tg5040/OpenRCT2 Pocket.pak/launch.sh
set -eu
PAK_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)"
# Find the SD card root. NextUI puts Paks in Tools/<platform>/, NX Redux and others directly in
# Tools/, so a fixed number of "../" is wrong on one or the other. Prefer the frontend's
# SDCARD_PATH, then the nearest folder above the Pak that has .userdata, then /mnt/SDCARD.
SD_ROOT="${SDCARD_PATH:-}"
if [ -z "$SD_ROOT" ] || [ ! -d "$SD_ROOT" ]; then
 SD_ROOT=""
 dir="$PAK_DIR"
 while [ "$dir" != "/" ]; do
  dir="$(dirname -- "$dir")"
  if [ -d "$dir/.userdata" ]; then SD_ROOT="$dir"; break; fi
 done
 [ -n "$SD_ROOT" ] || SD_ROOT="/mnt/SDCARD"
fi
USER_DIR="$SD_ROOT/.userdata/tg5040/openrct2-pocket"
mkdir -p "$USER_DIR"
exec >> "$USER_DIR/launch.log" 2>&1
printf '\nOpenRCT2 Pocket launch %s\n' "$(date)"
GAME_DIR="$PAK_DIR/game"
if [ ! -f "$GAME_DIR/openrct2" ] && [ -f "$PAK_DIR/openrct2" ]; then GAME_DIR="$PAK_DIR"; fi
LIB_DIR="$GAME_DIR/lib"
LOADER="$LIB_DIR/ld-linux-aarch64.so.1"
# Retain NextUI/PortMaster GPU paths; private loader --library-path replaces LD_LIBRARY_PATH.
RUNTIME_LIB_PATH="$LIB_DIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}:/usr/trimui/lib:/usr/lib:/lib"
printf 'Runtime library path=%s\n' "$RUNTIME_LIB_PATH"
for driver in /usr/trimui/lib/libEGL.so /usr/trimui/lib/libEGL.so.1 /usr/trimui/lib/libGLESv2.so /usr/trimui/lib/libGLESv2.so.2; do
 [ ! -e "$driver" ] || ls -l "$driver"
done
if [ ! -x "$GAME_DIR/openrct2" ] || [ ! -x "$LOADER" ] || [ ! -f "$GAME_DIR/data/g2.dat" ]; then
 echo 'Incomplete package: executable, loader or engine data missing.';exit 1
fi
# A damaged copy of a bundled library is skipped silently by the loader, which then falls back to
# the device's older one and the game refuses to start. Check the runtime against the Pak's
# checksums so the log names the bad file.
if command -v sha256sum >/dev/null 2>&1 && [ -f "$GAME_DIR/SHA256SUMS.txt" ]; then
 if ! (cd "$GAME_DIR" && grep '  lib/' SHA256SUMS.txt | sha256sum -c >/dev/null 2>&1); then
  echo 'Damaged runtime files (copy the Pak to the SD card again):'
  (cd "$GAME_DIR" && grep '  lib/' SHA256SUMS.txt | sha256sum -c 2>&1 | grep -v ': OK$')
 fi
fi
RCT2_DIR="${OPENRCT2_RCT2_PATH:-$PAK_DIR/RCT2}"
if [ ! -f "$RCT2_DIR/Data/g1.dat" ] && [ ! -f "$RCT2_DIR/data/g1.dat" ]; then
 echo "Original RCT2 assets missing. Copy Data, ObjData and Scenarios into $PAK_DIR/RCT2.";exit 1
fi
if [ ! -f "$USER_DIR/config.ini" ]; then cp "$PAK_DIR/config.ini" "$USER_DIR/config.ini";fi
export OPENRCT2_HANDHELD=1
# These physical mappings already map A/B correctly; do not apply the legacy swap.
unset OPENRCT2_HANDHELD_SWAP_AB
export SDL_RENDER_SCALE_QUALITY=0
# Preserve a supplied mapping override for changed firmware/controllers.
if [ -z "${OPENRCT2_BRICK_KEEP_MAPPING:-}" ]; then
 if mapping="$("$LOADER" --library-path "$RUNTIME_LIB_PATH" "$GAME_DIR/input-map")"; then
  export SDL_GAMECONTROLLERCONFIG="${SDL_GAMECONTROLLERCONFIG:+$SDL_GAMECONTROLLERCONFIG
}$mapping"
 fi
fi
printf 'Platform=%s Device=%s Assets=%s User=%s\n' "${PLATFORM:-unset}" "${DEVICE:-unset}" "$RCT2_DIR" "$USER_DIR"
printf 'SDL video=%s audio=%s\n' "${SDL_VIDEODRIVER:-auto}" "${SDL_AUDIODRIVER:-auto}"
# Some frontends (NX Redux among them) preload their own hooks. Preloads load before the Pak's
# private libraries are searched, so a preload that depends on the system libstdc++ drags in
# that older copy and the game refuses to start. The game gets the Pak's runtime only.
printf 'LD_PRELOAD=%s\n' "${LD_PRELOAD:-unset}"
[ ! -s /etc/ld.so.preload ] || { printf '/etc/ld.so.preload: '; tr '\n' ' ' < /etc/ld.so.preload; echo; }
unset LD_PRELOAD
# libstdc++ ships as libstdcxx.so.6 because some copy tools drop names containing '+'. Preload it
# by path so the game and ICU get the Pak's copy instead of the device's older one.
# (The path is relative to the game folder, set below: LD_PRELOAD splits on spaces, and the Pak's
# own folder name has one. The bundled glibc 2.26 loader predates --preload.)
PRELOAD=""
for cxx in libstdcxx.so.6 libstdc++.so.6; do
 if [ -f "$LIB_DIR/$cxx" ]; then PRELOAD="lib/$cxx"; break; fi
done
[ -n "$PRELOAD" ] || echo 'Warning: the Pak has no libstdc++ (libstdcxx.so.6); copy the Pak to the SD card again.'
cd "$GAME_DIR"
[ -z "$PRELOAD" ] || export LD_PRELOAD="$PRELOAD"
# A private glibc runtime avoids imposing a newer libc on NextUI itself.
# The loader affects this child only. Exiting returns normally to NextUI.
exec "$LOADER" --library-path "$RUNTIME_LIB_PATH" "$GAME_DIR/openrct2" \
 --user-data-path="$USER_DIR" --openrct2-data-path="$GAME_DIR/data" --rct2-data-path="$RCT2_DIR"
