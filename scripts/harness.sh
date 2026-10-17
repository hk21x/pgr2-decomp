#!/usr/bin/env bash
# Build the headless harness and run it.
#   scripts/harness.sh [seconds] [pad script] [extra env, e.g. REF=600 CAP=300]
#   KEEP_HDD=1 scripts/harness.sh ...   keeps .local/run/*/hdd from the last run (a normal boot after the first one);
#   without it every run starts with an empty hard disk (the first boot, cache partitions formatted)
# Logs: .local/run/1/logs, stdout: .local/run/1/run.out, frames: .local/run/1/cap -> captures/*.png
set -euo pipefail
cd "$(dirname "$0")/.."
SECS=${1:-60}; PAD=${2:-scripts/pad/boot.txt}; shift $(( $# > 2 ? 2 : $# )) || true
CXX="$(command -v ccache >/dev/null && echo 'ccache clang++' || echo clang++)" bash sdk/harness/build.sh -r . -g generated/game -o .local/harness -n game
if [ "${KEEP_HDD:-0}" = 1 ]; then
  find .local/run -mindepth 2 -maxdepth 2 ! -name hdd -exec rm -rf {} + 2>/dev/null || true
else
  rm -rf .local/run
fi
env "$@" bash sdk/harness/run.sh -x .local/harness/game -g assets -o .local/run -t "$SECS" -s "$PAD"
shopt -s nullglob
for f in .local/run/*/cap/*.tga .local/run/*/*.tga; do
  convert "$f" "captures/$(date +%m%d-%H%M)-$(basename "${f%.tga}").png" || echo "cannot convert $f"
done
echo "logs: .local/run/1/logs   captures: captures/"
