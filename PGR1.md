# Project Gotham Racing 1

PGR1's port is a separate repository (`hk21x/pgr1-recomp`, local until it is ready) built on the same SDK line
(the fork's `main`, which this repository tracks too). This page records what is known from PGR2's side; PGR1's own
PROGRESS.md and FINDINGS.md take over once it starts.

## The disc (Harry's PAL copy, reviewed 2026-10-22)

| Item | Finding |
| --- | --- |
| Title | "Project Gotham Racing", title id 4D530003, region PAL, version 5, built 2002-02-07 |
| `default.xbe` | SHA-1 `4878c3bdbd44663b64ce64986b313e8c6b7f8917` |
| XDK | XAPILIB 3950, D3D8 3948, XGRAPHC 3911, DSOUND 3936, XBOXKRNL 3911, LIBC 3911, LIBCPMT 3911: the launch XDK, much older than PGR2's 5659; `xbe_symbols` looks for `sdk/xdk/3948/` |
| Not linked | XNET, XONLINE, XMV, D3DX8, XVOICE: no system link, no Xbox Live, no XMV movies |
| Movies | Bink (`FMV\*.bik`, RAD 1998-1999), linked into the game |
| Vertex programs | 11 `Shaders\*.xvu` files, loaded at run time |
| Data | every INI and area file in a `GOTHPOK` container (magic, uncompressed size, compressed data) |
| Content | 12 districts (Tokyo, San Francisco, London, New York) with day and night sets; 472 car files |
| Multiplayer | split screen |
| Music | its own WMA tracks, and custom soundtracks from the dashboard's database |
| Second xbe | `XDemos\XDemos.xbe`, launched from the menu |

## What carries over from PGR2

Everything in the SDK: the recompiler, kernel, D3D8 on Direct3D 12 and 11 (W-buffering, point sprites, MSAA,
ultrawide, render scale), sound, input, wheels, the achievements runtime, the settings window, the harness and its
scripts (`sdk/scripts`), the launcher and the release workflow. PGR2's own code is about 1,400 lines (`src/game`);
its methods carry over, its addresses do not.

## What is new

1. The XDK: signatures and a `layout.toml` for 3911-3950, and a check of the render state and texture stage state
   numbering against 5659 before any rendering work (states added between XDK generations renumber them).
2. Push buffers: check CreatePushBuffer / RunPushBuffer and hand-written commands first, and report.
3. A `GOTHPOK` decoder for reading the game's INIs and areas during analysis.
4. PGR1's addresses for each PC feature (skip intro, field of view, draw distance, hide HUD, ultrawide aspect,
   60 fps races, the PC Settings menu entry, wheel feel) and an achievement set.
5. Split screen (four local pads) in the harness, and custom soundtracks as a later PC feature.

## Phases

0. Shared ground (done 2026-10-22): one SDK line, shared scripts, a template with the release pieces, PGR1's skeleton.
1. Inventory in Ghidra (push buffers, the GOTHPOK decoder).
2. XDK bring-up (signatures, layout, state numbering).
3. Boot, menus, a race headless; smoke test.
4. Windows parity.
5. PC features.
6. Achievements.
7. Launcher and release 1.0.
