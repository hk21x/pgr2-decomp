# PGR2 recomp - findings

What the work so far has established, by topic. PROGRESS.md has the dated log with the evidence for each item.
Each item is marked:
- **[console]**: how the Xbox, its XDK or the runtime behaves. It holds for any title and the fix is in the SDK.
- **[PGR2]**: the game's own code and data. Addresses are PGR2's (XDK 5659, title id 0x4D53004B).

## The image

- **[PGR2]** Releases: the North American `default.xbe` (region NA+Japan, version 5, SHA-1 ec6e902f...) differs from
  the PAL one (region rest of world, version 2, SHA-1 676895b8...) in 531 bytes, all in the signature (0x4-0x103)
  and the certificate (time stamp, region, ratings, version, signing keys). Every section is byte for byte the same,
  so every address in this project holds for both; the NTSC disc runs the smoke test unchanged. The signing keys
  differ, so a save signed by one release may not be accepted by the other.
- **[PGR2]** XDK 1.0.5659 for every library (XAPILIB, D3D8, D3DX8, XGRAPHC, DSOUND, XBOXKRNL, XMV, XONLINES, XVOICE,
  LIBCMT, LIBCPMT). XNet is linked inside XONLINES. The movies are XMV (42 files); there is no Bink.
- **[PGR2]** Push buffers: there is no CreatePushBuffer or RunPushBuffer, and the XPR bundles hold only textures.
  Four game functions push vertex shader constants by hand (0x1852D0, 0x185330, 0x1853B0, 0x185450). The runtime's
  `Walk` decodes those commands (0x1EA4 load slot, 0x0B80 constants), so this is handled.
- **[console]** 5659 needed its own `layout.toml`. The CDevice tail moved -0x10 from 5849, ZScale and SuperSample
  moved -4, and the front fields moved -4. Every offset is cited to a PGR2 instruction. Signatures:
  191 of 250 functions, 13 of 13 data.
- **[console]** A function the scan cannot match is left at 0 and becomes a trapping stub. A library function that
  runs its own code against device state the host never set up crashes or silently does the wrong thing. Every
  one of these was found when the game called it, then moved to the host and signed in:
  Reset, SetDepthClipPlanes, SetRenderTargetFast, SetVertexShaderInput(Direct), PresentInterval,
  XapiVerifyMediaInDrive, PlayEx/Pause, SetDopplerFactor, XNetGetConnectStatus, XNetUnregisterInAddr.

## Boot, files, saves

- **[console]** Cache partition. XAPI's formatter asks IOCTL_DISK_GET_PARTITION_INFO; whole partitions are now raw
  volumes. `FindFirstFile("*.*")` sends an empty name, which must match everything. NtUserIoApcDispatcher (the
  ReadFileEx completion) and IoQueryVolumeInformation (file sort keys) are now implemented.
- **[console]** The DVD authentication mode page (MODE SENSE 0x3E) must be answered, or XAPI init fails.
- **[console]** DirectSoundGetSampleTime reads an APU counter that never moves on the host. PGR2's frontend clock
  ran from it: menus froze at their first letter. It is now a host count of 48 kHz samples.
- **[PGR2]** Profiles are at `E:\UDATA\4d53004b\<id>\Profile.Sav` (30100 bytes, a 20-byte signature first). They
  are saved on creation, on leaving Options and on quitting a race. Byte 0x844 holds MPH/KM/H.
  `scripts/savetest.sh` checks this.

## Frame pacing and speed

- **[console]** Titles pace themselves with the present interval (0x80000000 + vblanks), and the runtime now paces
  every Swap by it. PGR2 runs its menus at 60 (interval 1) and its races at 30 (interval 2), as on the console.
  Pacing by `fps_cap` alone ran races at double speed.
- **[console]** Swap honours COPY / BYPASSCOPY / FINISH. PGR2 draws its own copy of the supersampled back buffer
  between Swap(2) and Swap(4); presenting at every Swap showed that copy pass as a flickering white quad.
- **[PGR2]** The frame step is FUN_000D90F0: dt = interval / refresh, and physics takes fixed 1/60 steps
  (FUN_000F5D60). The interval setter is 0x177C70. `race_60fps` clamps 2 to 1, so a race at 60 runs the same
  simulation as at 30 (an experiment, off by default).

## Rendering: console behaviours the runtime now matches

- **[console] Pushed commands keep their order.** Raw commands a title pushes (constants, visibility tests) are
  read at the next draw. Every state and constant setter must first read what is pending, or a later direct set
  is overwritten by an earlier push. That bug put the player's car body off screen.
- **[console] W-buffering (D3DZB_USEW, ZENABLE 2).** PGR2 uses it on nearly every draw. Depth is the pixel's own w,
  interpolated perspective correct, plus the polygon offset in w units, stored as floor(w) in 24-bit units. xemu
  interpolates w linearly on the screen, which streaks long triangles. A Z buffer lost the showroom car's panels.
  - Vertex programs carry w already scaled to depth units, in their own matrices.
  - Fixed-function and pre-transformed (XYZRHW) vertices are scaled by the XDK by ZScale / far w:
    - SetTransform keeps 1 / far w of the projection.
    - 0x1C0FC0 folds the scale into the fixed-function matrix.
    - 0x1C0300 puts it in the pass-through program's c0.w.
  - SetTransform is on the host, so the runtime now computes this itself. Without it, lamp glows and their
    visibility probes showed through buildings.
- **[console] A depth buffer bigger than the colour target serves it.** The console uses the depth buffer's top-left
  part. PGR2 renders the showroom car into a 500x200 texture with the 640x480 depth buffer bound; dropping depth
  for the size mismatch let the far wheel show through the body.
- **[console] SetVertexShaderInput(Direct)** replaces the stream sources and the declaration for the draws that
  follow. Without it, PGR2's fences, lamp posts and signs read packed shorts as floats and vanished.
- **[console] SetRenderTargetFast** must switch targets on the host. PGR2 draws its six reflection cube faces with
  it. The faces drawn on the GPU are copied into the cube once a frame.
- **[console] Visibility tests.** Titles poll GetVisibilityTestResult inside a frame, so D3D_SetFence flushes when a
  report was asked for. In PGR2 a light glow's brightness comes from a visibility probe: an XYZRHW quad drawn
  between Begin and EndVisibilityTest (0x114FA0 / 0x114D20, 0x1C42F0).
- **[console] Other fixes:**
  - Immediate mode under an FVF keeps every slot written. The movies' texture coordinates are in slot 9.
  - A declaration longer than the stride reads the next vertex's bytes.
  - 8-bit targets keep their blue byte.
  - Only 32-bit linear surfaces are written back.

## Rendering: PGR2's own techniques

- **Trees** are point sprites: a vertex program sets each point's size (oPts, console pixels, 2 to 64) and the sprite
  supplies the texture coordinates; drawn as single pixels until 2026-10-21, every tree looked bare.

- **Car shadow.** The car's 1024x1024 D16 shadow map is projected onto a ground mesh built each frame
  (vs B0101931) with a polygon offset and no depth writes. With a Z buffer it failed the road's depth test by up to
  4096 units and flickered; `src/game/draw_filter.cpp` drew it 4096 units nearer. Since W-buffering that nudge is a
  quarter of a metre and put the shadow on the car's lower body in the sun, while the plain pass looks right: the
  nudge is off (`PGR2_SHADOW_NEARER=1` brings it back).
- **Polygon offsets** match the XDK exactly at 3x upscale. The layers that rely on them are road shadows, kerbs and
  decals.
- **Light pool under the car.** This is a 256x256 target drawn each frame, sampled in stage 3 by the road. The
  under-car flicker at night was much improved by W-buffering.
- **Reflections.** A 256x256 R5G6B5 cube (six faces via SetRenderTargetFast) is added as specular. Bump-mapped
  reflection shaders also sample a 256x256 reflection target.
- **Combiner modes.** PGR2 sets stage 0 through its own mode switch FUN_00105ED0 (cache 0x3F29A4): mode 1 is
  MODULATE(diffuse, texture) and mode 5 is SELECTARG1(texture). The exhaust backfire flames (FUN_00021620 via
  FUN_00021CD0, car +0x1508, 'EXHL'/'EXHR') leave mode 5 set. Draws with a pixel shader are not affected.
- **Car lighting.** The body's sun term is c56 = sun colour x the car's brightness (0x1F036). The brightness eases
  to 0.25 per wheel standing on an unflagged track triangle (bit 7 of the triangle's flag byte means shade). The
  data, the query and the test all agree with the console's code.
- **Widescreen.** PGR2 reads XGetVideoFlags: widescreen renders anamorphic 16:9 into 640x480, with a wider field of
  view and the HUD moved out. It has no HD mode. More pixels come from the runtime's render scale.

## Time of day and weather

- **[PGR2]** Conditions are picked in FUN_000F14E0 (race setup) from per-city tables, after a rand() call. Night is
  0x383188 = 1 and weather is 0x383184 (2 = rain/overcast). Night in Instant Action is a normal random roll on the
  console too.
- Each city has three lighting sets (`Game\Areas\<city>.pak_day`, `pak_overcast`, `pak_night`), chosen at area load
  (0x116C20). The log line `pgr2: area loads with time of day T, weather W` shows the choice. `PGR2_NIGHT=1`
  forces night (an experiment).

## Xbox Live content

- **[PGR2]** Content packs are listed from T:\$C\<offer id> by XAPI's XFindFirstContent (0x192134) with a category
  mask: the disc build asks for 1 (car packs, 0x157180) and 2 (city packs, 0x1572F0: the pack's cities.ini and
  Routes.ini); the Paris and Long Beach booster packs are category 0x10 (ContentMeta.xbx +0x30), made for the title
  update (T:\$U, version 0x202, a different build). Each pack's header is checked against a keyed signature
  (0x19144B / 0x191366) and each file's against the metadata's table (0x156F00); community copies are unsigned
  until a console installs them. PGR2_DLC=1 lists them, passes the checks and puts the pack's object paks, audio
  files and ambience lines where the disc build reads them (src/game/dlc.cpp).
- **[PGR2]** What the disc build cannot take from the booster packs (title update data): object paks in the newer layout
  (a zlib compressed WMSH chunk first, where the disc's open with an INDX index: read as garbage meshes), materials
  whose bucket is past the city draw list's 106 (0x11B210, table 0x457188: a null write), and night or overcast sets
  (the packs hold the day set). PGR2 draws an object it cannot set up with its error material, pixel constant 0 magenta.
- **[PGR2]** A fatal error goes through 0x1388F0(message): it formats the cache partition and stops; look for its
  caller (Audio::Manager::Setup when a city has no stereo.ini line).

## System link

- **[console]** XNetGetConnectStatus must answer CONNECTED. The library's own version answers LOST when its network
  object is null, so PGR2 flagged the host as leaving and dropped it ("lost connection").
- **[console]** Loopback instances on Linux shift ports below 1024 up 20000 (PGR2 binds 1000 to 1002).
- **[PGR2]** The lobby, the race and the results work with two headless instances (`scripts/linktest.sh`).

## Sound

- **[console]** The software mixer plays everything PGR2 uses: streams, ADPCM, the engine via SetFrequency, and
  the music fades.
- Not modelled (accepted, no effect): the DSP effects image, I3DL2 reverb, filters and Doppler. Sound is dry.

## Lessons on working

- Game code registers through an explicit call from `GameInfo::init`, never a static initializer: the Windows exe
  dropped one.
- Leftover environment variables from earlier experiments cause false regressions. The log now lists every `XBE_*`
  and `PGR2_*` variable at startup.
- Captures flush the GPU every frame and never caught a timing-dependent flicker. The frame log (`XBE_FRAMELOG`, F4)
  records at full speed and found the one-frame state changes.
- The CPU reference renderer and the D3D12 backend are written separately. A fault in both (the showroom depth) is
  in shared state handling; a fault in one only is in that renderer.
- When a draw writes nothing, check the inputs before the shaders: vertex format (SetVertexShaderInput), constant
  order (pushed vs direct), target and depth sizes, and the depth mode.

## Still open

- Night building flicker (Edinburgh) and the remaining flicker on Windows.
- The CPU reference renderer's random spike pattern in single snaps.
- START during the second intro movie hung the player earlier. It now skips, but this was not root-caused.
- Sound effects (reverb, filters) are not modelled.
