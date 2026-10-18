// Hooks for default.xbe. A strong definition replaces the recompiled function at that address, for example:
//
// #include "xdk/hle.h"
//
// X86_EXTERN_FUNC(__imp__sub_00012345);
// X86_FUNC(sub_00012345) {
//     __imp__sub_00012345(ctx, base);  // the original
// }
#include <atomic>
#include <mutex>
#include <string>
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

// PGR2_LIGHT also tallies the flag byte (+0xB) of every distinct track triangle the ground query (0xC6210, ecx the
// grid cell: +8 count, records of 16 bytes from +0x18) looks at, logged every 2000 queries
X86_EXTERN_FUNC(__imp__sub_000C6210);
X86_FUNC(sub_000C6210) {
    static const bool on = std::getenv("PGR2_LIGHT") != nullptr;
    if (on) {
        static std::mutex m;
        static std::unordered_map<uint32_t, uint8_t> seen;
        static uint32_t calls = 0, hist[256] = {};
        uint32_t cell = ctx.ecx, n = kernel::R32(cell + 8);
        std::lock_guard lock(m);
        for (uint32_t i = 0; i < n && i < 4096; i++) {
            uint32_t e = cell + 0x18 + i * 0x10;
            uint8_t f = kernel::G<uint8_t>(e + 0xB);
            if (seen.emplace(e, f).second) {
                hist[f]++;
                if (seen.size() % 1500 == 1)
                    LOGR("pgr2 light: triangle at %08X: %08X %08X %08X %08X", e, kernel::R32(e), kernel::R32(e + 4),
                         kernel::R32(e + 8), kernel::R32(e + 12));
            }
        }
        if (++calls % 2000 == 0) {
            uint32_t bit7 = 0;
            std::string top;
            for (int b = 0; b < 256; b++) {
                if (b & 0x80) bit7 += hist[b];
                if (hist[b]) top += " " + std::to_string(b) + ":" + std::to_string(hist[b]);
            }
            LOGR("pgr2 light: %zu triangles queried, %u with bit 7; flag bytes%s", seen.size(), bit7, top.c_str());
        }
    }
    __imp__sub_000C6210(ctx, base);
}

// PGR2_LIGHT: each wheel's ground query (0x24DE0, ecx the wheel): its position (+0x40), the height found (+4), the
// flag (+0xD8) and colour (+0xDC), the first 200
X86_EXTERN_FUNC(__imp__sub_00024DE0);
X86_FUNC(sub_00024DE0) {
    uint32_t w = ctx.ecx;
    __imp__sub_00024DE0(ctx, base);
    static const bool on = std::getenv("PGR2_LIGHT") != nullptr;
    static std::atomic<int> n{0};
    if (on && n++ < 200)
        LOGR("pgr2 wheel %08X at %g %g %g ground %g flag %u colour %08X", w, F(w + 0x40), F(w + 0x44), F(w + 0x48), F(w + 4),
             kernel::G<uint8_t>(w + 0xD8), kernel::R32(w + 0xDC));
}
