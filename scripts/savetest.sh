#!/usr/bin/env bash
# Save test, headless, two boots (about 8 minutes):
#   1. a first boot plays scripts/pad/save1.txt: creates the profile "Player", sets Speedometer to KM/H, backs out
#      (the game saves the profile). Checks E:\UDATA\4d53004b\<profile>\Profile.Sav exists with KM/H stored
#      (byte 0x844: 1 MPH, 0 KM/H).
#   2. a normal boot of the same disk plays scripts/pad/save2.txt: the title offers the profile, Game Options open.
#      Checks the game read Profile.Sav back, the stored setting is still KM/H, and snaps the options screen to
#      captures/<date>-savetest.png (Speedometer must read KM/H).
set -uo pipefail
cd "$(dirname "$0")/.."
R=.local/run/1
LOG=$R/logs/pgr2_001.log
fail() { echo "SAVETEST FAIL: $*"; exit 1; }
profile() { ls $R/hdd/E/UDATA/4d53004b/*/Profile.Sav 2>/dev/null | head -1; }
speedo() { xxd -s 0x844 -l 1 -p "$1"; }

scripts/harness.sh 260 scripts/pad/save1.txt > .local/savetest1.out 2>&1 || fail "run 1 did not build or run"
p=$(profile)
[ -n "$p" ] || fail "run 1 saved no profile"
[ "$(speedo "$p")" = "00" ] || fail "run 1: Profile.Sav holds speedometer $(speedo "$p"), not KM/H (00)"
echo "run 1: $p saved, KM/H stored"
sum1=$(md5sum < "$p")

rm -f $LOG  # run 1's frames would end the wait below at once
KEEP_HDD=1 scripts/harness.sh 200 scripts/pad/save2.txt > .local/savetest2.out 2>&1 &
# the options screen is up from about vertical blank 6560
for i in $(seq 150); do
  f=$(grep -o "vblank [0-9]*" $LOG 2>/dev/null | tail -1 | awk '{print $2}')
  [ "${f:-0}" -ge 6700 ] && break
  timeout 2 tail -f /dev/null
done
rm -f $R/snap_1.tga $R/snap_2.tga
touch $R/snap
for i in $(seq 60); do [ -f $R/snap_2.tga ] && break; timeout 2 tail -f /dev/null; done
shot=captures/$(date +%m%d-%H%M)-savetest.png
[ -f $R/snap_2.tga ] && convert $R/snap_2.tga "$shot"
wait
grep -q "Profile.Sav" $LOG || fail "run 2: Profile.Sav never read"
p=$(profile)
[ "$(speedo "$p")" = "00" ] || fail "run 2: Profile.Sav now holds speedometer $(speedo "$p")"
[ "$(md5sum < "$p")" = "$sum1" ] && same=" (unchanged)" || same=" (saved again)"
echo "SAVETEST OK: the profile and KM/H survived a reboot$same; the options screen: $shot"
