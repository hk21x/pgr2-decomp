# Project Gotham Racing 2 - XBE Recomp

An unofficial native PC port of the original Xbox version of Project Gotham Racing 2, with extended graphics options and other enhancements.

This port was created using the [Xbox XBE Recomp](https://github.com/TGP482/XboxXBE-Recomp) tool.

> [!NOTE]
> This project does not include any game assets. You must provide the files from your own legally acquired copy of the game to install or build.

## Installation

- Put your game files in the `assets` folder. This can be an ISO, or the extracted data files.
- Run `PGR2.exe`.
- Recomp settings can be adjusted in `pgr2.toml` beside the exe.

## Features

### Display
- **Display Modes** - Fullscreen, borderless window or window, at any resolution the monitor offers, applied at once.
- **Widescreen** - The game's own 16:9 mode (wider field of view, HUD at the edges), chosen from the screen's shape or set to 4:3 or 16:9.
- **Frame Pacing** - Runs at the game's own rate, menus at 60 and races at 30 as on the console, with vsync and a frame rate cap.
- **Monitor** - Chooses which display the game uses.
- **FPS Counter** - Shows the frame rate in a corner.

### Graphics
- **Render Scale** - Renders at a percentage of the screen, from 25% to 200%.
- **Anisotropic Filtering** - Up to 16x.
- **Antialiasing** - FXAA on the finished frame; render scale above 100% supersamples.
- **Field of View** - Widens or narrows the race cameras, 50% to 200% of the console's.
- **Console Accurate Depth** - W-buffered depth as the Xbox does it, keeping car bodies, showroom cars and street lights right.
- **Reflections** - Car paint and glass reflections from the game's environment cube.

### Controller
Supports Xbox, PlayStation and Switch controllers, with hot plugging and rumble. Any console input can be mapped to any controller input.

### Keyboard
Plays as controller 1, with two keys for each console input, remappable in the settings window.

### General
- **Settings Window** - Press F1, or Back + Start on a controller, for display, graphics, game options and controls. Changes are saved to `pgr2.toml`.
- **Skip Intro** - Goes straight to the title screen.
- **Hide HUD** - Clears the race HUD for screenshots and replays.
- **Pause On Focus Loss** - The game and its sound stop while another window is in front.
- **Saves** - Profiles, options and career progress are kept on an emulated hard disk beside the exe.
- **System Link** - Plays over the LAN, or by connecting straight to a friend's IP address (`join_ip`).
- **60 FPS Races** - `race_60fps` runs races at 60 frames a second with unchanged physics (experimental).

### Miscellaneous
- **Boot Splash** - Place a `splash.png` (or `.bmp`) beside the exe to display a splash screen at launch.
- **Diagnostics** - Logs to `logs\pgr2.log`, and offers frame captures (F3) and frame logs (F4). See [WINDOWS.md](WINDOWS.md).

## Status

Boots, plays the movies and menus, races in every mode tried, saves and plays system link. Known open issues are
some remaining flicker on buildings at night, and sound effects (reverb, filters) that are not modelled. Progress is
in [PROGRESS.md](PROGRESS.md), technical findings in [FINDINGS.md](FINDINGS.md), and what may carry over to
Project Gotham Racing 1 in [PGR1.md](PGR1.md).

## System Requirements

### Minimum
- **OS**
  - Windows 10 64-bit
- **GPU** with support for Direct3D 12

## Building

### 1. Clone the Repository
Clone this repository using [Git](https://git-scm.com/).

### 2. Add the Required Game Files
Place your extracted game data files in the `assets` folder. `default.xbe` must be there, with the game's folders beside it.

### 3. Install Dependencies
Install Visual Studio 2022 with **Desktop development with C++**, **C++ Clang tools for Windows** and **C++ CMake tools for Windows** (`build.bat` offers to add the last two).

### 4. Build the Project
Run `build.bat`. The first build recompiles the game and takes a while. The exe is written to `out\build\release`.

## Credits

- [Xbox XBE Recomp](https://github.com/TGP482/XboxXBE-Recomp), the recompiler and runtime this is built on.
- [XenonRecomp](https://github.com/hedge-dev/XenonRecomp) for the overridable function layout and instruction hooks.
- [Zydis](https://github.com/zyantific/zydis) for decoding x86 instructions, [toml++](https://github.com/marzer/tomlplusplus) for the configs, [Dear ImGui](https://github.com/ocornut/imgui) for the settings overlay and [SDL](https://github.com/libsdl-org/SDL) for the window, input and controllers.
