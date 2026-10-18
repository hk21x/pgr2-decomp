// Hooks for default.xbe. A strong definition replaces the recompiled function at that address, for example:
//
// #include "xdk/hle.h"
//
// X86_EXTERN_FUNC(__imp__sub_00012345);
// X86_FUNC(sub_00012345) {
//     __imp__sub_00012345(ctx, base);  // the original
// }
#include <mutex>
#include <unordered_map>
#include <cstdlib>
#include <cstring>

#include "core/log.h"
#include "kernel/kernel.h"
#include "xdk/hle.h"

namespace {

// PGR2_LIGHT set: each car's lighting inputs whenever its wheel flags, the sun colour or its brightness (in 1/20s)
// change, the first 400 changes; to find why the car's light colour (vertex constant c56) is 0 on Windows
float F(uint32_t va) {
    uint32_t v = kernel::R32(va);
    float f;
    std::memcpy(&f, &v, 4);
    return f;
}

void LogCarLight(const char* where, uint32_t car) {
    static const bool on = std::getenv("PGR2_LIGHT") != nullptr;
    static std::unordered_map<uint32_t, uint64_t> last;  // per car: flags, sun, brightness in 1/20s
    static std::mutex m;
    if (!on || !car) return;
    uint64_t now = uint64_t(kernel::R32(car + 0x5B0) & 0xFF) | uint64_t(kernel::G<uint8_t>(car + 0x754)) << 8 |
                   uint64_t(kernel::G<uint8_t>(car + 0x8F8)) << 16 | uint64_t(kernel::G<uint8_t>(car + 0xA9C)) << 24 |
                   uint64_t(kernel::R32(0x49EEA8)) << 32;
    now ^= uint64_t(int(F(car + 0x150C) * 20)) * 0x9E3779B97F4A7C15ull;
    {
        std::lock_guard lock(m);
        auto [it, added] = last.try_emplace(car, now);
        if (!added && it->second == now) return;
        it->second = now;
        static int n = 0;
        if (n++ >= 400) return;
    }
    // brightness +0x150C is 0.25 per wheel whose surface flag (+0x5B0, +0x754, +0x8F8, +0xA9C) is clear, eased in
    // (unless 0x3B1EC7 is set and the mode 0x368928 is not 1 or 6: then 1); the sun colour is the D3DCOLOR 0x49EEA8
    LOGR("pgr2 light %s car %08X brightness %g sun %08X wheel flags %u %u %u %u colours %08X %08X %08X %08X "
         "+cc %g +d0 %g 3B1EC7 %u 368928 %u 49E110 %u",
         where, car, F(car + 0x150C), kernel::R32(0x49EEA8), kernel::G<uint8_t>(car + 0x5B0), kernel::G<uint8_t>(car + 0x754),
         kernel::G<uint8_t>(car + 0x8F8), kernel::G<uint8_t>(car + 0xA9C), kernel::R32(car + 0x5B4), kernel::R32(car + 0x758),
         kernel::R32(car + 0x8FC), kernel::R32(car + 0xAA0), F(car + 0xCC), F(car + 0xD0), kernel::G<uint8_t>(0x3B1EC7),
         kernel::R32(0x368928), kernel::G<uint8_t>(0x49E110));
}

}  // namespace

// the car's lighting from its wheels' surfaces, at the start (0x1CDD0) and each frame (0x20690); ecx the car
X86_EXTERN_FUNC(__imp__sub_0001CDD0);
X86_FUNC(sub_0001CDD0) {
    uint32_t car = ctx.ecx;
    __imp__sub_0001CDD0(ctx, base);
    LogCarLight("start", car);
}

X86_EXTERN_FUNC(__imp__sub_00020690);
X86_FUNC(sub_00020690) {
    uint32_t car = ctx.ecx;
    __imp__sub_00020690(ctx, base);
    LogCarLight("frame", car);
}
