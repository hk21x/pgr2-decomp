# Project Gotham Racing 2 (original Xbox) - static recompilation

Goal: a native Windows port of PGR2 built with the XboxXBE-Recomp toolkit in `sdk/` (a git submodule).
Read `sdk/README.md`, `sdk/harness/README.md` and `PROGRESS.md` at the start of every session.

## Environment
- Headless Ubuntu VM. No GPU, no window. Harry builds and play-tests the real Windows exe separately.
- Here you build the Linux tools, regenerate code, and run the game in the headless harness.
- Game files (Harry's own disc) are in `assets/`. Never commit, upload or paste them, or generated code.
  `assets/`, `generated/`, `build/`, `.local/` and `captures/` are git-ignored; keep it that way.
- Signatures in `sdk/xdk/*/signatures.toml` are hashes and lengths only. Never store library bytes.

## Commands
- `scripts/linux-tools.sh`   configure for Linux + build `xbe_symbols` and `xbe_recomp`
- `build/linux/xbe_symbols assets/default.xbe sdk/xdk --out config/game.toml`
     prints the XBE's XDK library versions, matches XDK functions, writes the config.
     Stops if `sdk/xdk/<build>/` has no signatures.toml.
- `scripts/recompile.sh`      regenerate `generated/game`
- `scripts/harness.sh [secs] [pad] [ENV=..]`   build + run headless; e.g. `scripts/harness.sh 120 scripts/pad/boot.txt REF=60`
     `REF=N` draws every Nth frame on the CPU into PNGs in `captures/` (view them with Read),
     `CAP=N` writes each Nth frame's draw list, `snap` file in the run folder grabs the next two frames.
- Ghidra: `python3 sdk/tools/symbols/xbe2pe.py config/game.toml .local/game.exe`, then
  `/opt/ghidra/support/analyzeHeadless ~/ghidra-projects pgr2 -import .local/game.exe` (add `-process`/scripts as needed).

## Plan
1. Extract the XBE's library versions. If the build is not 5849, create `sdk/xdk/<build>/`:
   seed it with copies of `sdk/xdk/5849/*.toml`, run xbe_symbols, then fix misses by hand in Ghidra.
   Re-derive every `layout.toml` offset from PGR2's own code, citing the instruction address as 5849 does.
2. Get to boot: resolve trapping stubs hit during boot, one at a time.
3. Main menu, then a race, then a full race without crashes.
4. Check early whether PGR2 uses D3DDevice_RunPushBuffer / CreatePushBuffer or writes push buffers directly.
   The runtime does not replay recorded push buffers; flag this to Harry as soon as you know.

## Working rules
- One problem at a time. After each meaningful step, add a dated entry at the top of PROGRESS.md's log
  (what changed, evidence, next step), update its Status block, and commit.
- Game-specific fixes go in `src/game` (X86_FUNC overrides, config [[hook]]/[[function]]/[[switch]]), not in
  generated code. SDK changes (new XDK data, runtime fixes) go in the `sdk/` submodule as separate commits.
- If the same problem survives 3 genuinely different attempts, stop: write up what you tried and what you
  think is happening in PROGRESS.md, commit, and tell Harry.
- Work on one branch per task. When the task ends and everything builds, fast-forward `main` to it
  (`git merge --ff-only`) and push both. SDK work goes on a branch of the fork (hk21x/XboxXBE-Recomp).
- Never force-push. Never rewrite history.
