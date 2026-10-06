#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-only
set -eu
repo="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd -P)"
fixture="$(mktemp -d)"
# Fixture cleanup is scoped to this exact freshly created temporary tree.
trap 'rm -r -- "$fixture"' EXIT
mkdir -p "$fixture/runtime" "$fixture/Ports/.ports/openrct2-handheld/engine/share/openrct2" \
  "$fixture/Ports/.ports/openrct2-handheld/RCT2/Data"
cp "$repo/test/handheld/launcher-control-fixture.sh" "$fixture/runtime/control.txt"
cp "$repo/test/handheld/launcher-game-fixture.sh" "$fixture/Ports/.ports/openrct2-handheld/openrct2"
chmod +x "$fixture/Ports/.ports/openrct2-handheld/openrct2"
cp "$repo/packaging/handheld/config.ini" "$fixture/Ports/.ports/openrct2-handheld/config.ini"
cp "$repo/licence.txt" "$fixture/Ports/.ports/openrct2-handheld/engine/share/openrct2/g2.dat"
cp "$repo/licence.txt" "$fixture/Ports/.ports/openrct2-handheld/RCT2/Data/g1.dat"
cp "$repo/packaging/handheld/OpenRCT2-Handheld.sh" "$fixture/Ports/OpenRCT2-Handheld.sh"
cp "$repo/packaging/handheld/OpenRCT2-Handheld.sh" "$fixture/Ports/.ports/OpenRCT2-Handheld.sh"
export EMU_DIR="$fixture/runtime"
bash "$fixture/Ports/OpenRCT2-Handheld.sh"
cp "$repo/licence.txt" "$fixture/Ports/.ports/openrct2-handheld/save-handheld/config.ini"
bash "$fixture/Ports/.ports/OpenRCT2-Handheld.sh"
cmp "$repo/licence.txt" "$fixture/Ports/.ports/openrct2-handheld/save-handheld/config.ini"
printf 'PASS: both NextUI launcher locations and existing config preservation\n'
