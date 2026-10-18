#!/usr/bin/env python3
"""Per-second levels of a WAV the harness wrote (WAV=file), plain Python.

    scripts/wavstats.py <file.wav> [log]  [--from S] [--to S] [--every S]

Each line: second of the file (and of the log, when the log's "audio: output started" line gives the offset),
RMS and peak in dBFS, clipped samples (at full scale), and the strongest frequency in 40..--top Hz of an
--fft sample window (both channels summed). Ends with totals: length, sample rate, silent seconds, clipped samples.
"""
import argparse
import cmath
import math
import re
import struct
import sys
import wave


def fft(x):
    n = len(x)
    if n == 1:
        return x
    even, odd = fft(x[0::2]), fft(x[1::2])
    t = [cmath.exp(-2j * math.pi * k / n) * odd[k] for k in range(n // 2)]
    return [even[k] + t[k] for k in range(n // 2)] + [even[k] - t[k] for k in range(n // 2)]


def peak_freq(mono, rate, n=4096, top=4000):
    if len(mono) < n:
        return 0.0
    w = [mono[i] * (0.5 - 0.5 * math.cos(2 * math.pi * i / (n - 1))) for i in range(n)]
    spec = fft(w)
    lo, hi = int(40 * n / rate), int(top * n / rate)
    k = max(range(lo, hi), key=lambda i: abs(spec[i]))
    return k * rate / n


def db(v):
    return 20 * math.log10(v) if v > 0 else -120.0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('wav')
    ap.add_argument('log', nargs='?')
    ap.add_argument('--from', dest='start', type=float, default=0)
    ap.add_argument('--to', dest='end', type=float, default=1e9)
    ap.add_argument('--every', type=float, default=1)
    ap.add_argument('--fft', type=int, default=4096, help='window for the frequency, a power of 2')
    ap.add_argument('--top', type=float, default=4000, help='highest frequency looked at')
    o = ap.parse_args()
    args = [o.wav] + ([o.log] if o.log else [])
    start, end, every = o.start, o.end, o.every
    w = wave.open(args[0], 'rb')
    rate, ch, width = w.getframerate(), w.getnchannels(), w.getsampwidth()
    frames = w.getnframes()
    offset = None
    if len(args) > 1:
        for line in open(args[1], errors='replace'):
            m = re.match(r'\[\s*([\d.]+)\].*audio: output started', line)
            if m:
                offset = float(m.group(1))
    print(f'{args[0]}: {rate} Hz, {ch} channels, {width * 8} bit, {frames / rate:.1f} s'
          + (f', first sample at log time {offset:.1f} s' if offset is not None else ''))
    print('  file_s   log_s   rms_db  peak_db  clipped  freq_hz')
    silent = clipped_total = 0
    t = start
    while t < min(end, frames / rate):
        w.setpos(int(t * rate))
        n = min(rate, frames - int(t * rate))
        data = struct.unpack(f'<{n * ch}h', w.readframes(n))
        sq = sum(s * s for s in data)
        rms = math.sqrt(sq / len(data)) / 32768 if data else 0
        peak = max(abs(s) for s in data) / 32768 if data else 0
        clipped = sum(1 for s in data if s >= 32767 or s <= -32767)
        clipped_total += clipped
        if rms < 1e-4:
            silent += 1
        mono = [(data[i * ch] + data[i * ch + ch - 1]) / 65536 for i in range(min(n, o.fft))]
        f = peak_freq(mono, rate, o.fft, o.top) if rms >= 1e-4 else 0
        log_t = f'{t + offset:7.1f}' if offset is not None else '      -'
        print(f'{t:8.1f} {log_t} {db(rms):8.1f} {db(peak):8.1f} {clipped:8d} {f:8.0f}')
        t += every
    print(f'silent seconds {silent}, clipped samples {clipped_total}')


if __name__ == '__main__':
    main()
