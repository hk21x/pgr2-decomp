// What a steering wheel's force feedback and an H-pattern shifter follow (GameInfo::wheel, sdk core/wheel.cpp), from
// the race state mapped for the achievements (ACHIEVEMENTS.md): player 1's car 0x49E258, racing when 0x38384C is 0
// (not the attract demo), the race type 0x383814 is set and the car's lap (int16 at 0x39B334 + car x 0x2498) is past
// the start; its speed the float at 0x298D50 + car x 0x6990, m/s.
// The gear: the car's gearbox is at 0x29DAB0 + car x 0x6990 (the car record 0x2973B0 + car x 0x6990 at its +0x24), its
// gear the signed byte at +0x1A (-1 reverse, 1 first; the speedometer 0xE5CF0 shows it). It changes through 0x19EB0 (up)
// and 0x19F60 (down), from 0x1A7B0: the automatic logic (0x1A580), or in manual the car's requests +0x1BC4 / +0x1BC5,
// which the pad's B (up) and X (down) set (measured headless, 2026-10-22). The gearbox is automatic when 0x16520 says so:
// the car's mode +0x19C8 is 1 or 2, or its player's (+0x19CC) transmission, the dword 0x39B31C + player x 0x2498, is 1
// or 2, or the byte 0x39B320 + player x 0x2498 is set. The gear is told only in manual: B and X are not shifts otherwise.
#include <cmath>
#include <cstring>

#include "core/input.h"
#include "kernel/kernel.h"

namespace pgr2 {

void WheelFeel(input::WheelFeel& feel) {
    uint32_t car = kernel::R32(0x49E258);
    if (car > 7) return;
    int16_t lap = int16_t(kernel::R32(0x39B334 + car * 0x2498) & 0xFFFF);
    feel.racing = kernel::G<uint8_t>(0x38384C) == 0 && kernel::R32(0x383814) > 0 && lap >= 0;
    uint32_t bits = kernel::R32(0x298D50 + car * 0x6990);
    float speed;
    std::memcpy(&speed, &bits, 4);
    feel.speed = std::isfinite(speed) ? speed : 0;
    uint32_t record = 0x2973B0 + car * 0x6990;
    uint32_t mode = kernel::R32(record + 0x19C8), player = kernel::R32(record + 0x19CC);
    if (!feel.racing || player > 3) return;
    uint32_t transmission = kernel::R32(0x39B31C + player * 0x2498);
    bool automatic = mode == 1 || mode == 2 || kernel::G<uint8_t>(0x39B320 + player * 0x2498) != 0 ||
                     transmission == 1 || transmission == 2;
    if (automatic) return;
    feel.gear = int(int8_t(kernel::G<uint8_t>(0x29DACA + car * 0x6990)));
    feel.shift_up = input::kB;
    feel.shift_down = input::kX;
}

}  // namespace pgr2
