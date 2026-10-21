// PGR2's draws the renderer changes (gpu::g_draw_filter)
#include <atomic>
#include <cstdlib>

#include "core/log.h"
#include "gpu/state.h"
#include "gpu/texture.h"

namespace pgr2 {
extern bool dlc;  // src/common/game_info.cpp
}

namespace {

// The car's shadow: a ground mesh around the car that the game builds each frame, drawn after the scene with the car's
// shadow map (a depth texture compared in stage 0), a polygon offset, no depth writes and a multiply blend. The mesh
// does not lie on the visible road and reaches behind the camera; on D3D12 parts of it failed the depth test against
// the road, changing as the camera moved: the shadow and the sunlit road around the car flickered. Without the depth
// test it was steady but showed through the car. Measured headless (which shader's depth stopped each failed pixel,
// and by how much): the road and kerbs by at most 4096 units of a 24 bit buffer, the car body by 4096 to 65536. So
// the pass keeps its depth test, 4096 units nearer. 0x300 is GL_SRC_COLOR.
// That was measured with a Z buffer. Since W-buffering (the console's depth for PGR2's draws) 4096 units are about a
// quarter of a metre of w, and in the sun the shadow then reached onto the car's lower body (Harry, 2026-10-20); his
// runs without it showed the shadow right. So it is off unless PGR2_SHADOW_NEARER=1.
constexpr int32_t kCarShadowBias = -4096;

void CarShadowNearer(const gpu::State& s, gpu::DrawAdjust& adjust) {
    auto simple = [&](uint32_t method) { return s.simple[method >> 2]; };
    if (!s.rs[gpu::kRsZEnable] || simple(gpu::kMZWriteEnable) || !simple(gpu::kMSolidOffsetEnable)) return;
    if (!simple(gpu::kMBlendEnable) || simple(gpu::kMSrcBlend) != 0 || simple(gpu::kMDestBlend) != 0x300) return;
    gpu::TextureDesc t;
    if (!s.textures[0] || !gpu::Describe(s.textures[0], t) || !gpu::Format(t.format).depth) return;
    adjust.depth_bias = kCarShadowBias;
    // each vertex shader it applies to, once (a few: the pass is one shader)
    static std::atomic<uint32_t> seen[4]{};
    for (auto& h : seen) {
        uint32_t cur = h.load();
        if (cur == s.vs_handle) break;
        if (cur == 0 && h.compare_exchange_strong(cur, s.vs_handle)) {
            LOGR("pgr2: car shadow pass drawn %d depth units nearer (vertex shader %08X)", -kCarShadowBias, s.vs_handle);
            break;
        }
    }
}

// PGR2's error material: an object whose material the engine cannot set up is drawn with a pixel shader and its
// combiner constant 0 magenta (ARGB 0x00FF00FF). The DLC city packs' meshes in the title update's layout come out that
// way on the disc build, with garbage vertices too: huge magenta triangles over the sky and the HUD (Harry's captures,
// 2026-10-21: draws with pixel shader and c0 = 1 0 1 0, the later fixed function draws only inherit the constant)
constexpr uint32_t kErrorMagenta = 0x00FF00FF;
bool g_skip_error_material = false;

void Filter(const gpu::State& s, gpu::DrawAdjust& adjust) {
    if (g_skip_error_material && s.ps && s.simple[gpu::kMPsConstant0 >> 2] == kErrorMagenta) {
        static std::atomic<int> said{0};
        if (said++ < 4) LOGR("pgr2: a draw with PGR2's error material (magenta) left out, vertex shader %08X", s.vs_handle);
        adjust.skip = true;
        return;
    }
    static const bool nearer = [] {
        const char* e = std::getenv("PGR2_SHADOW_NEARER");
        return e && *e == '1';
    }();
    if (nearer) CarShadowNearer(s, adjust);
}

}  // namespace

// called from the game's init (src/common/game_info.cpp): a static initializer here never ran in the Windows exe
void InstallDrawFilter() {
    const char* nearer = std::getenv("PGR2_SHADOW_NEARER");
    bool shadow = nearer && *nearer == '1';
    g_skip_error_material = pgr2::dlc;
    if (!shadow && !g_skip_error_material) return;  // the game's draws as they come
    gpu::g_draw_filter = Filter;
    if (shadow) LOGR("pgr2: PGR2_SHADOW_NEARER=1, draw filter installed (car shadow pass nearer)");
    if (g_skip_error_material) LOGR("pgr2: DLC cities on, draws with PGR2's error material are left out");
}
