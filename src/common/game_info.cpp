#include <cstdlib>

#include "app/app.h"

void InstallDrawFilter();  // src/game/draw_filter.cpp

namespace pgr2 {
extern bool skip_intro, hide_hud;
extern uint32_t fov_percent;
void SettingsEntryPad(int port, input::Pad& pad);  // src/game/settings_entry.cpp
std::vector<achievements::Def> AchievementSet();    // src/game/achievement_set.cpp
}

namespace {
// PGR2_SKIP_INTRO=1, PGR2_HIDE_HUD=1 and PGR2_FOV=percent set those options without a settings file (the headless harness has none)
void Init() {
    InstallDrawFilter();
    if (const char* e = std::getenv("PGR2_SKIP_INTRO"); e && *e == '1') pgr2::skip_intro = true;
    if (const char* e = std::getenv("PGR2_HIDE_HUD"); e && *e == '1') pgr2::hide_hud = true;
    if (const char* e = std::getenv("PGR2_FOV")) pgr2::fov_percent = uint32_t(std::atoi(e));
}
}  // namespace

namespace pgr2 {
bool race_60fps = false;  // races at 60 frames a second instead of the console's 30 (src/game/hooks.cpp)
bool skip_intro = false;  // no intro movies at start (src/game/hooks.cpp)
bool hide_hud = false;    // no race HUD, for screenshots (src/game/hooks.cpp)
uint32_t fov_percent = 100;  // the race cameras' field of view, percent of the console's (src/game/hooks.cpp)
}

// PGR2 asks for one vertical blank a frame in its menus and two in its races (D3D_PresentInterval), and steps its
// races by a fixed 1/30 s a frame; the runtime paces by that, so the default fps_cap of 60 runs both at console speed.

// what the runtime asks of the game; strings must be set, hooks may stay null
const app::GameInfo app::kGame = {
    .title = L"Project Gotham Racing 2",  // window and splash
    .short_title = L"PGR2",               // message boxes
    .name = L"PGR2",                      // window classes, crash dumps, handover event
    .log = "pgr2",                        // log file prefix
    .settings = "pgr2.toml",              // settings file beside the exe
    .splash = L"splash",                  // optional splash.png or splash.bmp beside the exe
    .missing = L"Put the game in the assets folder: its disc image (.iso), or default.xbe with its folders beside it.",
    .images = {{"default.xbe", "PGR2"}},  // each recompiled xbe and its exe, for title launches
    // game settings kept in the settings file
    .options = {{"skip_intro", &pgr2::skip_intro, nullptr, "Skip intro movies", "At the next start."},
                {"hide_hud", &pgr2::hide_hud, nullptr, "Hide the race HUD", "For screenshots and replays; at once."},
                {"fov_percent", nullptr, &pgr2::fov_percent, "Race camera field of view (%)",
                 "100 is the console's; 50 to 200, at once."},
                {"race_60fps", &pgr2::race_60fps, nullptr, "Races at 60 frames a second (experimental)",
                 "Same physics, sampled twice as often; at the next start."}},
    .load = nullptr,                      // reads more from the settings file's toml::table
    .save = nullptr,                      // writes more into it
    .init = Init,                         // after the settings load, before the window and boot
    .loaded = nullptr,                    // image in memory, before its entry point runs
    .window = nullptr,                    // the game window is up
    .message = nullptr,                   // window messages first; true takes one
    .ui = nullptr,                        // ImGui drawing each frame
    .pad = pgr2::SettingsEntryPad,        // changes a polled pad: B back from PC Settings
    .rumble = nullptr,                    // changes a rumble before it is set
    .achievements = pgr2::AchievementSet(),  // conditions on guest memory, checked each frame
};
