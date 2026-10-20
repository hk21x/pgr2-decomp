# What carries over to Project Gotham Racing 1

This is an assessment, not tested: there is no PGR1 disc in this VM. The first real step is to run
`xbe_symbols` on PGR1's `default.xbe` (your own disc), which prints its library versions. Several of the answers
below depend on that.

## Summary

| Area | Carries over | Notes |
| --- | --- | --- |
| SDK runtime fixes (kernel, D3D, D3D12, sound, XNet) | yes, as they are | written for any title, in the `sdk/` fork |
| PC features (F1 settings, keyboard, remapping, aspect, pacing) | yes, as they are | in the SDK; a new game only fills `app::kGame` |
| Headless harness, pad scripts, CPU renderer, frame log, captures | yes | the pad scripts' presses must be rewritten for PGR1's menus |
| Repo layout, scripts (smoke, soak, savetest, linktest, wincheck, shadercheck) | yes, with new pad scripts | |
| XDK data (`sdk/xdk/5659`) | partly | PGR1 is a 2001 launch title, so almost certainly an older XDK; see below |
| PGR2 addresses, hooks, draw filter, race_60fps | no | the methods carry over; every address must be found again |
| What we learnt about Bizarre's engine | probably much of it | PGR2 is built on the PGR1 / Metropolis Street Racer line; to check |

## 1. The SDK, which carries over as it is

Everything fixed in `sdk/` was made generic: the comments do not name PGR2, and the fixes depend on XDK behaviour,
not on the game. A PGR1 port starts with all of it:

- **Kernel and XAPI.**
  - The cache partition format.
  - The empty `*.*` find name.
  - ReadFileEx APCs.
  - The DVD authentication page.
  - Raw partitions as volumes.
  - Save games (XCreateSaveGame).
- **D3D.**
  - Pushed commands kept in order.
  - Present-interval pacing.
  - Swap COPY / FINISH.
  - Visibility tests.
  - SetRenderTargetFast.
  - SetVertexShaderInput.
  - SetDepthClipPlanes and Reset on the host.
  - Immediate mode under an FVF.
  - W-buffering with its fixed-function scale.
  - A larger depth buffer serving a smaller target.
- **D3D12.**
  - Cube faces copied from the GPU.
  - 8-bit targets.
  - Declaration overhang.
  - SV_Depth W-buffering.
- **Sound.**
  - DirectSoundGetSampleTime on the host. Any frontend clock driven by it works.
  - PlayEx / Pause.
  - SetDopplerFactor.
- **XNet.**
  - XNetGetConnectStatus.
  - The loopback port shift.
  - XNET_TRACE.
- **PC features.**
  - F1 settings window: display mode, resolution, vsync, aspect, render scale, anisotropic filtering, frame cap,
    the game's options and controls.
  - Keyboard play with two keys per input.
  - Controller remapping, saved to the game's toml.
- **Diagnostics.**
  - Frame log (F4, `XBE_FRAMELOG`).
  - F3 captures and multi-frame captures.
  - `XBE_DRAW_SKIP`, `XBE_DRAW_TEST`, `XBE_VSC`.
  - The startup environment listing.
  - The draw filter hook (`gpu::g_draw_filter`) for game-side fixes.
- **Harness.**
  - Headless runs.
  - The vblank pad clock.
  - Snaps.
  - `REF_DRAWLOG`, `REF_ZOWNER`, `REF_VERTS_VS`.
  - AUDIT.
  - WAV capture.
  - Two-instance system link.

## 2. The XDK version: the main piece of new work

PGR1 shipped with the console in November 2001. It will have been built with an early XDK, before the 5659 PGR2
uses and the 5849 the SDK started from. What that means:

- **New `sdk/xdk/<build>/`.** Seed it from 5659 (closer in age than 5849), run the scan and fill the misses by hand
  in Ghidra, as Plan step 1 in CLAUDE.md describes. Expect fewer signature matches: older library code differs
  more. The method from PGR2 is to find each miss by the push buffer methods it writes and the device fields it
  touches. It worked for 43 of 60 D3D misses.
- **`layout.toml` from PGR1's own code.** The CDevice offsets moved even between 5659 and 5849, so expect more
  movement from an older build. Cite each offset to an instruction.
- **Render state numbering, the biggest risk.** The D3DRS indices of D3D__RenderState changed between XDK
  generations, because states were added in the middle. The runtime's numbering is 5659/5849's. If PGR1's build
  numbers them differently, the runtime needs a per-build table. Cxbx-Reloaded keeps such tables, as a reference
  for the behaviour only. Check this first by reading what PGR1's SetRenderState_* functions store and where
  (as was done for ZEnable 0x8F in 5659).
- **Texture stage state numbering** may differ in the same way.
- **Libraries PGR1 probably lacks.** XONLINE: Xbox Live launched in 2002, so it cannot be there. XNet or
  System Link may be absent too; check the XBE's library list and the game's menus. XMV: check what PGR1's movies
  are.
- **Push buffers.** Check again, as for PGR2 (Plan step 4): CreatePushBuffer / RunPushBuffer, and game code writing
  commands by hand. The runtime handles hand-pushed constants but does not replay recorded push buffers.

## 3. The game: methods carry over, addresses do not

None of PGR2's addresses apply. These are the approaches that found each PGR2 problem, and they should find the
equivalent in PGR1 quickly if the engine is related:

- **Speed.** Find the present interval setter and the frame step. In PGR2 these are 0x177C70 and 0xD90F0, with
  fixed 1/60 physics steps. The runtime already paces by the interval, so the speed should be right from the start.
  A `race_60fps`-style option is then a small override, if PGR1 also uses fixed physics steps.
- **Car shadow.** If PGR1 projects a shadow map onto a ground mesh with a polygon offset, the same kind of draw
  filter works: match by state (depth texture in stage 0, offset, no depth writes, multiply blend), not by address.
- **Lamp glows and visibility tests, combiner mode caches, time of day.** Look for the same patterns: a
  Begin/EndVisibilityTest probe, a stage 0 mode switch with a cache, and per-city lighting sets.
- **W-buffering.** If PGR1 asks for D3DZB_USEW, the runtime handles it already. If it uses a Z buffer, nothing
  changes.

## 4. Process and tools

- **New repository.** Copy `sdk/templates/game` to a new repository (pgr1-decomp) with the same fork of the SDK as
  a submodule. Keep the `config/game.toml` / `config/<game>.toml` split, CLAUDE.md's rules, PROGRESS.md and the
  branch-per-task workflow.
- **Scripts to copy.** `harness.sh`, `smoke.sh`, `soak.sh`, `savetest.sh`, `linktest.sh`, `wincheck.sh`,
  `shadercheck.sh`, `framelog.py`, `wavstats.py`. The pad scripts must be rewritten: the menu paths differ and the
  timings are PGR1's.
- **The Ghidra helper scripts** (Decomp, Disasm, FindOperand, Callers, PushBufferScan, DeviceRefs) are in `.local/`,
  which is not committed. Worth committing them to `scripts/ghidra/` so a second project can use them.
- **The upstream PR** (draft in `.local/upstream-pr.md`) would put the generic fixes into TGP482's SDK for every
  title.

## Suggested first session for PGR1

1. Add PGR1's files to the new repo's `assets/` and run `xbe_symbols`. Note the XDK build and the library list.
2. Seed `sdk/xdk/<build>/` from 5659. Scan and count the misses.
3. Check the render state numbering against 5659 (the risk above).
4. Do the push buffer check (CreatePushBuffer, RunPushBuffer, hand-written commands) and report it to Harry.
5. Fill the layout, sign, recompile, first headless boot. Then proceed as with PGR2: boot, menus, race.

A rough estimate: if PGR1's XDK is close to 5659, it could be racing headless within a few sessions, as PGR2 was
(first frame to race in about two days). If its render state numbering differs, add the time to build and check
the per-build tables first.
