#!/usr/bin/env bash
# Configure for Linux and build the recompiler tools. The game exe itself only builds on Windows.
# SDL3 is configured without X11/Wayland so it works on a headless VM.
set -euo pipefail
cd "$(dirname "$0")/.."
[ -f assets/default.xbe ] || { echo "assets/default.xbe missing: extract the game first" >&2; exit 1; }
LAUNCH=(); command -v ccache >/dev/null && LAUNCH=(-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache)
cmake -S . -B build/linux -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ "${LAUNCH[@]}" \
  -DSDL_UNIX_CONSOLE_BUILD=ON -DSDL_X11=OFF -DSDL_WAYLAND=OFF
cmake --build build/linux --target xbe_symbols xbe_recomp
