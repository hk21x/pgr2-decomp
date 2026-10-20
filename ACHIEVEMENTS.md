# PGR2 achievements - draft set

A draft achievement set for Project Gotham Racing 2, built so it can be shown in the port now and submitted to
[RetroAchievements](https://retroachievements.org) later, if it ever supports the original Xbox. No PGR2 set exists
there yet. Status: **draft list**. The memory conditions are still to be mapped (see "Mapping").

## Design rules (RetroAchievements conventions)

- Every achievement is a condition on the game's memory, never on our own code. That way the same logic works for
  any runtime that exposes the console's RAM: this port, or a future RA Xbox core. Conditions are written in
  rcheevos syntax (`0xX<address>` reads 32 bits, `0xH` 8 bits, and so on), against guest addresses. Static data
  in the XBE has the same address on the console and here; data on the heap is reached by pointer chains from
  a static global (`I:0xX<global>_0xX<offset>=...`), not by fixed heap addresses.
- Achievements fire on the moment something is earned (a medal goes from below to at-or-above a level, a rank
  rises past N), with a delta check (`d0x...`), never on simply loading a finished profile. They also check that
  a race or the frontend is actually running, not a demo, replay or attract race.
- Points follow RA's scale: 1, 2, 3, 4, 5, 10, 25, 50 and 100, weighted by effort.
- **Progression** achievements are each part of the career. **Win** marks the end of a career. **Missable** and
  **challenge** achievements are separate.
- Achievements that need Xbox Live are left out (the service is gone). So are those that need a second player
  (split screen, system link): RA treats multiplayer-only goals as optional at best.
- Difficulty is never unlocked by settings: every medal achievement counts the medal the game awarded.

## The game's structure (from its own data)

- **Kudos World Series**: 14 classes (Compact Sports, Sports Convertible, Coupe, Sport Utility, Pacific Muscle,
  Roadster, Classics, Sports Coupe, American Muscle, Super Car, Grand Touring, Track Specials, Extreme, Ultimate).
  - Each class has 3 to 13 series: 75 series and 301 races in all.
  - Every series can be won at five medal levels: Novice (Steel), Easy (Bronze), Medium (Silver), Hard (Gold),
    Expert (Platinum).
  - Finishing the whole Kudos World Series at each medal level unlocks a car: Steel the Feroce, Bronze the
    250 GTO, Silver the 911 GT1, Gold the CLK-GTR, Platinum the Cerbera Speed 12.
- **Kudos ranks**: 60. Rank 2 needs 4,000 Kudos, rank 60 needs 8,153,000. Ranks pay out Kudos Tokens.
- **Garage**: Kudos Tokens unlock cars. The game lists 102 car entries (how many a player can own is still to
  confirm).
- **Arcade Racing**: Street Racing, Timed Run and Cone Challenge, 5 stages each, with medals.
- **Time Attack**: Circuit Challenge (lap records per route) and Car Challenge.
- **Geometry Wars** on the arcade machine in the garage.
- Race types: Street Race, Hot Lap, Timed Run, One on One, Speed Camera, Overtake, Cone Challenge, Championship.

## The set (64 achievements, 780 points)

### Kudos World Series: whole-series medals (progression and win)
| # | Title | Description | Points | Type |
| --- | --- | --- | --- | --- |
| 1 | Steel Nerves | Complete the Kudos World Series at Novice (Steel) | 25 | progression |
| 2 | Bronze Age | Complete the Kudos World Series at Easy (Bronze) or better | 25 | progression |
| 3 | Silver Service | Complete the Kudos World Series at Medium (Silver) or better | 50 | progression |
| 4 | Golden Era | Complete the Kudos World Series at Hard (Gold) or better | 50 | progression |
| 5 | Platinum Driver | Complete the Kudos World Series at Expert (Platinum) | 100 | win |

### Kudos World Series: classes (28)
For each of the 14 classes, two achievements:
- **"<Class> Graduate"**: every series in the class won with any medal (Steel or better). Points: 3 for classes
  1 to 7, 4 for classes 8 to 14.
- **"<Class> Master"**: every series in the class won at Platinum. Points: 5 for classes 1 to 7, 10 for classes
  8 to 14.

Suggested titles, Graduate / Master:

| Class | Graduate / Master |
| --- | --- |
| Compact Sports | Pocket Rocket / Hot Hatch Hero |
| Sports Convertible | Top Down / Wind in Your Hair |
| Coupe | Two Doors / Coupe de Grace |
| Sport Utility | Utility Player / Off the Beaten Path |
| Pacific Muscle | Pacific Rim / Rising Sun |
| Roadster | Open Road / Roadster Royalty |
| Classics | Old School / Timeless |
| Sports Coupe | Sport Mode / Coupe d'Etat |
| American Muscle | V8 Thunder / Stars and Stripes |
| Super Car | Super Licence / Supremacy |
| Grand Touring | Grand Tour / Gentleman Driver |
| Track Specials | Track Day / Race Bred |
| Extreme | Living on the Edge / Extremist |
| Ultimate | Ultimate Driver / The Ultimate |

Points across all 28: 21 + 28 + 35 + 70 = 154.

### Kudos ranks (7)
| Title | Description | Points |
| --- | --- | --- |
| Making a Name | Reach Kudos Rank 5 | 2 |
| Rising Star | Reach Kudos Rank 10 | 3 |
| Crowd Pleaser | Reach Kudos Rank 20 | 5 |
| Showman | Reach Kudos Rank 30 | 10 |
| Kudos King | Reach Kudos Rank 40 | 10 |
| Living Legend | Reach Kudos Rank 50 | 25 |
| Gotham's Finest | Reach Kudos Rank 60 | 50 |

Points: 105.

### Garage (5)
| Title | Description | Points |
| --- | --- | --- |
| Token Gesture | Unlock a car with Kudos Tokens | 2 |
| Collector | Own 25 cars | 5 |
| Curator | Own 50 cars | 10 |
| Museum Piece | Own 75 cars | 25 |
| Full House | Own every car | 50 |

Points: 92.

### Arcade Racing (7)
| Title | Description | Points |
| --- | --- | --- |
| Street Smart | Gold or better on all 5 Street Racing stages | 5 |
| Against the Clock | Gold or better on all 5 Timed Run stages | 5 |
| Cone Zone | Gold or better on all 5 Cone Challenge stages | 5 |
| Street Legend | Platinum on all 5 Street Racing stages | 10 |
| Clockwork | Platinum on all 5 Timed Run stages | 10 |
| Cone Artist | Platinum on all 5 Cone Challenge stages | 10 |
| Arcade Perfect | Platinum on all 15 Arcade stages | 25 |

Points: 70.

### Single-event feats (challenge) (8)
| Title | Description | Points |
| --- | --- | --- |
| Style Points | Earn 2,000 Kudos in a single race | 3 |
| Showboat | Earn 10,000 Kudos in a single race | 10 |
| Ton Up | Reach 100 mph at a Speed Camera | 2 |
| Flat Out | Reach 175 mph at a Speed Camera | 10 |
| Clean Sweep | Win an Expert One on One | 5 |
| Traffic Control | Win an Expert Overtake event | 5 |
| Untouchable | Win a Kudos World Series race at Expert without touching a wall or a car | 25 |
| Lap of Honour | Set a lap record in Time Attack on 10 different routes | 10 |

Points: 70. The thresholds are guesses, to be tuned once real values can be read from memory during play.

### Geometry Wars (4)
| Title | Description | Points |
| --- | --- | --- |
| Insert Coin | Play Geometry Wars in the garage | 1 |
| Pacifist | Score 10,000 in Geometry Wars | 3 |
| Grid Warrior | Score 100,000 in Geometry Wars | 10 |
| Geometry Master | Score 1,000,000 in Geometry Wars | 25 |

Points: 39.

### Totals
5 + 28 + 7 + 5 + 7 + 8 + 4 = **64 achievements**. 250 + 154 + 105 + 92 + 70 + 70 + 39 = **780 points**. RA sets
for racing games typically run 400 to 1,000.

## Also part of an RA set

- **Leaderboards**:
  - Time Attack best lap per route.
  - Geometry Wars high score.
  - Most Kudos in a single race.
  - Arcade stage results.
- **Rich presence**, the line shown on a player's profile. For example: "Kudos World Series, Super Car, Hard |
  Edinburgh, Princes Street | Ferrari Enzo, lap 2 of 3 | Kudos Rank 34". Built from the mode, class, city, route,
  car, lap and rank in memory.

## Mapping: what must be found in memory

Each item needs a static address or a pointer chain, found by changing one thing in game and watching memory (the
harness, gdb watchpoints, and Harry's save files for late-game states):

1. **The profile in RAM.** Profile.Sav (30,100 bytes) is loaded into a buffer. Known so far: byte 0x844 is the
   MPH/KM/H option, and there are career fields at 0x8DC and 0xFEC to 0x1002. Needed: Kudos total, rank, tokens
   held and spent, the medal level per series (75), Arcade medals (15), cars owned (a bit or byte per car), and
   lap records.
2. **Race state.** Game mode, class, series, race type, difficulty, the race's Kudos, position, finish flag,
   speed camera speed, collision counts.
3. **Context flags.** In race, in replay, attract demo, in frontend, profile loaded. Each achievement checks these.
4. **Geometry Wars.** Score and "playing".

Late-game states cannot be reached by headless pad scripts (they cannot steer). A few of Harry's Profile.Sav files
at different stages of a career let the career fields be read and checked directly.

## Mapped so far (2026-10-20)

Found with the harness's `memdump` (guest memory against a saved Profile.Sav) and the status screens' code:
- **Live profile**: static, at 0x3665AC (the XBE's data, the same on a console). Profile.Sav's 30,100 bytes match it
  except the 20-byte signature and runtime-filled records. A second copy is on the heap at 0x6321DC.
- **Kudos Tokens held**: dword 0x366EBC (profile +0x910), read by the token line at 0x6F3E1.
- **Total Kudos**: the sum of the dwords at 0x35D090 + 16 x i, for i below the dword at 0x35D080 (8): the object at
  0x35BA78, method 0xA66C0. Kudos ranks follow from it (Frontend\Ranking.Ini: rank 2 at 4,000 ... rank 60 at
  8,153,000).
- **Cars owned**: car n (0 to 101; count 0x3109F0) is owned when bit p of the dword at 0x35BAA4 + n x 0x1C is set,
  p being the player index at 0x366800, and the car's entry in the car table (0x310A68 + n x 0x9C) is non-zero
  (0xA63C0; the byte 0x291969 unlocks every car).
- **Race state** (the race HUD, 0xDB460 and 0xE0840; checked live against the HUD in a headless race):
  - player n's car: dword 0x49E258 + n x 0x158 (0 for player 1)
  - each car's record: 0x2498 bytes from 0x39B000 (car c at + c x 0x2498): lap (int16, from 0; negative before the
    start) at 0x39B334, position (dword, from 0) at 0x39B33C, a flag byte at 0x39B30E that takes a car out of the
    field count
  - cars in the race: dword 0x383818; laps: dword 0x383830; race type: dword 0x383814 (1 in Instant Action)
  - finished: lap >= laps (the HUD switches to "Finished" there, 0xDB5CC)
  - the end-of-race Kudos breakdown reads floats at 0x39D42C, 0x39D44C, 0x39D46C, 0x39D48C (+ the car's offset)
  - in condition syntax, player 1 finishing first: `0xX39b33c=0_0x 39b334>=0xX383830` (with car 0; a pointer chain
    through 0x49E258 for any car)
- To find: Kudos World Series and Arcade medals per series and stage, Time Attack challenges (92 circuit, 102 car),
  race state (mode, finish, position, the race's Kudos), Geometry Wars score. A profile with progress (Harry's saves)
  is the quickest way for the medals.

## In the port

Built (2026-10-20): `sdk/runtime/core/achievements.cpp` evaluates RetroAchievements-syntax conditions (memory sizes,
delta and prior, add source, sub source, add address, and/or next, reset and pause ifs, hit counts, alternative
groups) once a frame, waits for a condition to be unmet once before it can unlock, keeps unlocks in
`achievements.toml` beside the exe, shows a pop-up and lists the set in the settings window's Achievements tab.
`src/game/achievement_set.cpp` holds the mapped part of the set: the seven Kudos rank achievements.
`PGR2_ACH_TEST=1` adds a test one that unlocks at a race's start (checked headless).

The plan was:

- The SDK gets a small achievement runtime:
  - It evaluates the set's conditions against guest memory once a frame, as RA's rcheevos library does. The
    library itself (MIT licence) is a candidate to embed, which would make the format exactly RA's.
  - It shows a pop-up when an achievement unlocks.
  - It adds an Achievements tab to the settings window.
  - It saves unlocks to `achievements.json` beside the exe, separate from the game's saves.
- The set lives in a data file in this repository (titles, descriptions, points, conditions). Nothing from the
  disc is needed for it.
- A "hardcore" switch, as RA has, would turn off anything that changes the game (race_60fps, experiments) while
  achievements are on.
