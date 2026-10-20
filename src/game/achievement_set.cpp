// PGR2's achievements (ACHIEVEMENTS.md has the whole draft set and the memory map). Only those whose memory is mapped
// are here; conditions are in RetroAchievements' syntax so the set can go there as it is.
#include <cstdlib>
#include <vector>

#include "core/achievements.h"

namespace pgr2 {

// Total Kudos: the eight dwords at 0x35D090 + 16 i (the object at 0x35BA78, summed by 0xA66C0 for the status
// screen); the ranks' thresholds are Frontend\Ranking.Ini's
#define PGR2_TOTAL_KUDOS "A:0xX35d090_A:0xX35d0a0_A:0xX35d0b0_A:0xX35d0c0_A:0xX35d0d0_A:0xX35d0e0_A:0xX35d0f0_0xX35d100"

std::vector<achievements::Def> AchievementSet() {
    std::vector<achievements::Def> set = {
        {"rank5", "Making a Name", "Reach Kudos Rank 5", 2, PGR2_TOTAL_KUDOS ">=20250"},
        {"rank10", "Rising Star", "Reach Kudos Rank 10", 3, PGR2_TOTAL_KUDOS ">=61000"},
        {"rank20", "Crowd Pleaser", "Reach Kudos Rank 20", 5, PGR2_TOTAL_KUDOS ">=194250"},
        {"rank30", "Showman", "Reach Kudos Rank 30", 10, PGR2_TOTAL_KUDOS ">=382500"},
        {"rank40", "Kudos King", "Reach Kudos Rank 40", 10, PGR2_TOTAL_KUDOS ">=575000"},
        {"rank50", "Living Legend", "Reach Kudos Rank 50", 25, PGR2_TOTAL_KUDOS ">=1520000"},
        {"rank60", "Gotham's Finest", "Reach Kudos Rank 60", 50, PGR2_TOTAL_KUDOS ">=8153000"},
    };
    // PGR2_ACH_TEST=1: a test achievement that unlocks when a race starts (D3D's present interval, 0x1D093C, goes to
    // two vertical blanks a frame)
    if (const char* e = std::getenv("PGR2_ACH_TEST"); e && *e == '1')
        set.push_back({"test_race", "Test: green light", "Start a race", 1, "0xX1d093c=h80000002"});
    return set;
}

}  // namespace pgr2
