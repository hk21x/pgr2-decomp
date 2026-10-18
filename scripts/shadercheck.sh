#!/usr/bin/env bash
# Compile the shaders a harness run dumped (AUDIT=N writes .local/run/1/cap/hlsl) with DXC, as the D3D12 backend
# would: HLSL 2018 rules (the backend compiles with D3DCompile), vs/ps 6.0.   scripts/shadercheck.sh [hlsl folder]
# DXC for Linux is fetched into .local/dxc on first use.
set -uo pipefail
cd "$(dirname "$0")/.."
DIR=${1:-.local/run/1/cap/hlsl}
D=.local/dxc
if [ ! -x $D/bin/dxc ]; then
  mkdir -p $D && curl -sL -o $D/dxc.tgz \
    https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.9.2609/linux_dxc_2026_09_28.x86_x64.tar.gz &&
    tar xzf $D/dxc.tgz -C $D || { echo "cannot fetch dxc"; exit 1; }
fi
ls "$DIR"/*.hlsl >/dev/null 2>&1 || { echo "no shaders in $DIR: run scripts/harness.sh with AUDIT=N"; exit 1; }
ok=0 bad=0
for f in "$DIR"/*.hlsl; do
  if out=$(LD_LIBRARY_PATH=$D/lib $D/bin/dxc -HV 2018 -T "$(basename "$f" | cut -c1-2)_6_0" -E main "$f" -Fo /dev/null 2>&1); then
    ok=$((ok + 1))
  else
    bad=$((bad + 1)); echo "FAIL $f"; echo "$out" | grep error | head -3
  fi
done
echo "shaders: $ok compiled, $bad failed"
[ $bad -eq 0 ]
