"""Generate the original one-shot used by the Cinder Blade hit confirmation."""
from math import exp, pi, sin, tanh
from pathlib import Path
from random import Random
import struct
import wave

SAMPLE_RATE = 48_000
DURATION = 0.205
# Inharmonic, short-decaying partials suggest metal without a long bell tail.
METAL_MODES = ((678, 0.16, 31), (1467, 0.24, 47), (2720, 0.11, 56), (4380, 0.07, 83))


def render():
    noise = Random(752)
    low_noise = 0.0
    samples = []
    for index in range(round(SAMPLE_RATE * DURATION)):
        time = index / SAMPLE_RATE
        raw_noise = noise.uniform(-1.0, 1.0)
        low_noise += 0.17 * (raw_noise - low_noise)
        click = (raw_noise - low_noise) * 0.23 * exp(-115 * time)
        metal = sum(gain * exp(-decay * time) * sin(2 * pi * hz * time)
                    for hz, gain, decay in METAL_MODES)
        # A short downward sweep adds a body impact under the metal transient.
        body_phase = 2 * pi * (104 * time + 71 * (1 - exp(-67 * time)) / 67)
        body = 0.34 * sin(body_phase) * exp(-37 * time) + low_noise * 0.11 * exp(-41 * time)
        attack = min(1.0, time / 0.0014)
        tail = min(1.0, (DURATION - time) / 0.017)
        samples.append(tanh((click + metal + body) * 1.6) * attack * tail)
    return samples


def main():
    output = (Path(__file__).resolve().parent.parent / "SourceAssets" / "Audio" /
              "Combat" / "SW_ET_CinderBladeHit.wav")
    samples = render()
    peak = max(abs(sample) for sample in samples)
    pcm = struct.pack(f"<{len(samples)}h", *(round(sample * 0.61 * 32767 / peak)
                                             for sample in samples))
    output.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(output), "wb") as sound:
        sound.setparams((1, 2, SAMPLE_RATE, 0, "NONE", "not compressed"))
        sound.writeframes(pcm)
    print(output)


if __name__ == "__main__":
    main()
