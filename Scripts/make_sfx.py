# Synthesises the game's basic sound effects (numpy -> 16-bit mono WAV, 44.1 kHz) into SourceArt/Sfx.
#   python make_sfx.py <out_dir>
import sys, os, wave
import numpy as np
OUT = sys.argv[1]; os.makedirs(OUT, exist_ok=True)
SR = 44100
rng = np.random.default_rng(7)
def t(d): return np.arange(int(SR * d)) / SR
def env(n, a=0.005, r=0.3):
    e = np.ones(n); na = max(1, int(SR * a)); e[:na] = np.linspace(0, 1, na)
    return e * np.exp(-np.arange(n) / (SR * r))
def tone(f, d, r=0.3, a=0.004): x = t(d); return np.sin(2 * np.pi * f * x) * env(len(x), a, r)
def sweep(f0, f1, d, r=0.2):
    x = t(d); f = f0 + (f1 - f0) * (x / d); return np.sin(2 * np.pi * np.cumsum(f) / SR) * env(len(x), 0.004, r)
def noise(d, lo=0.0, hi=1.0):
    n = int(SR * d); s = np.fft.rfft(rng.standard_normal(n)); fr = np.fft.rfftfreq(n, 1 / SR)
    s[(fr < lo) | (fr > hi)] = 0; y = np.fft.irfft(s, n); return y / (np.abs(y).max() + 1e-9)
def mix(*parts):
    n = max(len(p) for p in parts); y = np.zeros(n)
    for p in parts: y[:len(p)] += p
    return y
def at(x, delay): return np.concatenate([np.zeros(int(SR * delay)), x])
def save(name, y, vol=0.8):
    y = y / (np.abs(y).max() + 1e-9) * vol
    n = len(y); f = min(n, int(SR * 0.01)); y[-f:] *= np.linspace(1, 0, f)
    with wave.open(os.path.join(OUT, "S_%s.wav" % name), "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR); w.writeframes((y * 32767).astype(np.int16).tobytes())
    print("SFX", name, round(n / SR, 2), "s")

save("Bell", mix(tone(1568, 1.3, 0.45), 0.5 * tone(3136, 1.0, 0.3), 0.3 * tone(4710, 0.7, 0.2), 0.2 * tone(784, 1.0, 0.4)))
save("Chop", mix(noise(0.09, 800, 6000) * env(int(SR * 0.09), 0.001, 0.02), 0.9 * sweep(180, 90, 0.12, 0.04)), 0.7)
n = int(SR * 1.4); sz = noise(1.4, 2500, 11000) * (0.55 + 0.45 * np.abs(noise(1.4, 0, 40))); sz[:int(SR * 0.05)] *= np.linspace(0, 1, int(SR * 0.05)); sz *= np.exp(-np.arange(n) / (SR * 0.7))
save("Sizzle", sz, 0.5)
save("Coin", mix(tone(1319, 0.3, 0.12), at(tone(1976, 0.5, 0.18), 0.07), 0.3 * at(tone(3951, 0.4, 0.1), 0.07)), 0.7)
save("Pop", sweep(420, 900, 0.11, 0.05), 0.6)
save("Plop", mix(sweep(650, 180, 0.2, 0.07), 0.25 * noise(0.15, 300, 2500) * env(int(SR * 0.15), 0.002, 0.04)), 0.7)
save("Click", mix(tone(2200, 0.05, 0.012), 0.5 * noise(0.03, 2000, 9000) * env(int(SR * 0.03), 0.001, 0.008)), 0.45)
save("Chime", mix(tone(1047, 0.7, 0.25), at(tone(1319, 0.7, 0.25), 0.09), at(tone(1568, 0.9, 0.3), 0.18)), 0.6)
save("Sparkle", mix(*[at(tone(f, 0.6, 0.18) * (0.9 - 0.1 * i), 0.07 * i) for i, f in enumerate((1568, 1976, 2349, 3136, 3951))], 0.3 * at(tone(6272, 0.5, 0.15), 0.3)), 0.6)
wn = noise(0.45, 300, 5000); x = t(0.45); save("Whoosh", wn * np.sin(np.pi * x / 0.45) ** 2 * (0.4 + 0.6 * x / 0.45), 0.5)
save("Thunk", mix(sweep(140, 60, 0.18, 0.06), 0.5 * noise(0.04, 1500, 8000) * env(int(SR * 0.04), 0.001, 0.01)), 0.8)
save("Bubble", mix(sweep(300, 950, 0.16, 0.08), at(sweep(420, 1200, 0.14, 0.07), 0.13), 0.6 * at(sweep(520, 1400, 0.12, 0.06), 0.24)), 0.6)
x = t(0.22); save("Error", np.sign(np.sin(2 * np.pi * 165 * x)) * env(len(x), 0.004, 0.09) * 0.6 + 0.4 * tone(110, 0.22, 0.09), 0.4)
save("Flip", mix(noise(0.16, 500, 4000) * np.sin(np.pi * t(0.16) / 0.16) ** 2, at(0.6 * noise(0.2, 3000, 10000) * env(int(SR * 0.2), 0.002, 0.07), 0.14)), 0.55)
