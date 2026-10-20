#!/usr/bin/env bash
# Runs one of scripts/ghidra/*.java on the analysed project (read-only) and prints its output lines.
#   scripts/ghidra.sh Decomp 13db90 13cca0        decompile functions, with their callers
#   scripts/ghidra.sh Disasm 14b380 40             instructions from an address
#   scripts/ghidra.sh FindOperand 0x27ecc8 "[0x291a24]"   instructions whose text has each argument
#   scripts/ghidra.sh Callers 1c42f0               callers of a function
# Needs the project made once (CLAUDE.md, Ghidra): .local/game.exe imported into ~/ghidra-projects/pgr2.
set -euo pipefail
cd "$(dirname "$0")/.."
script=$1; shift
timeout 900 /opt/ghidra/support/analyzeHeadless "$HOME/ghidra-projects" pgr2 -process game.exe -noanalysis -readOnly \
  -scriptPath scripts/ghidra -postScript "$script.java" "$@" 2>&1 |
  grep -v -E '^(INFO|WARN|ERROR) [^>]*$|\(HeadlessAnalyzer\) *$|^(openjdk|OpenJDK)|^    /(opt|home)/.*ghidra_scripts *$|^    /.*scripts/ghidra *$' |
  sed 's/ (GhidraScript) *$//; s/^INFO  [A-Za-z]*\.java> //'
