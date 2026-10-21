// What a steering wheel's force feedback follows (GameInfo::wheel, sdk core/wheel.cpp), from the race state mapped for
// the achievements (ACHIEVEMENTS.md): player 1's car 0x49E258, racing when 0x38384C is 0 (not the attract demo), the race
// type 0x383814 is set and the car's lap (int16 at 0x39B334 + car x 0x2498) is past the start; its speed the float at
// 0x298D50 + car x 0x6990, m/s
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
}

}  // namespace pgr2
