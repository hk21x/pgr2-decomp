// PGR2's draws the renderer changes (gpu::g_draw_filter)
#include <atomic>

#include "core/log.h"
#include "gpu/state.h"
#include "gpu/texture.h"

namespace {

// The car's shadow: a ground mesh around the car that the game builds each frame, drawn after the scene with the car's
// shadow map (a depth texture compared in stage 0), a polygon offset, no depth writes and a multiply blend. The mesh
// does not lie on the visible road and reaches behind the camera; on D3D12 parts of it failed the depth test against
// the road, changing as the camera moved: the shadow and the sunlit road around the car flickered. Without the depth
// test it was steady but showed through the car. Measured headless (which shader's depth stopped each failed pixel,
// and by how much): the road and kerbs by at most 4096 units of a 24 bit buffer, the car body by 4096 to 65536. So
// the pass keeps its depth test, 4096 units nearer. 0x300 is GL_SRC_COLOR.
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

}  // namespace

// called from the game's init (src/common/game_info.cpp): a static initializer here never ran in the Windows exe
void InstallDrawFilter() {
    gpu::g_draw_filter = CarShadowNearer;
    LOGR("pgr2: draw filter installed (car shadow pass nearer)");
}
