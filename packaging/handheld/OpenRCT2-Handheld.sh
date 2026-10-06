#!/bin/bash
# PortMaster / NextUI launcher for the Phase 1 Brick hardware proof.
set -e
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)"
for candidate in "${controlfolder:-}" "${EMU_DIR:-}" \
  /opt/system/Tools/PortMaster /opt/tools/PortMaster \
  "${XDG_DATA_HOME:-$HOME/.local/share}/PortMaster" /roms/ports/PortMaster; do
  if [ -n "$candidate" ] && [ -f "$candidate/control.txt" ]; then
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
if [ -d "$SCRIPT_DIR/.ports/openrct2-handheld" ]; then
  GAMEDIR="$SCRIPT_DIR/.ports/openrct2-handheld"
elif [ -d "$SCRIPT_DIR/openrct2-handheld" ]; then
  GAMEDIR="$SCRIPT_DIR/openrct2-handheld"
else
  GAMEDIR="/${directory#/}/ports/openrct2-handheld"
fi
cd "$GAMEDIR"
: > "$GAMEDIR/log.txt"
exec > >(tee "$GAMEDIR/log.txt") 2>&1
trap 'pm_finish' EXIT
if [ ! -x ./openrct2 ] || [ ! -f engine/share/openrct2/g2.dat ]; then
  pm_message 'Missing handheld executable or matching engine data.'
  exit 1
fi

RCT2_PATH="$GAMEDIR/RCT2"
if [ ! -f "$RCT2_PATH/Data/g1.dat" ] && [ ! -f "$RCT2_PATH/data/g1.dat" ]; then
  if [ -d "$SCRIPT_DIR/.ports/openrct2/RCT2" ]; then
    SIBLING_RCT2="$SCRIPT_DIR/.ports/openrct2/RCT2"
  else
    SIBLING_RCT2="/${directory#/}/ports/openrct2/RCT2"
  fi
  if [ -f "$SIBLING_RCT2/Data/g1.dat" ] || [ -f "$SIBLING_RCT2/data/g1.dat" ]; then
    RCT2_PATH="$SIBLING_RCT2"
  else
    pm_message 'Copy your original RCT2 installation into openrct2-handheld/RCT2.'
    exit 1
  fi
fi

RCT1_PATH="$GAMEDIR/RCT1"
if [ ! -d "$RCT1_PATH" ] && [ -d "/${directory#/}/ports/openrct2/RCT1" ]; then
  RCT1_PATH="/${directory#/}/ports/openrct2/RCT1"
fi

mkdir -p save-handheld
# Preserve user preferences and saves across updates.
if [ ! -f save-handheld/config.ini ]; then
  cp config.ini save-handheld/config.ini
fi
export OPENRCT2_HANDHELD=1
# NextUI exposes the physical A/B buttons in the opposite SDL order.
export OPENRCT2_HANDHELD_SWAP_AB=1
export SDL_GAMECONTROLLERCONFIG="${sdl_controllerconfig:-${SDL_GAMECONTROLLERCONFIG:-}}"
export LD_LIBRARY_PATH="$GAMEDIR/libs${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export TEXTINPUTPRESET="Name"
export TEXTINPUTINTERACTIVE="Y"
export TEXTINPUTNOAUTOCAPITALS="Y"
# No gptokeyb mouse/keyboard translation: SDL delivers the mapped controller directly.
pm_platform_helper "$GAMEDIR/openrct2"
read -r -a affinity <<< "${TASKSET:-}"
"${affinity[@]}" ./openrct2 --user-data-path=save-handheld \
  --openrct2-data-path=engine/share/openrct2 --rct2-data-path="$RCT2_PATH" \
  --rct1-data-path="$RCT1_PATH"
