# PGR2 recomp - progress log

Newest first. Each entry: date, what changed, what was learned, what's next.

## Status
- Milestone: 3 - **title screen** in the harness, first boot (cache format and copy) and normal boot both
- XDK 5659: 183 of 242 functions, 12 of 12 data (D3D complete; 25 DSOUND, 7 CRT, 6 Winsock still 0)
- SDK fork: hk21x/XboxXBERecomp branch `pgr2-xdk5659`, pushed; `main` here points at it
- Config: hand entries in `config/pgr2.toml`, `config/game.toml` is xbe_symbols output only
- Known: pressing START during Intro2 (a movie with audio) hangs the movie player; car bodies missing in the
  attract demo; the CPU reference renderer shows dotted textures and white bands (not checked on D3D12)
- Next: the menu after the title, then a race; see the newest log entry

## Log

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
