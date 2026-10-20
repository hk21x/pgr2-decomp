// PGR2's achievements (ACHIEVEMENTS.md has the whole draft set and the memory map). Only those whose memory is mapped
// are here; conditions are in RetroAchievements' syntax so the set can go there as it is.
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <string>
#include <vector>

#include "core/achievements.h"

namespace pgr2 {

// Total Kudos: the eight dwords at 0x35D090 + 16 i (the object at 0x35BA78, summed by 0xA66C0 for the status
// screen); the ranks' thresholds are Frontend\Ranking.Ini's
#define PGR2_TOTAL_KUDOS "A:0xX35d090_A:0xX35d0a0_A:0xX35d0b0_A:0xX35d0c0_A:0xX35d0d0_A:0xX35d0e0_A:0xX35d0f0_0xX35d100"

namespace {

// Medals: each race's record is 0x1C bytes from 0x35DD88 (the object 0x35BA78 + 0x2310), its first dword the medal won,
// 1 Steel to 5 Platinum, 0 none (0xA66E0 counts them for the status screens). The races of the Kudos World Series
// classes and the Arcade types are consecutive (the level tables at 0x34A8C8 and 0x34C284, from Frontend\Levels.Ini).
struct Range {
    int first, last;
};
constexpr Range kClasses[14] = {{1, 7},     {8, 14},    {15, 22},   {23, 30},   {31, 40},   {41, 51},   {52, 62},
                                {63, 75},   {76, 89},   {90, 103},  {104, 119}, {120, 136}, {137, 153}, {154, 172}};
constexpr Range kSeries = {1, 172};
constexpr Range kArcade[3] = {{173, 192}, {193, 212}, {213, 232}};  // street racing, timed run, cone challenge
constexpr Range kArcadeAll = {173, 232};

// every race in the range with at least that medal; the strings stay for the program's life
const char* Medals(Range r, int medal) {
    static std::deque<std::string> kept;
    std::string c;
    for (int i = r.first; i <= r.last; i++) {
        char b[32];
        std::snprintf(b, sizeof b, "%s0xX%x>=%d", c.empty() ? "" : "_", 0x35DD88 + i * 0x1C, medal);
        c += b;
    }
    return kept.emplace_back(std::move(c)).c_str();
}

}  // namespace

std::vector<achievements::Def> AchievementSet() {
    std::vector<achievements::Def> set = {
        {"kws_steel", "Steel Nerves", "Complete the Kudos World Series at Novice (Steel)", 25, Medals(kSeries, 1)},
        {"kws_bronze", "Bronze Age", "Complete the Kudos World Series at Easy (Bronze) or better", 25, Medals(kSeries, 2)},
        {"kws_silver", "Silver Service", "Complete the Kudos World Series at Medium (Silver) or better", 50, Medals(kSeries, 3)},
        {"kws_gold", "Golden Era", "Complete the Kudos World Series at Hard (Gold) or better", 50, Medals(kSeries, 4)},
        {"kws_platinum", "Platinum Driver", "Complete the Kudos World Series at Expert (Platinum)", 100, Medals(kSeries, 5)},
        {"rank5", "Making a Name", "Reach Kudos Rank 5", 2, PGR2_TOTAL_KUDOS ">=20250"},
        {"rank10", "Rising Star", "Reach Kudos Rank 10", 3, PGR2_TOTAL_KUDOS ">=61000"},
        {"rank20", "Crowd Pleaser", "Reach Kudos Rank 20", 5, PGR2_TOTAL_KUDOS ">=194250"},
        {"rank30", "Showman", "Reach Kudos Rank 30", 10, PGR2_TOTAL_KUDOS ">=382500"},
        {"rank40", "Kudos King", "Reach Kudos Rank 40", 10, PGR2_TOTAL_KUDOS ">=575000"},
        {"rank50", "Living Legend", "Reach Kudos Rank 50", 25, PGR2_TOTAL_KUDOS ">=1520000"},
        {"rank60", "Gotham's Finest", "Reach Kudos Rank 60", 50, PGR2_TOTAL_KUDOS ">=8153000"},
    };
    // two for each class: every race with a medal, every race at Platinum
    static const char* const kClass[14][3] = {
        {"Compact Sports", "Pocket Rocket", "Hot Hatch Hero"},       {"Sports Convertible", "Top Down", "Wind in Your Hair"},
        {"Coupe", "Two Doors", "Coupe de Grace"},                    {"Sport Utility", "Utility Player", "Off the Beaten Path"},
        {"Pacific Muscle", "Pacific Rim", "Rising Sun"},              {"Roadster", "Open Road", "Roadster Royalty"},
        {"Classics", "Old School", "Timeless"},                       {"Sports Coupe", "Sport Mode", "Coupe d'Etat"},
        {"American Muscle", "V8 Thunder", "Stars and Stripes"},       {"Super Car", "Super Licence", "Supremacy"},
        {"Grand Touring", "Grand Tour", "Gentleman Driver"},          {"Track Specials", "Track Day", "Race Bred"},
        {"Extreme", "Living on the Edge", "Extremist"},               {"Ultimate", "Ultimate Driver", "The Ultimate"}};
    static std::deque<std::string> text;  // ids and descriptions made here
    for (int c = 0; c < 14; c++) {
        bool late = c >= 7;
        const char* id = text.emplace_back("class" + std::to_string(c + 1) + "_any").c_str();
        const char* desc = text.emplace_back("Win every " + std::string(kClass[c][0]) + " event with a medal").c_str();
        set.push_back({id, kClass[c][1], desc, late ? 4u : 3u, Medals(kClasses[c], 1)});
        id = text.emplace_back("class" + std::to_string(c + 1) + "_platinum").c_str();
        desc = text.emplace_back("Win every " + std::string(kClass[c][0]) + " event at Platinum").c_str();
        set.push_back({id, kClass[c][2], desc, late ? 10u : 5u, Medals(kClasses[c], 5)});
    }
    set.insert(set.end(), {
        {"arcade_street_gold", "Street Smart", "Gold or better on every Street Racing stage", 5, Medals(kArcade[0], 4)},
        {"arcade_timed_gold", "Against the Clock", "Gold or better on every Timed Run stage", 5, Medals(kArcade[1], 4)},
        {"arcade_cone_gold", "Cone Zone", "Gold or better on every Cone Challenge stage", 5, Medals(kArcade[2], 4)},
        {"arcade_street_platinum", "Street Legend", "Platinum on every Street Racing stage", 10, Medals(kArcade[0], 5)},
        {"arcade_timed_platinum", "Clockwork", "Platinum on every Timed Run stage", 10, Medals(kArcade[1], 5)},
        {"arcade_cone_platinum", "Cone Artist", "Platinum on every Cone Challenge stage", 10, Medals(kArcade[2], 5)},
        {"arcade_platinum", "Arcade Perfect", "Platinum on every Arcade stage", 25, Medals(kArcadeAll, 5)},
    });
    // PGR2_ACH_TEST=1: a test achievement that unlocks when a race starts (D3D's present interval, 0x1D093C, goes to
    // two vertical blanks a frame)
    if (const char* e = std::getenv("PGR2_ACH_TEST"); e && *e == '1')
        set.push_back({"test_race", "Test: green light", "Start a race", 1, "0xX1d093c=h80000002"});
    return set;
}

}  // namespace pgr2
