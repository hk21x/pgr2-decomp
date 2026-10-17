#!/usr/bin/env bash
# Starts Claude Code Remote Control (server mode) in tmux session "claude", plus a captures web server.
# Run 'claude remote-control' once by hand in this folder first, to trust it and enable Remote Control.
set -uo pipefail
cd "$(dirname "$0")/.."
export PATH="$HOME/.local/bin:$PATH"
tmux has-session -t claude 2>/dev/null && { echo "already running: tmux attach -t claude"; exit 0; }
tmux new -d -s claude -n claude \
  "while true; do claude remote-control --spawn worktree; echo 'restarting in 30s'; sleep 30; done"
if command -v tailscale >/dev/null && ip=$(tailscale ip -4 2>/dev/null); then
  tmux new-window -t claude: -n captures "python3 -m http.server 8080 --bind $ip --directory captures"
  echo "captures: http://$ip:8080/"
fi
echo "started: tmux attach -t claude"
