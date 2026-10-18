# PGR2 recomp - progress log

Newest first. Each entry: date, what changed, what was learned, what's next.

## Status
- Milestone: 4 - **a race runs under pad control** in the harness (title -> Instant Action -> Yokohama race)
- Windows: Harry's first build boots and takes input; white quad and solid-colour movies fixed in shared code since
- XDK 5659: 185 of 244 functions, 12 of 12 data
- Known: car bodies partly missing; a stale band under the HUD seen once at a race start; reference renderer
  shows dotted textures (waiting for the Windows report)
- Speed: right at 30 fps (PGR2 default); Windows shows flat brown polygons and striped textures
- Smoke test: `scripts/smoke.sh` (title -> Instant Action -> race, no traps), run before every fast-forward of main
- Upstream: draft PR description for TGP482/XboxXBE-Recomp in `.local/upstream-pr.md`, not sent
- D3D12 groundwork (no capture): raw vertex path proven against the CPU path, all race shaders compile (DXC), env map
  cube fixed; brown polygons and stripes not reproduced, see 2026-10-18 D3D12 entry for the list to check on Windows
- Sound headless: not silent, no clipping, 48 kHz, menu music, engine pitch follows the throttle; DSP effects and
  reverb are accepted but not applied
- Saves: profile and settings persist across a reboot (`scripts/savetest.sh`); a lap record needs steering
- Next: more modes, soak; F3 captures when Harry is back

## Log

### 2026-10-18 - saves

PGR2 saves profiles with XCreateSaveGame: E:\UDATA\4d53004b\<12 hex digits>\Profile.Sav (30100 bytes) and
SaveMeta.xbx; TitleMeta/TitleImage/SaveImage.xbx at the title's folder. A profile is saved when made, when leaving
Options, and when a race is quit (statistics and career fields). The first 20 bytes are a signature.
- Interactive (cmd presses and snaps), on one disk: Create New Profile (name, plate, plate text, helmet, all
  defaults) -> saved. Options > General > Game Options: Speedometer MPH -> KM/H; saved on backing out of Options;
  byte 0x844 of Profile.Sav is that option (1 MPH, 0 KM/H). Time Attack (Edinburgh, Princes Street East, MINI):
  started, quit, profile saved again. Kudos World Series: Compact Sports Series, Street Race Florence Duomo 2,
  Novice: started, quit; saved again (career fields at 0x8DC and 0xFEC..0x1002). Rebooted on the same disk: the
  title offers "Start Player's Game", Game Options shows KM/H, the race HUD shows KM/H.
- `scripts/savetest.sh` (pad scripts save1.txt, save2.txt, about 8 minutes): a first boot makes the profile and
  sets KM/H, checks the byte; a normal boot of that disk opens Game Options, checks the game read Profile.Sav and
  the byte is still KM/H, and snaps the screen (KM/H). Passes.
- Not done: a record. Every record PGR2 keeps (fastest lap, best Kudos) needs a finished lap, which needs
  steering; a fixed pad script drives blind, and steering by snapshots took a minute a step (a lap would take an
  hour). Time Attack laps start rolling on Princes Street; holding throttle hits the barrier at its end.
  The Time Attack attempt and the career start do save and reload.
- Headless only: licence plate pictures are garbled and some full screen blurs are dotted in the reference
  renderer; worth a look on Windows (plate select screen).

### 2026-10-18 - sound, checked headless

- sdk harness: `WAV=file` writes the mixed output (48 kHz 16 bit stereo, header kept current, the log's
  `audio: output started` is its first sample). The headless mixer is now pulled on a fixed 10 ms schedule (it
  slept 10 ms after each pull, so it ran slow). `scripts/wavstats.py` prints RMS, peak, clipped samples and the
  strongest frequency per interval.
- Race run (race.txt, 240 s, `.local/run/1/race.wav`): movies -18 to -25 dB RMS with silent gaps between logos;
  attract and title music -27 dB RMS, peak -17.8 dB; race -19 to -24 dB RMS, peaks to -2 dB. Clipped samples 0.
  Sample rate 48 kHz out; voices at 44.1 and 22.05 kHz resampled by their SetFrequency.
- Music in the menus: the title's music is a 44.1 kHz stream at gain 0.13 (-17.7 dB: stream headroom -6 dB and the
  game's volume); it fades to 0 once Instant Action is chosen and the race loads.
- Engine: a temporary per-voice trace (not committed) showed the engine buffer (44.1 kHz) driven by SetFrequency
  48 kHz -> 73 kHz -> 76 kHz under throttle, dropping at each gear change (to 55k, 47k, 44k) and climbing again.
  Eleven 8 kHz streams sit queued at full gain with silence (voice chat style); harmless.
- DirectSound exports still running the library's code (the runtime's .XTLID audit), named from their export ids
  (alphabetical within each interface) and Ghidra:
  - B003E IDirectSound_GetCaps (0x1ECB84): reads the library's never-initialised counters, reports 0 free voices;
    PGR2 calls it at audio init and never reads the result.
  - B00A3 XAudioCalculatePitch, B00A5 XAudioCreatePcmFormat: pure arithmetic.
  - B00AB (0x1ECFE5): a file media object for data/audio (XWaveFile style), plain file reads.
  - B0080 FlushEx, B0093 SetMaxDistance, B0094 SetMinDistance, B009A SetPosition, B008B SetDopplerFactor: the
    IDirectSoundStream thunks, each calling the CDirectSoundStream method.
  - CDirectSoundStream_SetDopplerFactor (0x1ED954) was unmatched and wrote through a null voice pointer near guest
    address 0 on each 3D stream update: now on the host (accepted), signed into 5659 (186 of 245).
- Called but only accepted, no effect in the mixer: DownloadEffectsImage and SetEffectData (PGR2 loads
  6WayDistortion.bin, 15 DSP effects), SetI3DL2Listener/Source (reverb), SetFilter (buffers and streams),
  SetVelocity and SetDopplerFactor (Doppler), SetMixBinHeadroom, EnableHeadphones. Sound is dry: no distortion,
  reverb, filters or Doppler. Not linked by PGR2: the other 26 DSOUND names the scan lists as missing.
- Needs Windows: whether it sounds right (XAudio2 path, same mixer), the music volume in menus, and the engine.

### 2026-10-18 - D3D12 groundwork without a capture

Tools (headless, nothing needs a GPU):
- `AUDIT=N` (sdk harness/audit.cpp): every Nth frame's draws as the D3D12 backend would get them. Writes each
  generated shader to `cap/hlsl/`, assembles each draw both ways (raw guest vertices as D3D12 reads them, and the
  CPU conversion the harness and reference renderer use, `XBE_RAW_VERTICES=0` in the harness build) and compares
  every corner, lists sampled textures with their mip levels checked against their parents, and the targets drawn
  to (`cap/textures.txt`, `cap/vertices.txt`). `AUDIT_TEX=<addr>` saves a texture as its guest memory decodes, which
  is what D3D12 reads (the null backend never draws into guest memory between REF frames).
- `scripts/shadercheck.sh`: compiles `cap/hlsl` with DXC (fetched into .local/dxc), HLSL 2018 rules as FXC.
- `scripts/wincheck.sh`: syntax-checks d3d12.cpp and xaudio2.cpp with clang against mingw-w64 headers (unpacked into
  .local/mingw, no root). D3D12 changes are now at least compiled before Harry gets them.

What the D3D12 path draws when something fails (read, d3d12.cpp): a shader that fails to compile logs
`d3d12: vs_5_0 compile failed` plus the source and the draw is skipped; a failed pipeline logs and skips; a missing
or uncreatable texture is a null view (samples 0, black); an unknown texture format decodes its first byte as grey.
No fallback is a flat colour, and Harry's logs have no compile or pipeline errors.

Findings, race (Instant Action, every 10th frame) and the menus plus attract races (every frame, 1.3M draws):
- Shaders: 77 vertex/pixel shaders in a race, 75 in menus and attract; all compile with DXC.
- Textures used: DXT5 (787), DXT1 (108), A8R8G8B8, YUY2 (movies), L8/AL8/LIN_L8, R5G6B5 cube, LIN_X8R8G8B8, LIN_D16.
  DXT goes to the GPU as BC1/BC3 unchanged; every mip level of every texture is within 0.04 of its parent shrunk
  (only 2x2 and 1x1 DXT levels differ more), so level offsets are right. Same decode code as the reference.
- Vertices: 334,600 raw draws, 11,646 differed from the CPU path. Cause: declarations longer than the stream stride
  (a 24 byte FLOAT3 D3DCOLOR NORMSHORT2 NORMPACKED3 layout on a 20 byte stride, decals and a second texture coordinate):
  the console reads the next vertex's bytes, D3D12 read zeros at the last vertex. Fixed (sdk a867407): the raw buffer
  includes the overhang. Now 0 of 1,303,670 differ. The two shaders involved never read the bad element, so this
  was not visible.
- Environment map: the game draws ~800 draws a frame into face 0 of a 256x256 R5G6B5 cube (0x83458000) and three
  pixel shaders add it as specular (t3.rgb * v1). D3D12 always read cubes from guest memory, which is all zeros
  (AUDIT_TEX dump): reflections were black on Windows. Fixed (sdk 8770388): faces drawn on the GPU are copied into
  the cube once a frame.
- An 8 bit L8 target (0x83AA5000, 256x256, 3 draws a frame) sampled at stage 3: the console's B8 surface keeps the
  blue byte, D3D12 keeps BGRA (alpha 0 from the clear). Now read as its blue byte. The shaders that read it use only
  rgb of grey content, so not visible. The reference renderer wrote 16 bit 565 texels into it (fixed).
- Sync/late copies wrote 32 bit linear rows over any surface: over the L8 or swizzled targets that would run past
  them. Only 32 bit linear surfaces are written back now.

Not found, needs Windows: the brown polygons and the green/magenta stripes. Every input D3D12 gets (vertices,
textures, mips, shaders) now matches the reference path headless, so what is left is the HLSL itself versus the
reference interpreter (same rules, separately written; only a GPU runs HLSL), the D3D12 state mapping (blend,
depth, stencil, samplers), upscaling, and timing (render thread). Wanted from Windows: an F3 capture with
`XBE_CAPTURE_DIR` while the fault is on screen; and whether reflections on cars and wet roads look right now.

### 2026-10-18 - smoke test, upstream draft

- `scripts/smoke.sh`: runs `scripts/pad/race.txt` headless (240 s) and fails on any trap, unimplemented export or
  guest trap in the log, when the title menu is not up before the menu choice (frames 3900-4200 under 100 draws a
  frame), when the last 300 frames average 100 draws a frame or fewer, or when frames stop. First boot: menu 11,
  race 1586 draws a frame; KEEP_HDD=1: race 2810; `PAD=scripts/pad/none.txt` fails as it should (11 draws a frame).
  CLAUDE.md: run it before every fast-forward of main.
- sdk: comments no longer name PGR2; harness README documents FPS. The branch (40e9c35..) is generic.
  `.local/upstream-pr.md` drafts the upstream PR; Harry decides when to send it.
- Rendering (brown polygons, green/magenta stripes): not started. `.local/win` has only pgr2.log, with F3 stack
  dumps and no capture: XBE_CAPTURE_DIR was not set, or its folder not copied. Every texture format used in a race
  (0x0, 0x1, 0x6, 0xC, 0xF, 0x1E, 0x24, 0x30) decodes in the headless renderer, so the fault is likely D3D12 only.
- Playtest: no notes and no CrashDumps folder; the log ends with Exit (0).

### 2026-10-18 - game speed: 30 frames a second

Harry finished a full Instant Action race on Windows; the game ran far too fast at fps_cap 60, and right at 30.
PGR2 steps races by a fixed time per frame (the frame time path at 0xD9317 takes measured swap times only in some
modes) and showed 30 frames a second on the console: it presents on the next vertical blank
(interval 0x80000001) and its frames took more than one. Measured headless: the race clock ran 1.84x at 60
(3:48 -> 3:07 in 22.3 s), 0.91x at 30 with snap overhead (4:35 -> 4:14 in 23 s).
- `src/common/game_info.cpp`: PGR2's init sets fps_cap 30 unless the settings file has an fps_cap line.
- sdk: the swap callback is called when a swap is shown (new layout key CDevice_SwapCallback, 5659 0x1DA4);
  PGR2 records frame timestamps there. Correct, but not what fixed the speed.
- harness: paces to the settings' fps_cap when FPS is unset, so it runs at 30 like Windows; pad scripts halved.
The settings file `pgr2.toml` only exists once written from the settings overlay; create it by hand to override.

Windows screenshots also show large flat brown polygons over track and sky, and green/magenta striped surfaces;
car bodies are fine on D3D12 (headless only). Needs F3 captures with XBE_CAPTURE_DIR set.

### 2026-10-18 - Windows prep, white quad, movies, menus, first race

**Windows build prep.** `.gitmodules` uses https://github.com/hk21x/XboxXBERecomp.git (SSH push URL only in the
VM's sdk config). PGR2 names: `PGR2.exe`, window "Project Gotham Racing 2", `pgr2.toml`, logs `pgr2_*.log`
(the harness log is now `.local/run/1/logs/pgr2_001.log`). `build.bat` keeps the committed config (it runs
xbe_symbols only when `config/game.toml` has no `[symbols]`). WINDOWS.md has the steps and checklist.
D3D12 review found one real gap: titles poll GetVisibilityTestResult in a loop (PGR2 0x114CC0) and D3D12 wrote
reports only at Present; D3D_SetFence now flushes when a report was asked for and Flush delivers reports.

**Harry's first Windows run**: builds, boots, takes the pad. White quad flickering over the menus, movies solid.

**White quad (shared code, D3D12 and headless).** PGR2 calls Swap(2) (copy, no finish), draws its own copy of
the supersampled back buffer to the front buffer (a white quad sampling texture 0 = back buffer, u 0..1280, plus
pillarbox bars; 0xD983D..0xD9B59), then Swap(4). The host presented on every Swap, so every other frame showed
the copy pass: an untextured white quad. Swap now honours COPY/BYPASSCOPY/FINISH: the copy draws go to a target
that is never shown, FINISH shows the back buffer, the swap count moves once a frame.

**Movies.** The decoder writes correct YUY2 (checked in memory) and YUY2 conversion existed. PGR2's movie player
gives texture coordinates in register 9 under FVF XYZRHW | DIFFUSE; my immediate-mode FVF path packed only the
FVF's fields, so every pixel sampled texel (0,0). The FVF is now widened by the slots written.
`captures/1018-0722-ref_00600.png` (Bizarre Creations), `1018-0722-ref_01200.png` (Microsoft Game Studios).

**Car bodies / direct constant uploads.** 0x1852D0, 0x185330, 0x185450 run thousands of times in the attract
demo (0x1853B0 never); their constants reach `g_state.vsc` (114..117 hold a matrix). Not the cause. Bodies are
still partly missing; left for the Windows report.

**Menus.** Three bugs stood between the title and the menus:
- XAPI init failed: MODE SENSE page 0x3E (DVD authentication) through IOCTL_SCSI_PASS_THROUGH_DIRECT was answered
  with zeros, then the signed security sector check (0x19372C). Now the page is answered and the check passes on
  the host (`XapiVerifyMediaInDrive`, our name). T: and U: are mounted, no reboot to the dashboard, no "not enough
  free blocks" dialog.
- The harness ran unpaced (thousands of frames a second): presses were milliseconds long. Paced to 60 (FPS=N).
- DirectSoundGetSampleTime (0x1EB88B, .XTLID 0xB0011) read the APU sample counter 0xFE80200C, which never moved:
  the frontend clock (0xAD280, delta at 0x2FEE14) stood still, so menu text stopped at its first letter and items
  were drawn at near zero size. Now a host function counting 48 kHz samples. Menus animate and respond.
`captures/1018-s01.png` (title, no white quad), `1018-t01.png` (title menu), `1018-t03.png` (Instant Action).

**Race.** Title -> RIGHT, RIGHT -> Instant Action -> "Welcome to Yokohama" (`1018-t04.png`) -> race start
(`1018-t05.png`) -> accelerating, lap 1 of 2 at 80 mph (`1018-t06.png`), lap 2 (`1018-t08.png`).
`scripts/pad/race.txt` does it from a clean start: `1018-0748-ref_10800.png` (in the race, lap 1).

**START during Intro2** no longer hangs: it skips to the title (`1018-0750-ref_02400.png`). The frozen sample
clock was the likely cause; not separately root-caused.

**Next.** Windows retest; the stale band under the HUD at race start (`1018-t05.png`); car bodies; dotted
textures once the Windows report says whether D3D12 shows them.

### 2026-10-17 - fork, config split, cache partition, first real drawing: attract demo and title screen

**Housekeeping.** Submodule URL fixed to git@github.com:hk21x/XboxXBERecomp.git (`.gitmodules` and the sdk's own
remote), `pgr2-xdk5659` pushed, a fresh clone of `xdk-5659-bringup` builds both tools and its scan reproduces
the config, `main` fast-forwarded. gdb is system wide now; `.local/tools` removed.

**Config split.** The build, `--sign` and xbe2pe read `config/pgr2.toml`, which includes `config/game.toml`
(xbe_symbols output, never edited) and holds every hand entry. A rescan reproduces game.toml byte for byte.
Documented in CLAUDE.md.

**Cache partition (SDK kernel, generic).**
- `InitialiseFileCache - !XFormatUtilityDrive`: XAPI's formatter (0x193A83) asks IOCTL_DISK_GET_PARTITION_INFO,
  which returned zeros, so the size check failed before any write. It does not call IoQueryVolumeInformation.
  Now a partition opened whole (`\Device\Harddisk0\PartitionN`) is a raw volume: the retail partition layout
  (Z: 750 MB), raw reads give zeros, raw writes are accepted, and a FATX header written at offset 0 empties the
  partition's host folder (`files: partition 5 formatted`).
- IoQueryVolumeInformation (76) implemented anyway: its caller is XGetFilePhysicalSortKey (0x18C31F), which the
  cache copy (0x13B770) uses to sort files. File objects get a FILE_OBJECT body with a zeroed FsContext.
- The cache copy then found nothing: FindFirstFile (0x191139) sends `*.*` as an empty name, which the runtime
  matched against nothing. An empty name now matches everything. `D:\Cache\Resource\*.*` is copied to Z:.
- NtUserIoApcDispatcher (232) trapped: the APC routine of ReadFileEx; it now calls the completion routine.
- `KEEP_HDD=1 scripts/harness.sh ...` keeps the hard disk between runs. Both paths tested: first boot formats Z:
  and copies the cache, a kept disk boots straight through; both reach the title screen.

**Movies (XMV).** Intro1 (audio), Intro2 (3 audio tracks), English (no audio, paced by the vertical blank count
via GetDisplayFieldStatus 0x1C4350), Intro3, then the attract demo. They play through without presses (about
100 s headless). **START during Intro2 hangs the player** (0x13C810 polls XMVDecoder_GetNextFrame 0x2113E8,
which keeps returning "no frame"); not root-caused, `scripts/pad/boot.txt` waits until the attract demo.
No skip hook was needed because nothing stalls unless a movie is skipped.
The movies themselves render black in the reference captures (YUY2 640x480 texture, format 0x24).

**Crashes fixed on the way (SDK, generic):**
- D3DDevice_SetDepthClipPlanes (0x1C2050, flags 3) ran library code over host-less device state and crashed in
  0x1C19D0: now a host function that accepts it (the renderer keeps the viewport depth range).
- IDirectSoundBuffer_PlayEx (0x1ECCD9) and IDirectSoundBuffer_Pause (0x1ECD15, switch 0/1/2 in 0x1F39F8) started
  or paused hardware voices in library code: now host functions.
- Headless only: the game polls GetVisibilityTestResult (0x1C3610) until the GPU report is written; the null
  backend never wrote reports. It now writes them at once with 0 pixels. D3D12 already delivered them.

**Title screen.** The frontend draws with Begin/End and FVF 0x144 (XYZRHW, diffuse, one texture). EndPrimitive
turned FVF handles into an empty program ("pretransformed immediate mode vertices are not supported"). It now packs
the slots in the FVF's layout and draws through the fixed pipeline. Captures:
- `captures/1017-2339-ref_06600.png`, `1017-2339-ref_07800.png`, `1017-2346-ref_12000.png`,
  `1017-2346-ref_72000.png`: attract demo races (Hong Kong and a European city); car bodies partly missing.
- `captures/1017-2355-ref_24000.png` (kept disk) and `captures/1017-2358-ref_24000.png` (first boot): the
  Project Gotham Racing 2 title screen.
- `captures/1017-2355-ref_40000.png`: the screen after pressing A on the title (blurred city, button glyphs).
The reference renderer shows dotted textures and white bands across UI panels; not checked against D3D12 here.

**Signatures:** 183 of 242 functions (D3DDevice_SetDepthClipPlanes, IDirectSoundBuffer_PlayEx and _Pause added;
5849 knows the names without signatures). Fallback ids: Reset FFFFFE04, SetDepthClipPlanes FFFFFE05, PlayEx
FFFFFF0D, Pause FFFFFF0E.

**Next.** The menu after the title (what the A press leads to), then a race. Look at the missing car bodies
(possibly the game's own vertex shader constant uploads, 0x1852D0 and friends, now decoded but unverified), the
START-in-Intro2 hang, and the reference renderer's YUY2 and dotted textures.

### 2026-10-17 - 5659 SDK bring-up, first presented frame

**Housekeeping.** Scaffold committed to `main` (d47ce51), `main` fast-forwarded over `xdk-5659-survey`, branch-per-task
rule added to CLAUDE.md. `sdk` submodule URL set to https://github.com/hk21x/XboxXBE-Recomp.git (`.gitmodules` and the
sdk's `origin`, with an SSH push URL because HTTPS has no credentials here). **The push of `pgr2-xdk5659` fails:
GitHub answers "Repository not found" for hk21x/XboxXBE-Recomp** (SSH authenticates as hk21x). The SDK commits stay
local until the fork exists; this branch's gitlink points at them.

**Weak matches (all 7 correct).** SelectVertexShader 0x1C0100 writes 0x1EA0 with the address; LineWidth 0x1C1320
writes 0x380 and render state 0x9D; TexCoordIndex 0x1C1670 writes texgen 0x3C0 + stage * 0x10; InitDefaultState
0x1C9280 loops SetTransform/SetRenderStateNotInline/SetTextureStageStateNotInline; SetVertexShader 0x1C03F0 stores
the shader at device + 0x790; SetTexture 0x1C3FE0 stores [stage] at + 0xF78; SetPixelShader 0x1C25C0 copies the
shader's 0x39 render states to D3D__RenderState 0x1D0740.

**D3D misses: 43 of 60 found** (my earlier "63" counted wrong; it was 60 functions + 4 data). Found by what each
writes to the push buffer, the device fields it touches and its ret size, e.g. Swap 0x1C55D0 (returns +0x2468),
Clear 0x1C2C50 (0x1D98/0x1D8C, ret 0x18), SetRenderTarget 0x1C3980, SetViewport 0x1C3DB0, SetTransform 0x1C34A0,
MakeSpace 0x1C6140, D3D_MakeRequestedSpace 0x1C6000, D3D_SetFence 0x1C5DE0, D3D_BlockOnTime 0x1C5E90,
CDevice_KickOff 0x1C5CC0 (fastcall), the render states by the D3D__RenderState index each stores (ZEnable 0x8F ...
DoNotCullUncompressed 0xA5). The other **17 are not in the image**: no caller in game code, nothing in the D3D
section's undefined gaps (the only sizeable one, 0x1C47C0, is the state query): BeginPush, EndPush, CopyRects,
DeletePixelShader, DeleteVertexShader, EnableOverlay, FlushVertexCache, GetVertexShaderFunction, IsFencePending,
LightEnable, PersistDisplay, D3DDevice_Release, SetLight, SetMaterial, SetPalette, SetTile, SetVertexShaderConstant4.
Data: D3D_DirtyFlags 0x1D0538, D3D_IndexData 0x1D0534, D3D_DeferredTextureState 0x1D0540, D3D_RenderState 0x1D0740.
Also IDirectSound_DownloadEffectsImage 0x1EBA16 (calls CDirectSound::DownloadEffectsImage 0x1EB7DC).

**layout.toml re-derived**, every value cited to a PGR2 instruction; "unverified" header removed. The CDevice tail
moved -0x10 from 5849 (miniport at +0x1C18, so the vertical blank and gamma fields too), ZScale and SuperSample
moved -4, the front fields -4 (PixelShader 0x780, VertexShader 0x790, IndexBuffer 0x79C). DSOUND and XPP offsets are
unchanged from 5849 (constructor 0x1EDE60, effects image reader 0x1EFA36, XGetDevices 0x26E033).

**Signatures.** `xbe_symbols --sign config/game.toml --out sdk/xdk/5659/signatures.toml`, then a fresh
`xbe_symbols` run reproduces the hand-filled config exactly. Two things needed for that: `[functions]` names the
CDirectSoundStream constructor so the two stream vtables get separate anchors (otherwise both anchor 0x1EC26F and
the scan drops both), and that `[functions]` line is the one thing the scan does not write back. **Note: a scan
run rewrites the whole config, so `[[hook]]`/`[[function]]` entries added later are lost if xbe_symbols runs again.**

**Walk.** `sdk/runtime/xdk/d3d8.cpp` `Walk` and `EndPush` now load vertex shader constants: 0x1EA4 sets the load
slot, 0x0B80-0x0BFC write dwords from it, each method group moves the slot on by its constants (the XDK's own
SetVertexShaderConstantNotInlineFast 0x1BFEF0 sends one load and several 16 dword groups, which needs exactly this).

**First boot.** Crashed at once in guest D3D code 0x1CB2C1: the game calls D3DDevice_Reset 0x1C38D0 (from
0x177BF0), the library's own Reset rebuilds the frame buffers through display hardware the host never set up.
Added a host `D3DDevice_Reset` (name added to the 5849 and 5659 signatures): frame buffers for the parameters
(kept when unchanged), set as the target with the device references moved, cleared. With that the game runs: CPU
threads up, ~3,100 presents a second in the harness, 0 draws. `captures/1017-2235-ref_00060.png` shows the Xbox
**dirty disc error** ("There's a problem with the disc you're using..."). A breakpoint on the error routine
0x1388F0 (called from 0x13DD77 in 0x13DCE0) gives the message `InitialiseFileCache - !XFormatUtilityDrive`: with
no `Z:\Version.12` the game formats its cache partition and XFormatUtilityDrive returns FALSE. No trapping stubs
were hit. Boot log also warns that kernel imports 76 IoQueryVolumeInformation, 232 NtUserIoApcDispatcher and
343 XcPKGetKeyLen are unimplemented.

**Tooling notes.** `sdk/harness/*.sh` are mode 644 upstream, so `scripts/harness.sh` runs them through `bash`.
No gdb on the VM and no sudo: `.local/tools/gdb.sh` is gdb 15.1 extracted from the Ubuntu packages. REF=60 at
3,100 frames a second wrote 5.6 GB of TGAs in 120 s; use a much larger REF (or `snap`) until frames are paced.

**Next.** Find why XFormatUtilityDrive fails (it is guest XAPI code: trace its kernel calls, starting with the
unimplemented IoQueryVolumeInformation), or give the harness hard disk a formatted cache partition. Then the boot
sequence onward to the menu.

### 2026-10-17 - XDK survey, 5659 seed, push buffer check

**Tools.** `scripts/linux-tools.sh` builds `xbe_symbols` and `xbe_recomp` cleanly.

**Library versions** (`xbe_symbols assets/default.xbe sdk/xdk`), title id 0x4D53004B:
XAPILIB, D3D8, D3DX8, XGRAPHC, DSOUND, XBOXKRNL, XMV, XONLINES, XVOICE, LIBCMT, LIBCPMT, all 1.0.5659.
There is no separate XNET library entry; XNet is linked as part of XONLINES (sections XONLINE and XNET).
The XBE has no BINK section and no Bink strings: movies are XMV (42 `.xmv` files).

Sections: `.text 11000-1BF7FC`, `D3D 1BF800-1D2EDC`, `D3DX`, `XGRPH`, `DSOUND`, `WMADEC`, `XMV`, `XONLINE`,
`XNET`, `XPP 26C740-2755E8`, `.rdata`, `.data`, `DOLBY`, `XON_RD`, `.data1`, `$$XTIMAGE`, `$$XSIMAGE`, `.XTLID` (352 ids).

**5659 seed.** The first run stopped (only 5849 signatures). Created `sdk/xdk/5659/` from copies of
`sdk/xdk/5849/{layout,signatures}.toml` and re-ran. Result: **135 of 238 functions, 8 of 12 data**.
- 24 found only by `.XTLID` id (Direct3D_CreateDevice, 4 IDirectSound*, all the XNet/Winsock and XOnlineStartup/Cleanup ones).
- 12 found by constants. Low scores to double check in Ghidra: D3DDevice_SelectVertexShader 0x1C0100 (0.60),
  SetRenderState_LineWidth 0x1C1320 (0.60), SetTextureState_TexCoordIndex 0x1C1670 (0.65), D3D_InitDefaultState
  0x1C9280 (0.67), SetVertexShader 0x1C03F0 (0.75), SetTexture 0x1C3FE0 (0.76), SetPixelShader 0x1C25C0 (0.78).
  1.00: DrawIndexedVertices 0x1C09B0, DrawVertices 0x1C0910, SetRenderState_StencilFail 0x1C2210; 0.88: SetPixelShaderConstant 0x1C27C0, SetShaderConstantMode 0x1C01A0.

Misses (left at 0 in `config/game.toml`):
- Not applicable (no Bink in this game): BinkDoFrame BinkGetSummary BinkNextFrame BinkWait
- D3D (60, first counted as 63 by mistake): CDevice_KickOff D3D_BlockOnResource D3D_BlockOnTime D3D_KickOffAndWaitForIdle D3D_MakeRequestedSpace D3D_SetFence
  D3DDevice_BeginPush BlockOnFence BlockUntilVerticalBlank Clear CopyRects DeletePixelShader DeleteVertexShader DrawVerticesUP
  EnableOverlay End EndPush FlushVertexCache GetBackBuffer2 GetDepthStencilSurface2 GetRenderTarget2 GetVertexShaderFunction
  InsertFence IsFencePending LightEnable LoadVertexShader MakeSpace PersistDisplay Release SetFlickerFilter SetGammaRamp
  SetIndices SetLight SetMaterial SetPalette SetRenderState_{DoNotCullUncompressed Dxt1NoiseEnable MultiSampleAntiAlias
  MultiSampleMode MultiSampleRenderTargetMode OcclusionCullEnable PSTextureModes RopZCmpAlwaysRead RopZRead StencilCullEnable
  StencilEnable TextureFactor YuvEnable ZEnable} SetRenderTarget SetScissors SetScreenSpaceOffset SetSoftDisplayFilter
  SetTextureStageStateNotInline SetTextureState_BumpEnv SetTile SetTransform SetVertexShaderConstant4 SetViewport Swap
- DSOUND (29): CDirectSoundStream_SetFormat CDirectSoundStream_SetVelocity DirectSoundUseFullHRTF DirectSoundUseLightHRTF
  IDirectSoundBuffer_{GetCurrentPosition Lock SetAllParameters SetI3DL2Source SetMaxDistance SetMinDistance SetOutputBuffer
  SetRolloffCurve StopEx Unlock} IDirectSoundStream_SetEG IDirectSound_{CommitEffectData CreateSoundBuffer DownloadEffectsImage
  GetOutputLevels GetSpeakerConfig SetDistanceFactor SetDopplerFactor SetOrientation SetPosition SetRolloffFactor SetVelocity}
- XNet/Winsock (6): WSARecvFrom accept getsockname getsockopt listen send (may simply not be linked)
- CRT (7): crt_atan crt_cos crt_log crt_log10 crt_sin crt_sqrt crt_tan
- Data (4): D3D_DeferredTextureState D3D_DirtyFlags D3D_IndexData D3D_RenderState
  (DirtyFlags/RenderState are read from SelectVertexShader/SetPixelShader, whose bytes did not match)
Found data: D3D_Device 0x1D09E0, D3D_pDevice 0x1D09D8, D3D_Streams 0x1CF630, D3D_SimpleRenderStateEncode 0x2759D0,
DSound_StreamVtable 0x283F70 / 2 0x283F60, XPP_GamepadType 0x26C7F8, XPP_MemoryUnitType 0x26C778.

Two misses identified on the way (not yet put in the config):
- 0x1C6140 is the push buffer space request (MakeSpace / D3D_MakeRequestedSpace): every inline push compares
  `[D3D_Device]` with `[D3D_Device+4]` and calls it, e.g. 0x1852E3 (game) and in D3D 0x1C5290, 0x1C7390.
- 0x1C34A0 is D3DDevice_SetTransform: copies 16 dwords to `pDevice + 0xC50 + type*64`, ors a dirty bit into
  0x1D0538 (likely D3D_DirtyFlags). **5849 has CDevice_Transforms = 0xC60**, so the CDevice layout has moved
  in 5659 and `sdk/xdk/5659/layout.toml` must be re-derived entry by entry (header comment added saying so).

**Ghidra.** `xbe2pe.py` -> `.local/game.exe` (135 exports), `analyzeHeadless ~/ghidra-projects pgr2 -import` OK (~3 min).
Helper scripts in `.local/ghidra_scripts/` (PushBufferScan, DeviceRefs, Decomp; git-ignored).

**Push buffers.**
- No `D3DDevice_CreatePushBuffer`/`CreateFixup`: nothing anywhere builds a resource header with type
  push buffer (0x20001) or fixup (0x60001). The only push buffer type tests in D3D are generic resource code:
  0x1C3360 (Register: keeps a push buffer's address unmasked, called from 10 game sites for XPR data),
  0x1C3090 (resource teardown).
- No `RunPushBuffer` found: no D3D function reads a push buffer object and emits an NV2A jump/call to it.
- The 30 XPR0 bundles in `assets/` hold 222 resources, all textures (type 4); no push buffer resources.
  The `.pak*` archives were not inspected.
- **The game does write hardware commands itself.** Four `.text` functions push straight into the device
  push buffer, inlining MakeSpace: 0x1852D0, 0x185330, 0x1853B0, 0x185450 (only `.text` code touching
  `[0x1D09E0]`/`[0x1D09E4]`). Each writes `0x41EA4, slot` (SET_TRANSFORM_CONSTANT_LOAD) then `0x400B80`/`0x800B80`
  (16 or 32 dwords of SET_TRANSFORM_CONSTANT), i.e. hand-rolled vertex shader constant uploads (16/32 floats,
  e.g. matrices), and clear bits in a game global 0x49DFD8. Callers not yet traced.
  The runtime's `Walk` (`sdk/runtime/xdk/d3d8.cpp`) only decodes immediate-mode vertex methods and
  begin/end; it **ignores 0x1EA4/0x0B80**, so these constants would be silently lost and geometry using them
  would draw with stale constants. Needs a decision (Walk support or X86_FUNC overrides of the 4 functions).
- Other `.text` hits for method-header-shaped immediates were checked and are colours/flags, not commands.

**Repo notes.** `sdk/xdk/5659/` is committed in the `sdk` submodule only locally: the submodule's remote is
upstream TGP482/XboxXBE-Recomp, so the superproject's gitlink was left at 40e9c35 to avoid pointing at an
unpushed commit. Harry needs to decide where SDK commits are pushed (fork of the SDK, or vendor it).

**Next.** Verify the low-score constant matches, find the remaining D3D misses (starting with MakeSpace
0x1C6140, SetTransform 0x1C34A0, Swap, Clear, SetRenderTarget, SetViewport) and re-derive each `layout.toml`
offset from 5659 code; then decide how to handle the 4 direct constant pushers; then recompile and first boot.
