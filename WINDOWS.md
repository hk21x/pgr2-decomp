# Building and running PGR2 on Windows

The first Windows build. Everything below runs on your PC; nothing here touches the Linux VM.

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

**Settings window: F1** (in game, any time). Tabs: Display (display mode, resolution, vsync, aspect ratio),
Graphics (render scale, anisotropic filtering, frame rate cap), Game (race_60fps), Controls. Everything is saved to
`pgr2.toml` at once. Display mode and resolution apply at once; aspect ratio, render scale and game options at the
next start. While the window is open the game gets no input.

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
- Setting these: in Command Prompt `set XBE_VSC=56 54`; in PowerShell `$env:XBE_VSC = "56 54"` (PowerShell's
  `set` does not set an environment variable). Start `PGR2.exe` from that same window.

`release.bat` zips `PGR2.exe` with an empty `assets` folder; not needed yet.

## What to look at and report

Please note for each: works / wrong (how) / not reached, and send the log. Screenshots help.

1. **Starts at all**: window titled "Project Gotham Racing 2", no message box. If it closes, the log's last lines.
2. **Intro movies** (Microsoft Game Studios / Bizarre Creations logos, the intro): in the headless renderer they
   are black. Picture? Sound? Smooth or stuttering? Do they end by themselves?
3. **Do not press START during the second intro movie**: it hangs the movie player (known, see PROGRESS.md).
   Pressing it during the first movie or later is worth one try each: does it skip, or hang?
4. **Attract demo** after the movies (a race in Hong Kong and other cities, no input): are the **car bodies**
   there? In the headless renderer some cars show only wheels and driver. Lighting too dark?
5. **Title screen** (press START during the attract demo): the PGR2 logo over a car picture. Headless it shows
   **dotted textures and white bands** across the panels; are they on Windows too?
6. **After pressing A on the title**: what screen comes up, does the menu respond to the D-pad?
7. Sound in the menus and the attract demo.
8. Frame rate (the log has it with XBE_PERF=1) and anything flickering.
