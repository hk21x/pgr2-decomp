#!/usr/bin/env python3
"""Reads an XBE_FRAMELOG file: for each draw (shader, count, textures, indices) seen in many frames, how its
vertex bytes, texture contents and stage states change from frame to frame. Static scenery keeps one value;
a value that alternates (A B A B) or comes back after a change is the sign of a flicker.
    scripts/framelog.py logs/framelog.txt [screen targets, comma separated: only those, as one]"""
import sys, collections, re
rows = collections.defaultdict(lambda: collections.defaultdict(list))
# render targets drawn as the screen (back buffers alternate), comma separated
screen = set(sys.argv[2].split(',')) if len(sys.argv) > 2 else set()
rts = collections.Counter()
frames = set()
for l in open(sys.argv[1]):
    f = l.split()
    if len(f) < 26: continue
    fr = int(f[0]); frames.add(fr)
    g = lambda k, n=1: f[f.index(k) + n]
    rt = g('rt'); rts[rt] += 1
    if screen and rt in screen: rt = 'screen'
    key = (rt, g('vs'), g('n'), g('tex'), g('tex', 2), g('tex', 3), g('tex', 4), g('ib'))
    rows[key][fr].append((g('vb'), g('th'), g('th', 2), g('th', 3), g('th', 4), g('st')))
main = 'screen' if screen else None
frames = sorted(frames)
def pattern(seq):
    names = {}
    return [names.setdefault(x, len(names)) for x in seq]
def show(p):
    return "".join("ABCDEFGHIJKLMNOPQRSTUVWXYZ"[x] if x < 26 else "+" for x in p)
report = collections.Counter(); examples = {}
for key, per in rows.items():
    if main and key[0] != main: continue
    if len(per) < len(frames) * 0.8: continue
    if any(len(v) != 1 for v in per.values()): continue  # drawn more than once a frame: ambiguous
    seq = [per[fr][0] for fr in frames if fr in per]
    for i, what in enumerate(("vertices", "tex0", "tex1", "tex2", "tex3", "states")):
        p = pattern([s[i] for s in seq])
        kinds = len(set(p))
        if kinds == 1: continue
        # changes every frame (dynamic) versus coming back to an earlier value (a flicker)
        back = any(p[j] in p[:j] and p[j] != p[j - 1] for j in range(1, len(p)))
        label = f"{what}: {'returns to earlier values' if back else 'changes'}"
        report[label] += 1
        examples.setdefault(label, []).append((key, p))
print(f"{len(frames)} frames, {len(rows)} draw kinds")
for label, n in report.most_common():
    print(f"{n:6d}  {label}")
    for key, p in examples[label][:4]:
        print(f"          {key[1]} n {key[2]} tex {key[3]} {key[6]} rt {key[0]}: {show(p)}")
