#include <fstream>
#include <string>

#include "app/app.h"

namespace {
// PGR2 steps its races by a fixed time each frame and showed 30 frames a second on the console (it presents on
// the next vertical blank and took more than one), so at 60 it runs twice as fast. 30 unless the settings file
// sets fps_cap.
constexpr uint32_t kConsoleFps = 30;
void Init() {
    std::ifstream f(app::ExeDir() / app::kGame.settings);
    for (std::string line; std::getline(f, line);) {
        size_t key = line.find_first_not_of(" \t");
        if (key != std::string::npos && line.compare(key, 7, "fps_cap") == 0) return;
    }
    app::Get().fps_cap = kConsoleFps;
}
}  // namespace

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
    .options = {},                        // game settings kept in the settings file
    .load = nullptr,                      // reads more from the settings file's toml::table
    .save = nullptr,                      // writes more into it
    .init = Init,                         // after the settings load, before the window and boot
    .loaded = nullptr,                    // image in memory, before its entry point runs
    .window = nullptr,                    // the game window is up
    .message = nullptr,                   // window messages first; true takes one
    .ui = nullptr,                        // ImGui drawing each frame
    .pad = nullptr,                       // changes a polled pad
    .rumble = nullptr,                    // changes a rumble before it is set
};
