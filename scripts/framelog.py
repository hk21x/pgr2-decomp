#!/usr/bin/env python3
"""Reads an XBE_FRAMELOG file: for each draw (shader, count, textures, indices) seen in most frames, how what it
reads changes from frame to frame: vertex bytes, texture contents, stage states, deferred render states, pixel
shader constants, and the vertex shader constants by block of 16 registers (c-96 up). Static scenery keeps one
value for its own inputs; camera matrices change for every draw. A value that comes back after a change (A B A B)
is the sign of a flicker.
    scripts/framelog.py logs/framelog.txt [screen targets, comma separated: only those, as one] [-v block]"""
import sys, collections
args = [a for a in sys.argv[1:]]
screen = set(args[1].split(',')) if len(args) > 1 and not args[1].startswith('-') else set()
rows = collections.defaultdict(lambda: collections.defaultdict(list))
frames = set()
for l in open(args[0]):
    f = l.split()
    if len(f) < 26: continue
    fr = int(f[0]); frames.add(fr)
    g = lambda k, n=1: f[f.index(k) + n]
    rt = g('rt')
    if screen:
        if rt not in screen: continue
        rt = 'screen'
    key = (rt, g('vs'), g('n'), g('tex'), g('tex', 2), g('tex', 3), g('tex', 4), g('ib'))
    inputs = {'vertices': g('vb'), 'tex0': g('th'), 'tex1': g('th', 2), 'tex2': g('th', 3), 'tex3': g('th', 4)}
    if 'tss' in f:
        inputs['stage states'] = g('tss'); inputs['render states'] = g('rs')
        i = f.index('psc'); inputs['pixel constants'] = " ".join(f[i + 1:i + 17])
        j = f.index('vsc')
        for b in range(12): inputs[f'vsc c{b * 16 - 96}..c{b * 16 - 81}'] = f[j + 1 + b]
    else:
        inputs['states'] = g('st')
    rows[key][fr].append(inputs)
frames = sorted(frames)
def pattern(seq):
    names = {}
    return [names.setdefault(x, len(names)) for x in seq]
def show(p):
    return "".join("ABCDEFGHIJKLMNOPQRSTUVWXYZ"[x] if x < 26 else "+" for x in p)
report = collections.Counter(); examples = collections.defaultdict(list)
for key, per in rows.items():
    if len(per) < len(frames) * 0.8: continue
    if any(len(v) != 1 for v in per.values()): continue  # drawn more than once a frame: ambiguous
    seq = [per[fr][0] for fr in frames if fr in per]
    for what in seq[0]:
        p = pattern([s[what] for s in seq])
        if len(set(p)) == 1: continue
        back = any(p[j] in p[:j] and p[j] != p[j - 1] for j in range(1, len(p)))
        kind = 'every frame' if len(set(p)) >= len(p) - 1 else 'returns to earlier values' if back else 'changes'
        label = f"{what}: {kind}"
        report[label] += 1
        examples[label].append((key, p, [s[what] for s in seq]))
print(f"{len(frames)} frames, {len(rows)} draw kinds")
for label, n in sorted(report.items(), key=lambda x: -x[1]):
    print(f"{n:6d}  {label}")
    for key, p, vals in examples[label][:3]:
        print(f"          {key[1]} n {key[2]} tex {key[3]} rt {key[0]}: {show(p)}")
