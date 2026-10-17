#!/usr/bin/env bash
# Build the headless harness and run it.
#   scripts/harness.sh [seconds] [pad script] [extra env, e.g. REF=60 CAP=300]
# Logs: .local/run/1/logs, stdout: .local/run/1/run.out, frames: .local/run/1/cap -> captures/*.png
set -euo pipefail
cd "$(dirname "$0")/.."
SECS=${1:-60}; PAD=${2:-scripts/pad/boot.txt}; shift $(( $# > 2 ? 2 : $# )) || true
CXX="$(command -v ccache >/dev/null && echo 'ccache clang++' || echo clang++)" sdk/harness/build.sh -r . -g generated/game -o .local/harness -n game
rm -rf .local/run
env "$@" sdk/harness/run.sh -x .local/harness/game -g assets -o .local/run -t "$SECS" -s "$PAD"
shopt -s nullglob
for f in .local/run/*/cap/*.tga .local/run/*/*.tga; do
  convert "$f" "captures/$(date +%m%d-%H%M)-$(basename "${f%.tga}").png"
done
echo "logs: .local/run/1/logs   captures: captures/"
