#!/usr/bin/env bash
# System link test: two headless instances on loopback (127.0.0.1 and .2), scripts/pad/link_host.txt and
# link_join.txt. Fails unless both load the race's city after the lobby and neither session ends after the join.
#   scripts/linktest.sh [seconds, default 340]
set -uo pipefail
cd "$(dirname "$0")/.."
SECS=${1:-340}
OUT=.local/link
fail() { echo "LINKTEST FAIL: $*"; exit 1; }
scripts/harness.sh 1 scripts/pad/none.txt > /dev/null 2>&1 || fail "harness did not build"
rm -rf $OUT
XNET_TRACE=1 bash sdk/harness/run.sh -x .local/harness/game -g assets -o $OUT -t "$SECS" \
    -s scripts/pad/link_host.txt -s scripts/pad/link_join.txt > .local/linktest.out 2>&1
for k in 1 2; do
  L=$(ls $OUT/$k/logs/*.log)
  # the attract demo loads a city too: the race is the second load
  [ "$(grep -c 'area loads' "$L")" -ge 2 ] || fail "instance $k never reached the race"
  joined=$(grep -n "XNetRegisterKey" "$L" | head -1 | cut -d: -f1)
  [ -n "$joined" ] || fail "instance $k never joined"
  tail -n +"$joined" "$L" | grep -q "link session .* ends" && fail "instance $k lost the session after joining"
  grep -qE 'guest trap|is not implemented|trapping stub' "$L" && fail "instance $k trapped"
done
echo "LINKTEST OK: host and joiner raced each other"
