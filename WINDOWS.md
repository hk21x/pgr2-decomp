# Building and running PGR2 on Windows

Everything below runs on your PC; nothing here touches the Linux VM. FINDINGS.md explains the fixes referred to here.

## What you need

- Windows 10 or 11, 64 bit, a GPU with Direct3D 12.
- Visual Studio 2022 Community with **Desktop development with C++**. `build.bat` offers to add the two extra
  components it needs (**C++ Clang tools for Windows**, **C++ CMake tools for Windows**) through the Visual Studio
  Installer; say Y, or add them by hand under Modify, Individual components.
- Git for Windows: https://git-scm.com/download/win
- 7-Zip on the PATH, only for `release.bat`.
- Your PGR2 disc's files: `default.xbe` with the folders beside it (Cache, CarDynamics, data, Frontend, Game,
  media, ...), exactly as they are in the VM's `assets` folder. An .iso of the disc also works.
- About 15 GB free: the generated C++ alone is around 100 MB, the object files several GB.

## Steps

1. Clone with the SDK. This repository is private: Git for Windows opens a browser to sign in to GitHub the
   first time. The SDK fork is public and needs no sign-in.
   ```
   git clone --recurse-submodules https://github.com/hk21x/pgr2-decomp.git
   cd pgr2-decomp
   ```
   Already cloned before the SDK moved? Run `git pull`, then `git submodule sync`, then
   `git submodule update --init`.
2. Put the game in `assets\`: `assets\default.xbe` and its folders, or one `.iso` (then also copy
   `default.xbe` from the disc into `assets\`, the build recompiles it from there).
3. Run `build.bat` from Explorer or a plain command prompt.
   - [1/3] finds Visual Studio and clang.
   - [2/3] says `config\game.toml already written`. It must: the config is committed and must not be
     regenerated. If it ever runs `xbe_symbols` instead, stop and tell me.
   - [3/3] recompiles default.xbe (seconds) and compiles everything (30 to 90 minutes the first time).
   - Answer Y to "Play now?", or run `out\build\release\PGR2.exe` yourself.
4. The first start formats the emulated cache partition and copies the frontend files to it, then plays the four
   intro movies (about two minutes). Later starts reuse it. To repeat the first start, delete
   `out\build\release\hdd`.

Files the exe writes beside itself, in `out\build\release`:
- `logs\pgr2.log` the log of the last run (overwritten each start; send it with any report)
- `pgr2.toml` settings (window, vsync, graphics options). Speed: the game's own frame rate is honoured (menus
  60, races 30, as on the console); `fps_cap` only caps it further (60 by default, 0 for no cap of its own).
  `race_60fps = true` runs races at 60 instead (an experiment: same physics, one 1/60 step a frame).
- `hdd\` the emulated hard disk (E: saves, X/Y/Z: caches)

**Settings window: F1**, **Back + Start** on a controller, or in the game's menus **Options > General Options > PC
Settings** (it replaces Screen Center; closing the window returns to General Options by itself) (Start alone closes it; the D-pad or left stick moves,
A selects, B goes back). Tabs:
- Display: monitor (with more than one), display mode, resolution, vsync, aspect ratio.
- Graphics: render scale, anisotropic filtering, frame rate cap, antialiasing (FXAA, MSAA 2x/4x/8x), frames-a-second
  counter.
- Game: skip intro movies, hide the race HUD, race camera field of view (%), races at 60 (experimental).
- General: pause when the window is not in front.
- Controls: keys and controller remapping.
Everything is saved to `pgr2.toml` at once. Display mode, resolution, monitor, render scale, antialiasing, the counter, hide HUD
and field of view apply at once; aspect ratio, skip intro and race_60fps at the next start. While the
window is open the game gets no input.

Keyboard (plays as controller 1 while the game window has the focus; remap in F1 > Controls, two keys per input):
- Steer / menus: arrow keys or A / D (left stick); accelerate W (right trigger), brake S (left trigger)
- A: Enter or Space; B: Backspace; X: Q; Y: E; Black: R; White: F; Start: Escape or P; Back: Tab
- D-pad: numpad 8 / 2 / 4 / 6; right stick: I / J / K / L; stick clicks: C and V
Controller remapping: F1 > Controls, the last column picks which physical input drives each console input.
- `CrashDumps\` a minidump when it crashes

Useful while testing:
- **F3** writes every game thread's stack to the log. With the environment variable `XBE_CAPTURE_DIR` set to a
  folder (`set XBE_CAPTURE_DIR=C:\pgr2cap` before starting the exe from the same prompt) it also captures the
  next frame there. The log says `F3: thread stacks, the next frame captured` when the key arrives and
  `d3d12: capturing a frame into ...` when it starts. Without the key: create an empty file named `capture` beside
  `PGR2.exe` (`type nul > out\build\release\capture` from a second prompt) and the next frame is captured.
- `XBE_CAPTURE_FRAMES=6` makes F3 (or the capture file) capture that many frames in a row, one folder each
  (`HHMMSS_01` ... `HHMMSS_06`): for a flicker, so a good and a bad frame can be compared.
- Experiments, each `0` or `1` (the log says when one is set): `XBE_RENDER_THREAD` (drawing on its own thread, on
  by default), `XBE_RAW_VERTICES` (vertex data straight to the GPU, on by default), `XBE_STATE_WALK` (state set in
  program order, on by default).
- `XBE_DRAW_SKIP=00000042 B0101C41` (experiment): draws with those vertex shader handles are left out, to find
  which layer of the scene flickers (the log names the first one skipped).
- `XBE_FRAMELOG=60`: F4 records the next 60 frames' draws into `logs\framelog.txt` at full speed (no images, no
  slowdown worth noticing), with hashes of what each draw reads; for flicker a capture does not catch.
- `XBE_WBUFFER=0`: back to the old depth (z / w). By default depth works as the console's W-buffer, which PGR2
  asks for on nearly every draw (car selection panels, the showroom car's wheels, street lights behind buildings).
- `XBE_NO_TARGET_UPLOAD=1` (experiment): render targets the game reads (light pools, reflections) are never reloaded
  from memory. The log now lists such reloads (`reloaded from guest memory`) without any switch.
- `XBE_VISIBLE_ALL=1` (experiment): every GPU visibility test answers "visible" at once (lamp glows and anything
  the game draws only when visible).
- `XBE_ZBIAS=3` (experiment): every polygon offset (the depth nudge that keeps shadows and road markings on top of
  the road) times that number. Captures now list each render target's guest and host size at the top of frame.txt.
- `XBE_PERF=1` logs frame timing; `XBE_D3D12_DEBUG=1` turns on the Direct3D 12 debug layer (slow).
- `XBE_VSC=56 54` logs who writes those vertex shader constants and the values (each distinct one once); used to
  find why the player's car body is black.
- `XBE_CAPTURE_DRAWS=1580-1660` makes a capture also save the image after each draw in that range (the numbers in
  the capture's frame.txt); `XBE_CAPTURE_DRAWS=vs:B01019E1` after every draw with that vertex shader (the car body),
  plus once before the first.
- `XBE_DRAW_TEST=B01019E1:alphakill` (or `:cull`, `:depth`, several joined by commas): an experiment, draws with that
  vertex shader without that test.
- Game switches (experiments, `1` to turn on): `PGR2_NIGHT` every race at night; `PGR2_NO_FILTER` the car shadow
  pass without its depth fix; `PGR2_FLARE_LOG` / `PGR2_FLARES` exhaust flame diagnostics; `PGR2_LIGHT` /
  `PGR2_LIGHT_CLEAR` car lighting diagnostics. `XNET_TRACE=1` logs every network call (system link).
- Leftover variables from an earlier experiment give false regressions: the log lists every `XBE_*` and `PGR2_*`
  variable it sees at the top. Clear them (`Remove-Item Env:XBE_DRAW_TEST` in PowerShell) when done.
- Setting these: in Command Prompt `set XBE_VSC=56 54`; in PowerShell `$env:XBE_VSC = "56 54"` (PowerShell's
  `set` does not set an environment variable). Start `PGR2.exe` from that same window.

`release.bat` zips `PGR2.exe` with an empty `assets` folder; not needed yet.

## System link

On one LAN: start PGR2 on both PCs, then Multiplayer > System Link: one creates a game, the other searches.
Past the LAN: set `join_ip = 'a.b.c.d'` (the host's address) in the joiner's `pgr2.toml`; the search also asks that
host. On one PC: copy `out\build\release` to a second folder and start each copy with `PGR2.exe --instance 1` and
`--instance 2`. Headless this works end to end (lobby, race, results); it has not been tried on Windows yet.

## What to look at and report

Please note for each: works / wrong (how) / not reached, and send `logs\pgr2.log`. Screenshots and short clips help;
for a flicker, a frame log (F4 with `XBE_FRAMELOG=60`) at the spot.

Confirmed on Windows so far: boot, movies, menus, races, saves, sound, reflections, the player's car body, the car
shadow, fences and signs, car selection, the F1 window.

Still to confirm:
1. **Street lights at night**: glows hidden behind buildings, still visible where the lamp is in view.
1a. **PC Settings in the game's menu**: Options > General Options > PC Settings opens the window over the page;
   close it (Start, F1 or the window's X) and the menu goes back to General Options on its own.
1b. **Achievements**: F1 > Achievements lists the Kudos rank ones; reaching Kudos Rank 5 (20,250 Kudos in total)
   should pop up "Achievement unlocked" top right and write achievements.toml beside the exe.
1d. **Draw distance** (F1 > Game, experimental): try 200-400% in a city with long views (Sydney, Nürburgring);
   in Hong Kong it changes nothing visible, the street ends first.
1c. **New options** (F1): Back + Start from a controller (Xbox and PlayStation pads) and navigating the window with
   it; skip intro (next start goes straight to the title); hide HUD in a race; field of view at 130% (is the road
   near the car drawn? the CPU renderer here leaves a band there at any setting); FXAA on and off; the counter; pause
   on alt-tab (sound stops too); a second monitor if you have one.
2. **Flicker**: buildings at night (Edinburgh), kerbs, signs; the light patch under the car at night.
3. **Keyboard and remapping**: drive a race on the keyboard; remap a key and a controller input, restart, check
   they were kept.
4. **Display options**: each display mode and a resolution change while racing; aspect 4:3 and 16:9 (next start).
5. **System link**: two PCs, or two copies with `--instance`.
6. **race_60fps**: feel, replays and ghosts recorded at 60.
7. **Sound**: music volume in the menus, the engine, anything missing (reverb and filters are not modelled).
