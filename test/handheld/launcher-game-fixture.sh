#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-only
set -eu
test "$OPENRCT2_HANDHELD" = 1
test "$OPENRCT2_HANDHELD_SWAP_AB" = 1
test "$1" = --user-data-path=save-handheld
test "$2" = --openrct2-data-path=engine/share/openrct2
test "$3" = "--rct2-data-path=$PWD/RCT2"
test -f save-handheld/config.ini
printf 'PASS: launcher environment and title-screen arguments\n'
