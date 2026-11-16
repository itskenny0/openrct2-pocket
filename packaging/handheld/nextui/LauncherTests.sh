#!/bin/bash
set -eu
root=$(mktemp -d)
trap 'rm -rf "$root"' EXIT
pak="$root/Tools/tg5040/OpenRCT2 Pocket.pak"
mkdir -p "$pak/game/lib" "$pak/game/data"
cp "$(dirname "$0")/launch.sh" "$pak/launch.sh"
cp "$(dirname "$0")/config.ini" "$pak/config.ini"
printf 'marker' > "$pak/game/data/g2.dat"
cat > "$pak/game/lib/ld-linux-aarch64.so.1" <<'LOADER'
#!/bin/sh
[ "$1" = --library-path ] || exit 88
shift 2
exec "$@"
LOADER
cat > "$pak/game/input-map" <<'MAPPER'
#!/bin/sh
printf 'runtime-guid,Brick,a:b1,b:b0,\n'
MAPPER
cat > "$pak/game/openrct2" <<'GAME'
#!/bin/sh
printf '%s\n' "$@" > "$TEST_RECORD/arguments"
printf '%s' "${SDL_GAMECONTROLLERCONFIG:-}" > "$TEST_RECORD/mapping"
printf '%s' "${OPENRCT2_HANDHELD_SWAP_AB:-unset}" > "$TEST_RECORD/swap"
exit 7
GAME
chmod +x "$pak/game/"{input-map,openrct2} "$pak/game/lib/ld-linux-aarch64.so.1"
export TEST_RECORD="$root" OPENRCT2_HANDHELD_SWAP_AB=1
if sh "$pak/launch.sh"; then exit 90;fi
[ ! -e "$root/arguments" ]
grep -q 'assets missing' "$root/.userdata/tg5040/openrct2-pocket/launch.log"
mkdir -p "$pak/RCT2/Data"
touch "$pak/RCT2/Data/g1.dat"
set +e
sh "$pak/launch.sh"
code=$?
set -e
[ "$code" = 7 ]
grep -Fq -- "--user-data-path=$root/.userdata/tg5040/openrct2-pocket" "$root/arguments"
grep -Fq -- "--rct2-data-path=$pak/RCT2" "$root/arguments"
grep -q 'runtime-guid' "$root/mapping"
[ "$(cat "$root/swap")" = unset ]
printf 'user preference' > "$root/.userdata/tg5040/openrct2-pocket/config.ini"
sh "$pak/launch.sh" || [ "$?" = 7 ]
[ "$(cat "$root/.userdata/tg5040/openrct2-pocket/config.ini")" = 'user preference' ]
echo 'NextUI launcher: paths with spaces, missing assets, mapping, persistent config, exit status PASS'
