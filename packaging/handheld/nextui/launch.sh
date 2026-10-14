#!/bin/sh
# NextUI Tools/tg5040/OpenRCT2 Pocket.pak/launch.sh
set -eu
PAK_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)"
SD_ROOT="$(CDPATH= cd -- "$PAK_DIR/../../.." && pwd -P)"
USER_DIR="$SD_ROOT/.userdata/tg5040/openrct2-pocket"
mkdir -p "$USER_DIR"
exec >> "$USER_DIR/launch.log" 2>&1
printf '\nOpenRCT2 Pocket launch %s\n' "$(date)"
GAME_DIR="$PAK_DIR/game"
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
  export SDL_GAMECONTROLLERCONFIG="$mapping${SDL_GAMECONTROLLERCONFIG:+
$SDL_GAMECONTROLLERCONFIG}"
 fi
fi
printf 'Platform=%s Device=%s Assets=%s User=%s\n' "${PLATFORM:-unset}" "${DEVICE:-unset}" "$RCT2_DIR" "$USER_DIR"
printf 'SDL video=%s audio=%s\n' "${SDL_VIDEODRIVER:-auto}" "${SDL_AUDIODRIVER:-auto}"
cd "$GAME_DIR"
# A private glibc runtime avoids imposing a newer libc on NextUI itself.
# The loader affects this child only. Exiting returns normally to NextUI.
exec "$LOADER" --library-path "$RUNTIME_LIB_PATH" "$GAME_DIR/openrct2" \
 --user-data-path="$USER_DIR" --openrct2-data-path="$GAME_DIR/data" --rct2-data-path="$RCT2_DIR"
