#!/usr/bin/env bash
# Syntax-check Windows-only sources (the D3D12 backend, XAudio2) with clang against mingw-w64 headers, no linking.
#   scripts/wincheck.sh [file.cpp...]   default: sdk/runtime/gpu/d3d12.cpp sdk/runtime/audio/xaudio2.cpp
# The headers are unpacked from Ubuntu's packages into .local/mingw on first use (no root needed).
set -uo pipefail
cd "$(dirname "$0")/.."
M=.local/mingw/root/usr
if [ ! -f $M/x86_64-w64-mingw32/include/d3d12.h ]; then
  mkdir -p .local/mingw && (cd .local/mingw && apt-get download mingw-w64-x86-64-dev mingw-w64-common g++-mingw-w64-x86-64-posix >/dev/null &&
    for d in *.deb; do dpkg -x "$d" root; done) || { echo "cannot fetch mingw-w64 headers"; exit 1; }
fi
[ -d build/linux/_deps/imgui-src ] || { echo "run scripts/linux-tools.sh first (imgui sources)"; exit 1; }
FILES=("$@"); [ ${#FILES[@]} -gt 0 ] || FILES=(sdk/runtime/gpu/d3d12.cpp sdk/runtime/audio/xaudio2.cpp)
rc=0
for f in "${FILES[@]}"; do
  out=$(clang++ -fsyntax-only -std=c++20 --target=x86_64-w64-mingw32 -nostdinc -nostdinc++ \
    -isystem $M/lib/gcc/x86_64-w64-mingw32/13-posix/include/c++ \
    -isystem $M/lib/gcc/x86_64-w64-mingw32/13-posix/include/c++/x86_64-w64-mingw32 \
    -isystem "$(clang -print-resource-dir)/include" -isystem $M/x86_64-w64-mingw32/include \
    -Isdk/runtime -Isrc -Ibuild/linux/_deps/imgui-src -Ibuild/linux/_deps/tomlplusplus-src/include \
    -DWIN32_LEAN_AND_MEAN -DNOMINMAX -Wno-everything "$f" 2>&1 | grep -E "error" | head -20)
  if [ -n "$out" ]; then echo "$out"; rc=1; else echo "ok $f"; fi
done
exit $rc
