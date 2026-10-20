// The settings window from PGR2's own menu: Options > General Options > Screen Center (which a PC monitor has no use
// for) becomes PC Settings. Its page still builds (a page that builds nothing cannot be left), the settings window
// opens over it, and when the window closes the player is given a B press back to General Options.
//
// General Options is built by 0xA4100: each item a button with a page builder (0x58970), a label (0x89200) and a help
// line (0x891E0), both pointers from the UTF-16 string table at 0x28A870 by StringList.ini id.
#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <string>

#include "app/app.h"
#include "core/input.h"
#include "core/log.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "xdk/hle.h"

namespace pgr2 {

namespace {

constexpr uint32_t kStrings = 0x28A870;  // string table: a UTF-16 pointer per id
struct Text {
    uint32_t id;
    const char16_t* text;
};
constexpr Text kTexts[] = {
    {965, u"PC Settings"},                            // IDS_OPTIONS_SCREEN_CENTRE, the item and its page's text
    {1406, u"Display, graphics, controls and more."},  // IDS_ON_FOCUS_HELP_OPTIONS_SCREEN
    {1044, u"PC SETTINGS"},                           // IDS_SCREEN_CENTRE, the page title
};
uint32_t g_guest[3];  // our copies in guest memory

// the window opened from the menu: B is pressed once it has closed and the page has settled
std::atomic<bool> g_from_menu{false};
using Clock = std::chrono::steady_clock;
Clock::time_point g_closed{};
constexpr auto kSettle = std::chrono::milliseconds(700), kHold = std::chrono::milliseconds(200);

}  // namespace

// each frame (src/game/hooks.cpp, before the frame step): the table entries point at our texts; the table is filled
// when the game loads its language's strings, so they are put back whenever the game wrote its own
void ApplySettingsEntry() {
    if (!kernel::R32(kStrings + 965 * 4)) return;  // strings not loaded yet
    for (int i = 0; i < 3; i++) {
        const Text& t = kTexts[i];
        if (!g_guest[i]) {
            size_t n = std::char_traits<char16_t>::length(t.text) + 1;
            g_guest[i] = mem::AllocSystem(uint32_t(n * 2));
            std::memcpy(kernel::HostPtr(g_guest[i]), t.text, n * 2);
        }
        uint32_t entry = kStrings + t.id * 4;
        if (kernel::R32(entry) != g_guest[i]) {
            static int logged = 0;
            if (logged++ < 3) LOGR("pgr2: string %u becomes %s", t.id, i == 0 ? "PC Settings" : i == 1 ? "its help" : "its title");
            kernel::W32(entry, g_guest[i]);
        }
    }
}

// GameInfo::pad: after the window opened from the menu closes, and the page has had time to settle (a press while it
// slides in is lost), B held briefly on port 0
void SettingsEntryPad(int port, input::Pad& pad) {
    if (port != 0 || !g_from_menu) return;
    auto now = Clock::now();
    if (app::OverlayOpen()) {
        g_closed = {};
        return;
    }
    if (g_closed == Clock::time_point{}) g_closed = now;
    if (now < g_closed + kSettle) return;
    if (now < g_closed + kSettle + kHold) {
        pad.analog[1] = 255;  // Pad::analog: A B X Y ...
        return;
    }
    g_from_menu = false;
    LOGR("pgr2: back to General Options");
}

}  // namespace pgr2

// Screen Center's page builder (0xA2E60, no arguments): the page as the game builds it, then the settings window
X86_EXTERN_FUNC(__imp__sub_000A2E60);
X86_FUNC(sub_000A2E60) {
    __imp__sub_000A2E60(ctx, base);
    LOGR("pgr2: PC Settings chosen in the Options menu");
    {
        std::lock_guard lock(app::UiLock());
        if (!app::OverlayOpen()) app::ToggleOverlay();
    }
    pgr2::g_closed = {};
    pgr2::g_from_menu = true;
}
