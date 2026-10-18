#!/usr/bin/env bash
# Regenerate generated/game from assets/default.xbe and config/game.toml
set -euo pipefail
cd "$(dirname "$0")/.."
cmake --build build/linux --target recompile_PGR2
