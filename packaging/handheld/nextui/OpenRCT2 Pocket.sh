#!/bin/sh
# Launch via installed NextUI PortMaster while reusing the existing testing package.
set -eu
SD_ROOT="${SDCARD_PATH:-/mnt/SDCARD}"
GAME_LAUNCH="$SD_ROOT/Tools/tg5040/OpenRCT2 Pocket.pak/launch.sh"
if [ ! -f "$GAME_LAUNCH" ]; then
 echo "Existing OpenRCT2 Pocket testing package not found: $GAME_LAUNCH" >&2
 exit 1
fi
# The NextUI PORTS.pak wrapper supplies its graphics/audio/runtime environment.
exec sh "$GAME_LAUNCH"
