#!/usr/bin/env bash
# Syntax-check Windows-only sources (the D3D12 backend, XAudio2) with clang against mingw-w64 headers, no linking.
#   scripts/wincheck.sh [file.cpp...]   default: every sdk/runtime source that includes windows.h
# Defines only what the Windows build does (UNICODE, _UNICODE, _CRT_SECURE_NO_WARNINGS): each file must define
# NOMINMAX and WIN32_LEAN_AND_MEAN itself before windows.h, as the real build sees it
# The headers are unpacked from Ubuntu's packages into .local/mingw on first use (no root needed).
set -uo pipefail
cd "$(dirname "$0")/.."
M=.local/mingw/root/usr
if [ ! -f $M/x86_64-w64-mingw32/include/d3d12.h ]; then
  mkdir -p .local/mingw && (cd .local/mingw && apt-get download mingw-w64-x86-64-dev mingw-w64-common g++-mingw-w64-x86-64-posix >/dev/null &&
    for d in *.deb; do dpkg -x "$d" root; done) || { echo "cannot fetch mingw-w64 headers"; exit 1; }
fi
[ -d build/linux/_deps/imgui-src ] || { echo "run scripts/linux-tools.sh first (imgui sources)"; exit 1; }
FILES=("$@"); [ ${#FILES[@]} -gt 0 ] || mapfile -t FILES < <(grep -l "windows.h" sdk/runtime/*/*.cpp | sort)
rc=0
for f in "${FILES[@]}"; do
  # mingw's headers leave min/max alone in C++, the Windows SDK's do not: windows.h needs NOMINMAX before it
  if grep -q "^#include <windows.h>" "$f" &&
     ! awk '/^#include <windows.h>/{exit} /define NOMINMAX/{found=1} END{exit !found}' "$f"; then
    echo "$f: windows.h without NOMINMAX defined before it (min/max macros break the build on Windows)"; rc=1; continue
  fi
  out=$(clang++ -fsyntax-only -std=c++20 --target=x86_64-w64-mingw32 -nostdinc -nostdinc++ \
    -isystem $M/lib/gcc/x86_64-w64-mingw32/13-posix/include/c++ \
    -isystem $M/lib/gcc/x86_64-w64-mingw32/13-posix/include/c++/x86_64-w64-mingw32 \
    -isystem "$(clang -print-resource-dir)/include" -isystem $M/x86_64-w64-mingw32/include \
    -Isdk/runtime -Isrc -Igenerated/game -Ibuild/linux/_deps/imgui-src -Ibuild/linux/_deps/tomlplusplus-src/include -Ibuild/linux/_deps/sdl3-src/include \
    -DUNICODE -D_UNICODE -D_CRT_SECURE_NO_WARNINGS -Wno-everything "$f" 2>&1 | grep -E "error" | head -20)
  if [ -n "$out" ]; then echo "$out"; rc=1; else echo "ok $f"; fi
done
exit $rc
