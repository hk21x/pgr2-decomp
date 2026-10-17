# PGR2 recomp - progress log

Newest first. Each entry: date, what changed, what was learned, what's next.

## Status
- Milestone: 1 - XDK libraries identified, `sdk/xdk/5659/` seeded from 5849, config written; nothing fixed yet
- XDK build of default.xbe: **5659** (all libraries)
- xbe_symbols: 135 of 238 functions, 8 of 12 data found; 103 functions + 4 data missing (trapping stubs if hit)
- `sdk/xdk/5659/layout.toml`: still the 5849 values, unverified (one known wrong, see log)
- Push buffers: no `RunPushBuffer`/`CreatePushBuffer` found; 4 game functions write vertex shader constants
  straight into the D3D push buffer, which the runtime's `Walk` currently ignores (see log)
- Trapping stubs remaining: unknown until the first harness run

## Log

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
- D3D (63): CDevice_KickOff D3D_BlockOnResource D3D_BlockOnTime D3D_KickOffAndWaitForIdle D3D_MakeRequestedSpace D3D_SetFence
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
