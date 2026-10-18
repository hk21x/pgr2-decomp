#!/usr/bin/env bash
# Smoke test: boot headless, play scripts/pad/race.txt (title -> Instant Action -> race, accelerate) and fail unless
#   - the harness builds and runs, with no trapping stub, unimplemented kernel export or guest trap logged,
#   - the title menu is up before the menu choice (vertical blank 6060 of race.txt: under 100 draws a frame),
#   - the race draws: the last 300 frames average more than 100 draws a frame, and frames are still presented in
#     the last 20 seconds.
# Run before every fast-forward of main.   scripts/smoke.sh [seconds, default 240]
# KEEP_HDD=1 reuses the last run's hard disk (a normal boot); by default it is a first boot (format, cache copy).
set -uo pipefail
cd "$(dirname "$0")/.."
SECS=${1:-240}
PAD=${PAD:-scripts/pad/race.txt}  # another pad script checks that the test fails without the race
LOG=.local/run/1/logs/pgr2_001.log
fail() { echo "SMOKE FAIL: $*"; exit 1; }

scripts/harness.sh "$SECS" "$PAD" > .local/smoke.out 2>&1 || fail "harness did not build or run (.local/smoke.out)"
[ -f "$LOG" ] || fail "no log at $LOG"

traps=$(grep -E 'guest trap|is not implemented|trapping stub|unknown code' "$LOG" | head -5)
[ -z "$traps" ] || fail "traps:"$'\n'"$traps"

# draws a frame between vertical blanks a and b, from the log's every-60-frames lines
window() { grep 'renderer: frame' "$LOG" | awk -v a="$1" -v b="$2" '{
    match($0, /frame [0-9]+/); f = substr($0, RSTART + 6, RLENGTH - 6) + 0
    match($0, /[0-9]+ draws/); d = substr($0, RSTART, RLENGTH - 6) + 0
    match($0, /vblank [0-9]+/); v = substr($0, RSTART + 7, RLENGTH - 7) + 0
    if (v >= a && v <= b) { if (!n++) { f0 = f; d0 = d } f1 = f; d1 = d } }
    END { print (f1 > f0) ? int((d1 - d0) / (f1 - f0)) : -1 }'; }
# the title menu just before the choice at vertical blank 6060 draws little; a 3D scene would mean the presses
# missed it
menu=$(window 5260 5860)
[ "$menu" -ge 0 ] && [ "$menu" -lt 100 ] || fail "not on the title menu before the choice: $menu draws a frame (vertical blanks 5260-5860)"

# draws per frame over the last 300 frames
read -r frames draws < <(grep 'renderer: frame' "$LOG" | tail -6 | awk '{
    match($0, /frame [0-9]+/); f = substr($0, RSTART + 6, RLENGTH - 6) + 0
    match($0, /[0-9]+ draws/); d = substr($0, RSTART, RLENGTH - 6) + 0
    if (NR == 1) { f0 = f; d0 = d } f1 = f; d1 = d } END { print f1 - f0, d1 - d0 }')
[ "${frames:-0}" -gt 0 ] || fail "no frames presented"
# still presenting at the end: a crash after the race started fails too
last=$(grep 'renderer: frame' "$LOG" | tail -1 | awk '{ t = $2; sub(/\]/, "", t); print int(t) }')
[ "${last:-0}" -ge $(( SECS - 20 )) ] || fail "frames stopped at ${last}s of ${SECS}s"
per=$(( draws / frames ))
[ "$per" -gt 100 ] || fail "race not drawing: $per draws a frame over the last $frames frames"

echo "SMOKE OK: menu $menu draws a frame, race $per draws a frame over the last $frames frames, no traps"
