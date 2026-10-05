#!/usr/bin/env bash
set -euo pipefail
project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
build_dir="${1:-$project_root/build-handheld}"
cmake -S "$project_root" -B "$build_dir" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DDISABLE_DISCORD_RPC=ON -DDISABLE_IPO=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build "$build_dir" --parallel "${BUILD_JOBS:-4}"
cmake -S "$project_root/test/handheld" -B "$build_dir/input-tests" -G Ninja
cmake --build "$build_dir/input-tests" --parallel "${BUILD_JOBS:-4}"
ctest --test-dir "$build_dir/input-tests" --output-on-failure
