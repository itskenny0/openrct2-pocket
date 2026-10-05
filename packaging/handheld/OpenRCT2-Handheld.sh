#!/usr/bin/env bash
# Staging template, not a verified Brick release. See docs/handheld/BUILDING.md.
set -e
for candidate in /opt/system/Tools/PortMaster /opt/tools/PortMaster \
  "${XDG_DATA_HOME:-$HOME/.local/share}/PortMaster" /roms/ports/PortMaster; do
  if [ -f "$candidate/control.txt" ]; then
    controlfolder="$candidate"
    break
  fi
done
if [ -z "${controlfolder:-}" ]; then
  echo 'PortMaster control.txt not found; launch through your installed PortMaster runtime.' >&2
  exit 1
fi
source "$controlfolder/control.txt"
if [ -f "$controlfolder/mod_${CFW_NAME}.txt" ]; then
  source "$controlfolder/mod_${CFW_NAME}.txt"
fi
if [ -z "${TASKSET+x}" ] && [ -f "$controlfolder/tasksetter" ]; then
  source "$controlfolder/tasksetter"
fi
get_controls
GAMEDIR="/${directory#/}/ports/openrct2-handheld"
cd "$GAMEDIR"
exec > >(tee "$GAMEDIR/log.txt") 2>&1
trap 'pm_finish' EXIT
if [ ! -x ./openrct2 ] || [ ! -f engine/share/openrct2/g2.dat ]; then
  echo 'Missing handheld executable or matching engine data. This template alone is not a game build.'
  exit 1
fi
if [ ! -f RCT2/Data/g1.dat ] && [ ! -f RCT2/data/g1.dat ]; then
  echo 'Copy your original RCT2 installation into RCT2 first (including Data/g1.dat).'
  exit 1
fi
if [ ! -f test.park ]; then
  echo 'Put a disposable copy of a compatible park at openrct2-handheld/test.park.'
  exit 1
fi
mkdir -p save-handheld
if [ ! -f save-handheld/config.ini ]; then
  cp config.ini save-handheld/config.ini
fi
export OPENRCT2_HANDHELD=1
export SDL_GAMECONTROLLERCONFIG="${sdl_controllerconfig:-${SDL_GAMECONTROLLERCONFIG:-}}"
export LD_LIBRARY_PATH="$GAMEDIR/libs${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
# No gptokeyb mouse/keyboard translation: SDL delivers the mapped controller directly.
pm_platform_helper "$GAMEDIR/openrct2"
read -r -a affinity <<< "${TASKSET:-}"
"${affinity[@]}" ./openrct2 test.park --user-data-path=save-handheld \
  --openrct2-data-path=engine/share/openrct2 --rct2-data-path=RCT2
