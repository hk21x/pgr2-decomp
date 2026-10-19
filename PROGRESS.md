# PGR2 recomp - progress log

Newest first. Each entry: date, what changed, what was learned, what's next.

## Status
- Milestone: 4 - **a race runs under pad control** in the harness (title -> Instant Action -> Yokohama race)
- Windows: Harry's first build boots and takes input; white quad and solid-colour movies fixed in shared code since
- XDK 5659: 187 of 246 functions, 13 of 13 data
- Known: car bodies partly missing; a stale band under the HUD seen once at a race start; reference renderer
  shows dotted textures (waiting for the Windows report)
- Speed: paced by the game's own present interval (menus 60, races 30, as on the console); fps_cap 60 default;
  `race_60fps` option (experimental, off) runs races at 60
- Smoke test: `scripts/smoke.sh` (title -> Instant Action -> race, no traps), run before every fast-forward of main
- Upstream: draft PR description for TGP482/XboxXBE-Recomp in `.local/upstream-pr.md`, not sent
- D3D12 groundwork (no capture): raw vertex path proven against the CPU path, all race shaders compile (DXC), env map
  cube fixed; brown polygons and stripes not reproduced, see 2026-10-18 D3D12 entry for the list to check on Windows
- Sound headless: not silent, no clipping, 48 kHz, menu music, engine pitch follows the throttle; DSP effects and
  reverb are accepted but not applied
- Saves: profile and settings persist across a reboot (`scripts/savetest.sh`); a lap record needs steering
- Coverage: pad scripts for Time Attack, Kudos World Series, Arcade, replay; 60 minute soak clean
- System link: two harness instances find each other and exchange the join, then the session stalls (open)
- Windows round 2: boots, movies, audio, saves OK; player car body black, stripes on some walls (captures in
  .local/win, see 2026-10-18 car bodies entry); environment map cube faces fixed (SetRenderTargetFast)
- Car body: fixed (pushed constants applied in order), headless matches the console; Windows to confirm
- Flicker: car shadow fixed (draw filter, src/game/draw_filter.cpp); buildings still flicker, some barriers missing
- Next: building flicker and missing barriers; system link waits for Harry (three attempts used)

## Log

### 2026-10-19 - car shadow: depth test kept, 4096 units nearer

Without the depth test the car shadow was steady but showed through the car's rear (Harry's screenshot). Measured
headless with the new REF_ZOWNER (for each pixel the pass failed: which shader wrote the depth that stopped it, and
by how much): road (B0001B11) and kerbs (B0001CC1) at most 4096 units of the 24 bit buffer, the car body (B00019E1)
4096 to 65536. The filter now keeps the test and draws the pass 4096 units nearer (gpu::DrawAdjust::depth_bias): the
road and kerb failures are gone, the car still hides it. Main depth buffer is LIN_D24S8 (0x2E), the shadow map
LIN_D16 (0x30): no float depth involved. Smoke OK.
Next: Harry checks the shadow under the car; the missing fence (capture 124828 against the YouTube frame).

### 2026-10-19 - car shadow flicker fixed on Windows

Harry: car shadow right and steady, cone shadows fine (08118bf). Still open, both older than this work: buildings
flicker, and some barriers do not show.
Next: a clip and a capture where a barrier is missing; bisect the building flicker with XBE_DRAW_SKIP (candidates:
B0101B41 blended with polygon offset -1, building decals).

### 2026-10-19 - car shadow fix did not run on Windows: static initializer

Harry's run of 56c3109: flicker as before, and no "car shadow pass" line in the log: the filter was never set. It
was registered by a static initializer in src/game/draw_filter.cpp, a file nothing else references; it ran in the
harness (it matched with and without the render thread) but not in the Windows exe. Now game_info.cpp's init calls
InstallDrawFilter(), which logs "pgr2: draw filter installed". Smoke OK.
Lesson: game code registers through an explicit call, never a static initializer.

### 2026-10-19 - flicker fixed (pending Windows): car shadow pass without the depth test

Harry's bisection: XBE_DRAW_SKIP=B0101931 (the car shadow projection) made the flicker almost vanish;
XBE_DRAW_TEST=B0101931:depth the same; :alphakill much worse. The pass draws a ground mesh around the car that the
game builds each frame (flat, about 10 x 11 m at road height, reaching behind the camera: negative w), with the
car's shadow map compared in stage 0, polygon offset, no depth writes, multiply blend. Headless, as many of its
pixels failed the depth test as passed, most by far more than any offset; with the test off one frame in three
drew a band across the screen in the CPU renderer (which clips differently from D3D12), not seen on Windows.
- sdk: gpu::g_draw_filter, a hook for a title's code to drop a draw's depth test or the draw (D3D12 and the CPU
  renderer).
- src/game/draw_filter.cpp: draws with a depth texture in stage 0, polygon offset, no depth writes and a multiply
  blend go without the depth test. Matches only that pass (logged once per shader). Smoke OK.
Next: Harry checks the flicker and that the car shadow is right (not drawn over the car or barriers).

### 2026-10-19 - flicker: the polygon offset scale was right; layer bisection switch

Harry: no flicker while stationary; less with XBE_ZBIAS=10; most visible in sunlit areas. Capture 105745 lists
the targets: every one is 3x (main depth 640x480 -> 1920x1440, D24S8 as on the console). The offsets match the
XDK at 3x exactly (ZBIAS n gives units -n, slope -n/4, times 3: -1/-0.75, -9/-6.75, the shadow map's 8 -> 24), so
the slope scale is not lost; the earlier reading was wrong. Sunlit, moving, helped by more offset: the layers on
the road that rely on a polygon offset (multiply passes 00000042 with -9 and B0101C41 with -1, the shadow map
projection B0101931, blended B0101B41 decals with -1) are the suspects.
- sdk: XBE_DRAW_SKIP="h1 h2": draws with those vertex shader handles left out, to find the flickering layer.
Next: Harry's runs skipping one layer at a time.

### 2026-10-19 - flicker: car shadow and road shadows; polygon offset experiment

Harry's clean run: car body right with the render thread on (the earlier break was the leftover XBE_DRAW_TEST),
flicker unchanged. Clip .local/win/flicker-example.mp4 and five 6-frame captures (102016-102051, none caught a
flicker frame). In the clip, at the green corner after the KWS start, the road's shadow pattern and a yellow kerb
wedge swap between states from one game frame to the next, the same spot where headless showed it.
Findings: the 256x256 target 83AA5000 is a light pool (sprite 83A2AB80 is a white blob), stable; the 1024x1024
D16 shadow map holds the car from the sun, identical in every frame; matched road draws keep their textures and
camera constants (only c54, a per piece shade flag, and c71-74, the light pool projection, change between them).
Headless REF_ZEPS (a depth tolerance) did not reduce pops, so it is not z-fighting between road and kerb there.
The shadow layers (car shadow multiply vs B0101931, building shadow decals vs B0101C41) rely on a polygon offset
(units -1, slope -0.75); on Windows the main depth buffer's offset slope printed unscaled (-0.75), which means its
host and guest sizes matched: either it is not upscaled, or the scale is lost. With a 3x render the slope should
be 3x. Added: XBE_ZBIAS=N (scales every polygon offset) and the target list (guest and host sizes) at the top of
each capture's frame.txt, to settle this on Windows. Harness: REF_DRAWLOG=2 logs constant changes, REF_ZEPS.
Next: Harry's run with XBE_ZBIAS=3 and 10, and one capture to read the target sizes.

### 2026-10-19 - Windows build fix: GetEnvironmentStringsA

Harry's build failed in system_info.cpp: the Windows SDK has no GetEnvironmentStringsA under UNICODE (its narrow
function is plain GetEnvironmentStrings), mingw has both, so wincheck passed. sdk: the wide block instead.
scripts/wincheck.sh now defines UNICODE, _UNICODE and _CRT_SECURE_NO_WARNINGS as the Windows build does (it then
reproduces the failure on the old code) and checks every runtime source that includes windows.h by default (13).

### 2026-10-19 - "car broken again" was a leftover XBE_DRAW_TEST

Harry's three 6-frame captures (.local/win/095552_*, 095709_*, 095805_*): in the first two the body draws (vs
B01019E1) ran with zen 0, zwrite 0, cull 0 and per-draw images were saved: XBE_DRAW_TEST and XBE_CAPTURE_DRAWS from
the 2026-10-17 experiments were still set in his environment. The body's shader handle is B01019E1 with the render
thread and B00019E1 without it (memory layout), so XBE_RENDER_THREAD=0 only seemed to fix the car: the experiment
stopped matching. No code fault. The captures caught no flicker event (a few hundred scattered pixels).
- sdk: the log lists every XBE_* and PGR2_* environment variable at startup.
Next: a clean run (no leftover variables) and a new clip of the flicker.

### 2026-10-19 - flicker: Harry's videos analysed; state order; multi-frame capture

Harry's 30 and 60 fps videos (.local/win/30fps.mp4, 60fps.mp4; car body right in both, also at 30). One-frame
pops in almost every frame; in the large ones the car's shadow on the road vanishes for a frame and a dark
translucent rectangle with a hard vertical edge darkens part of the screen (sky and far buildings, right half).
Headless (KWS Florence race, frame pairs and 6-frame runs with the new SNAP_FRAMES): similar one-frame artefacts
with and without the render thread, so not a threading race. The new REF_DRAWLOG lists each draw (pixels, box,
state, textures): the road's lightmap stage samples a 256x256 shadow texture (surface 03AA5000) filled each frame
by a full quad and two sprite quads; a 1024x1024 D16 shadow map is applied by a multiply pass (vs B0001931);
six 256x256 reflection faces are drawn each frame. Textures of matched draws do not swap between frames.
- sdk 72975c8: every state setter reads pending pushed commands first (XBE_STATE_WALK=0 to compare), as the
  constant setters did since the car body fix. Headless pops over 8 six-frame runs: 121,688 pixels with it,
  171,398 without (fewer, not gone). Smoke OK.
- XBE_CAPTURE_FRAMES=N: F3 captures N frames in a row, so a bad frame can be diffed with its neighbours on D3D12.
- XBE_RENDER_THREAD / XBE_RAW_VERTICES environment overrides for Windows A/B.
Next: Harry's 6-frame capture over a flicker; his A/B with XBE_STATE_WALK=0 and XBE_RENDER_THREAD=0.

### 2026-10-18 - checked: does a capture leave state that could whiten walls?

Read the D3D12 capture path: a capture's saves blit and then Flush (Submit + wait, command list reset, Begin).
Begin re-sets heaps and root signature and forgets the bound targets; every draw allocates its own descriptor
table and upload space and sets topology, blend factor and stencil ref, so nothing cached survives the reset.
No state leak found: the white walls Harry saw are probably not a capture artefact. To settle it on Windows:
look at the same spot with no XBE_CAPTURE_DIR set and no capture taken.

### 2026-10-18 - option: races at 60 frames a second (experimental, off)

`race_60fps = true` in pgr2.toml (or PGR2_RACE60=1 headless) runs races at 60. src/game/hooks.cpp overrides the
interval setter 0x177C70 (thiscall, interval 0/1/2; stores 0x80000000 + n in D3D's global and n in 0x4860F1):
asked for 2, it passes 1. The frame step 0xD90F0 then gives dt 1/60 and physics takes one 1/60 step a frame, so
the simulation is the one the console runs at 30, sampled twice as often.
- Evidence (headless): the race presents 60 a second, interval stays 0x80000001; the start countdown ran 19 s in
  22.2 s wall with two CPU snaps (the snaps cost more at 60). A replay recorded at 60 plays back (the result
  screen's background replay moves). Not checked: replay and ghost speed against a 30 fps recording (0x179F0
  compares a ghost sample step with dt), Kudos timing, system link with one side at 60.
- Smoke OK with the option off (the default).
Next: Windows checks (pacing, car body, stripes, race_60fps feel and replays).

### 2026-10-18 - frame pacing by the game's present interval

The runtime now paces each Swap by the presentation interval the game set: 60 / vblanks a second, within fps_cap.
PGR2 asks for one vblank (0x80000001) in the movies and menus and two (0x80000002) in races, so menus run at 60 and
races at 30, as on the console, and the forced fps_cap 30 default is gone (game_info.cpp has no init any more).
- sdk 3889abd: data key D3D_PresentInterval (5659 0x1D093C; the swap helper 0x1C5290 reads it at 0x1C534A),
  signed on an anchor; round trip reproduces game.toml. fps_cap 0 now means "only the interval". The log has
  `d3d: present interval XXXXXXXX` at each change.
- Harness pad scripts count vertical blanks (each presented frame adds the vblanks it asked for), not frames.
  All pad scripts remapped: unchanged before the movies (the first boot presents at most 30 a second there by
  itself, also unpaced), doubled after. The title now needs START twice (attract -> PRESS START -> menu); a second
  START 150 vblanks after the first. smoke.sh, savetest.sh and soak.sh read the vblank count.
- Evidence: smoke OK (menu 11 draws a frame, race 2009); savetest OK; race clock 18.7 s of lap time in 20.1 s wall
  with two snaps (0.93x, the snap cost, as 0.91x before). Not re-run: arcade, kws, timeattack, replay, soak.
- Windows: if pgr2.toml has `fps_cap = 30`, set it to 60 (or delete the line) or the menus stay at 30.
Next: an option for races at 60 (the interval clamped to 1; see the research entry for what to check).

### 2026-10-18 - player car body fixed: pushed constants applied out of order

The missing body (only wheels, driver and plate) reproduced headless with the car standing on the grid (earlier
headless frames had the car moving). The reference renderer's per draw counts showed the body draws' triangles
cover no pixel: off screen. An event trace before the first body draw: the game pushes c14-c25 as raw commands
(SET_TRANSFORM_CONSTANT_LOAD 110 and 118, the world view-projection), then sets the car's own matrices with
SetVertexShaderConstantNotInlineFast(110, 16). The HLE wrote those at once, the pushed packets were read by Walk at
the next draw and overwrote them: every body draw used the world matrix with object space vertices.
Fix (sdk 9688e51): the constant setters and SetVertexShader read pending pushed commands first, so order is as on
the console. The grid frame now shows the TVR Tamora in its dark teal paint, as Harry's console screenshot does,
without PGR2_LIGHT_CLEAR. The wheel shade logic was right; PGR2_LIGHT_CLEAR is no longer needed (kept as an
experiment switch).
Harness: RAW=1, RT_THREAD=1, GEN_OPT=-O2 build like the Windows exe (none of them reproduced it on their own).

### 2026-10-18 - car body on Windows: the body draws write no pixels

Capture 213135 (XBE_CAPTURE_DRAWS=vs:B01019E1, PGR2_LIGHT_CLEAR on): the 16 body draws (962-977, up to 5722
vertices) change no pixel of the target; the tyre draws right after (same vertex and pixel shaders) do. The body
texture's alpha (tex_81629000) looks right, the light constants are right. So on D3D12 the body is dropped by alpha
kill, culling or the depth test (headless it is drawn). XBE_DRAW_TEST=B01019E1:alphakill|cull|depth switches each
off for that shader to find which. The walls were white again in this capture.

### 2026-10-18 - car body: a second, Windows-only cause

With PGR2_LIGHT_CLEAR on Windows (capture 211350): the body's constants are right (c56 1 1 1 1, c54 0.698, paint
c51/c52 set, pc0 0.188) yet the body still does not appear; headless the same switch shows the body in its paint.
Back buffer snapshots: before the car draws (1572) the road, after (1902) wheels, driver and the white shadow quad
under the car, no body. The body draws are 1610-1617 (vs B01019E1, ps B0101D10, t1 the 1024 D16 shadow map,
t3 the environment cube). New: XBE_CAPTURE_DRAWS=a-b saves the colour target after each draw in that range, to see
whether the body draws land and what covers them.

### 2026-10-18 - black car: what the shade flag does, and an experiment switch

Harry's reference (a YouTube video of a 07-11-03 pre-release build, Instant Action in Florence): on the grid the
player car sits in building shade with its paint clearly visible (dimmer, reflection heavy); moving in sun it is
bright. Ours in shade shows no paint at all.
- The wheel flag means shade: FUN_00023620 gives a wheel's light as the triangle's baked colour when set, the sun
  colour times it when clear (used for the tyre smoke colour, FUN_00023670).
- Brightness eases (0.9/0.1) to 0.25 per unflagged wheel; c56 = sun x brightness; the paint (c51/c52) reaches the
  body only through c56, so at brightness 0 the body shows just its environment reflection (looks transparent:
  the street behind it is reflected) plus a tiny pc0 (the road's baked colour times a body tint).
- Headless A/B at the same frame (Florence Timed Run, Tamora): normal = no body visible; PGR2_LIGHT_CLEAR=1 (every
  wheel's flag forced 0) = body in its teal paint. The draw lists are identical; only the constants differ.
- Checked and ruled out: the track data (inflated by the game's zlib, 66% of triangles near the track flagged), the
  query's height (right triangle), the flag test sense, threads (one thread queries), 0x49E110 (race running).
So in the console's shade the sun term cannot be 0 the way it is here; where the difference comes from is open
(which triangle wins when several overlap, x87 precision in the hit test 0xC19F0/0xC11D0, or data per time of day).
PGR2_LIGHT_CLEAR=1 is an experiment switch only (src/game/hooks.cpp), not a fix.
Also: capture 194603 shows white walls and no body; 192449 (an earlier build, same spot) did not: needs a plain
screenshot to tell whether the capture changes (depth target saved mid-frame) affect that frame.

### 2026-10-18 - black car: the shade flag is real track data

Harry: on the original Xbox the car is lit (a normal car from the selection). Checked headless with more probes
(PGR2_LIGHT, src/game/hooks.cpp):
- Of 9633 distinct track triangles the ground query looked at in a Florence race, 6316 have bit 7 of the flag
  byte; the flag bytes are a small set of values (0..52 and 128..180).
- The triangle records are not stored raw in Florence.pak_*: a watchpoint on one shows it written once at load by
  0x1DC533, zlib's inflate window copy (D3DX section) with the final value; every other file goes through it too.
- The query finds the right triangle: a wheel at y 1.93 gets ground 1.59 (a wheel radius below) and that triangle's
  flag and baked colour.
- 0x24DE0 stores bit 7 of the hit byte as the wheel flag; 0x207CC skips the 0.25 for a set flag.
So data, query and test agree: in Florence the player's wheels mostly sit on flagged triangles, brightness eases to
0, c56 (sun) is 0 and the body keeps only pc0, which for the body draw is tiny (0, 0.02, 0.027): no paint shows. In
Edinburgh (Time Attack) the car takes the other branch (+0x19C8 == 5, c56 = +0x1520, 0.8) and is lit.
Next: what the console shows in a shaded Florence street (paint visible, how dark), and what the flag bit means;
whether pc0 for the body (paint times an ambient level) should be larger.

### 2026-10-18 - black player car: the game's own shadow logic, the same headless

Windows run with XBE_VSC and capture 194603 (Florence grid): c56 is written only as 0 0 0 0 by 0x1F036 and c54 as
0 0 0 0.4 by 0x1F15C, so the body has ambient light only (pc0 0.188); the car's shadow map renders properly
(1441_8327A000_z). The cause is the game's own state, not the renderer:
- 0x1F036 writes the sun colour (0x49EEA8, FFFFFFFF) scaled by the car's brightness (+0x150C).
- Each frame (0x20690) the brightness eases (x0.9 + 0.1 x target) to 0.25 x the wheels whose flag is 0
  (TEST/JNZ at 0x207CC: flag 1 = in shadow).
- A wheel's flag is bit 7 of the flag byte of the track triangle under it (FUN_00024de0 -> 0xC74D0 -> 0xC6210,
  byte +0xB of the 16 byte triangle record), and its baked colour comes back too.
- `PGR2_LIGHT=1` (src/game/hooks.cpp, game only) logs each car's flags, colours, brightness and the sun colour
  when they change. Headless, the Instant Action car's wheels are flagged most of the time as well, its brightness
  near 0, the same as Windows; early headless frames showed race cars as "wheels and driver" from the start.
Both renderers draw what the game asks for. Open: whether the console shows the same (the flag may mean shaded by
buildings, in which case a dark car in a shaded street is right), or a bit 7 the runtime gets wrong. Next:
compare with the console or xemu at the same spot (Florence, Duomo grid; Chicago).

### 2026-10-18 - Windows capture of the black car

Captures work from Harry's PC (PowerShell's `set` had not set the variables). Capture 192449 (Florence grid,
black car): the six environment faces now go to six targets on Windows (SetRenderTargetFast fix confirmed).
The body's constants were not listed (only small draws listed them) and the shadow pass's depth was not saved:
both now captured. XBE_VSC used its 200 lines on frontend writes (0x118BFA, 0x118CA2) before the race; it now logs
each distinct register, value and caller once (headless race: c56 from 0x1F036, the sun colour times the car's
brightness, c54 from 0x1F15C).

### 2026-10-18 - Windows: reflections confirmed, release logs

Harry confirms reflections on car paint and glass now work on Windows (SetRenderTargetFast plus the D3D12 cube
copy). F3 "did nothing": his log shows the F3 stack dump at 7 s of a 19 s run (during the movies), but release
builds keep only error and report lines, so XBE_VSC, the F3 note and the capture lines were never written. Those
are reports now. The release log is logs\pgr2.log, overwritten each start (WINDOWS.md said pgr2_*.log).

### 2026-10-18 - car bodies, environment map, Windows captures

Harry's Windows test: boots, movies, audio and saves (KM/H and Time Attack across a restart) work; the licence plate
screen is right on Windows (garbled headless only). The player's car in Instant Action is a black body with wheels
(other cars fine), and some walls show green/magenta stripes. F3 captures copied to .local/win (6 frames).

Environment map. PGR2 draws its car reflection cube's six faces with D3DDevice_SetRenderTargetFast (0x1C3C00),
which ran the library's code: it writes the surfaces to the hardware directly, so the host never switched targets
and all six faces were drawn into whatever was bound before (the Windows capture shows face 0 drawn six times).
Now on the host (sdk f6f4edd), signed into 5659 (187 of 246). Headless all six faces fill (AUDIT shows +0 to +A0000
drawn) and the dumped cube shows the street. With the D3D12 cube copy from the earlier entry, reflections should
now work on Windows.

Black player car, what is known:
- The car body's vertex shader (B01019E1) gives diffuse = c56 x (N.L - c1.w^2) x paint (c51/c52) and specular from
  c54. In Harry's capture (160442, the Camaro) paint is white but c56 is 0 and c54 is (0, 0, 0, 0.4): no light,
  so black. Headless the same constants are c56 0.8, c54 0.557 (set by SetVertexShaderConstant1Fast from 0x1F080
  and 0x1F15C, FUN_0001ef90).
- FUN_0001ef90 scales the sun colour (0x49EEA8) by the car's brightness (+0x150C); both zeros on Windows mean the
  brightness or the sun colour is 0 there. The brightness is 0.25 x the wheels whose ground query (FUN_00024de0 ->
  FUN_000c74d0, track collision) says the surface is not flagged (bit 7 of its surface byte): all four flagged
  gives a black car. Pure CPU code; why it differs on Windows is open. Visibility query counts do not matter
  (headless with a forced count: same picture).
- Headless also had a black body for another reason: the reference renderer ignored polygon offset, so the car's
  self-shadow map (drawn with offset 128, slope 24) shadowed the car itself. Fixed (sdk b5993fc); the MINI shows
  its paint now (dark green, c51 = 0 0.04 0).
- `XBE_VSC=56 54` (new) logs every write to those constants with the caller: one Windows run with it answers
  whether 0x1F080/0x1F15C write zeros there.

Stripes (capture 085551, car reversing into a wall): every 2D texture the frame bound decodes cleanly (montage of
397); the brown in that frame is the clear colour of the 256x256 reflection target 834CA000, which most of the
scene samples at stage 3; the striped wall uses bump-mapped reflection shaders (dot product reflect, normal map
82501200). Needs a capture of the target after each draw near the wall to go further.

### 2026-10-18 - coverage, soak, system link

Pad scripts (each from a first boot, making the profile as save1.txt does; checked with snaps at each step):
- `timeattack.txt`: Circuit Challenge, Edinburgh, Princes Street East, MINI Cooper S, Auto; rolling start, 60 s
  of throttle.
- `kws.txt`: Kudos World Series, Compact Sports Series, Street Race (Florence, Duomo 2), MINI, Auto, Novice;
  grid start, 60 s of throttle.
- `arcade.txt`: Arcade Racing, Timed Run 1 (Stockholm, Gamla Stan Loop, Vauxhall VX220), Novice; 60 s throttle.
- `replay.txt`: the same Timed Run left to time out (2:45), then View or Save Replay: the replay plays with its
  controls (Save, rewind, pause, fast forward, cameras).
No traps in any; quitting a race (pause > Quit > Yes) goes back to its menus or the title.

Soak (`scripts/soak.sh 60`): Instant Action races back to back for 60 minutes, each 60 s of throttle, then
pause, Quit, Instant Action again (a random city and car each time: Barcelona, Edinburgh, Florence, ...).
31 races scripted, 30 3D scenes drawn, 107,460 frames, 200 million draws; no traps, no errors, no hang (frames to
the end, 31 empty frames, one per load). Memory: RSS 100 MB at 5 minutes, 104 MB at 60 (+4.4 MB an hour), data
600 -> 619 MB: no leak worth chasing. 10 minute run before it: the same.

System link (two instances, `sdk/harness/run.sh -n 2`):
1. Binding failed: PGR2 binds ports 1000-1002, Linux keeps ports below 1024 for root (WSAEACCES). Fixed (sdk
   35f0f68): loopback instances on a non-Windows host shift those ports up 20000 and back. Both bind now.
2. Host: Multiplayer > System Link > Create New Game > Edinburgh > Princes Street East > options: the lobby
   ("Player (Host)", seven open, waiting). Joiner: System Link > Search for Games. Traced (not committed): the
   search broadcast (9 bytes) reaches the host, the host's 204 byte answer reaches the joiner, the joiner sends a
   186 byte join to the host's port 1002, the host answers 1183 bytes, the joiner acks 12 bytes. Then neither sends
   anything; the host lists no one and the joiner says "You have lost connection with the host".
3. A joiner with a different profile name: the same.
Three attempts, stopped. Ideas for next time: which XNet calls run after the ack (XNetRegisterKey with the host's
key, XNetXnAddrToInAddr on the host's XNADDR from the 1183 bytes; the runtime keeps one key), whether a socket
waits in select() or an overlapped receive that never completes, and whether the identical console identity of
two instances (same EEPROM, serial, profile id 12E9194916CD) matters. On Windows with two PCs the port shift does
not apply; worth trying there.

### 2026-10-18 - research: video settings, timing and 60 fps (no code)

Video settings (EEPROM). PGR2 reads them: XGetVideoFlags (0x191297) calls ExQueryNonVolatileSetting(8,
XC_VIDEO_FLAGS) and returns (value >> 16) & 0x5F (1 widescreen, 2 720p, 4 1080i, 8 480p, 0x10 letterbox, 0x40
PAL60), dropping 720p/1080i unless the AV pack (0x192f34) is HDTV. Its callers: the device setup 0x1783a0 and the
PAL60 check 0x143050. The device setup always asks for a 640x480 back buffer; widescreen sets
D3DPRESENTFLAG_WIDESCREEN and the flag at +0x37C, 480p sets D3DPRESENTFLAG_PROGRESSIVE. 720p and 1080i are never
used: PGR2 has no HD mode.
- The runtime already reports widescreen when the output is 3:2 or wider, plus 480p (Windows: always widescreen
  on Harry's 3440x1440). Headless A/B with the new `OUTPUT=WxH` (same Time Attack frame,
  captures/1018-ab43-race.png and 1018-ab169-race.png): widescreen renders anamorphic 16:9 into 640x480, a wider
  horizontal field of view (more of the street) and the HUD narrowed and moved out to the edges; the D3D12 backend
  shows it at 16:9, so the shapes come out right. The 2D menus are the same in both.
- So nothing to enable: widescreen is on, 480p changes nothing but a flag, and more pixels come from the runtime's
  render scale (render_percent), not from the game.

Timing. FUN_000d90f0 (from the main loop 0x179830) runs each frame:
- dt = interval / refresh, from the presentation interval byte 0x4860F1 and the refresh rate byte 0x48614B (60).
- game time += dt x timescale (timescale 1.0, set by FUN_000d9060); in some modes measured time is used instead:
  swap timestamps from the swap callback 0xD9010 (or the performance counter), capped at 0.15 s.
- Physics steps a fixed 1/60 s at a time (FUN_000f5d60) until it catches up with game time: two steps a frame at
  30 fps, one at 60.
The interval comes from FUN_00177c70(n), which sets the D3D presentation interval: n = 1 0x80000001 (every vblank),
2 0x80000002 (every second vblank), and stores it in D3D's interval global 0x1D093C and the byte 0x4860F1.
Measured headless: 1 through the movies and menus, 2 from the race on. So the console runs the frontend at 60 and
races at 30, by presentation interval; the runtime ignores the interval and paces only by fps_cap, which is why
races ran double speed at 60 and why the 30 fps default slows the menus to 30.

What 60 fps would take:
1. First, generic: the runtime honours the presentation interval in Swap (pace to refresh / interval; an XDK
   layout key for D3D's interval global, 0x1D093C in 5659). Menus at 60, races at 30 as on the console, and the
   fps_cap 30 default could go. Test that the game speed is right in both.
2. Then races at 60: keep the interval at 1 in races (an override of FUN_00177c70 clamping 2 to 1, in src/game, as
   an option). dt becomes 1/60 and physics takes one 1/60 step a frame: the same simulation, so handling is
   unchanged. To check: anything per frame outside the physics loop (FUN_000179f0 compares an interval-derived
   step in replay/ghost timing; ghosts saved at 30 replayed at 60; Kudos and HUD timers; particles; system link
   between a 30 and a 60 machine), and that 60 frames a second of the race fit the frame budget on PC.

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
