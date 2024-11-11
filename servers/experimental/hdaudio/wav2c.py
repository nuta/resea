#!/usr/bin/env python3
from scipy.io import wavfile
import argparse
import matplotlib.pyplot as plt
import numpy as np
import sys
import subprocess

def generate_c_file(left, right):
    text = ""
    text += "#ifndef __SOUND_DATA_H__\n"
    text += "#define __SOUND_DATA_H__\n"
    text += "// Generated by wav2c.py\n"
    text += "signed short sound_data[] = {\n"
    text += ", ".join(
        map(lambda t: str(int(t[0])) + "," + str(int(t[1])),
            zip(left, right)))
    text += "};\n"
    text += "#endif\n"
    return text

def check_bits_per_sample(wav_file):
    """
    Checks if the wav file is a signed 16-bit (little endian) PCM data.
    """
    p = subprocess.Popen(
        ["ffmpeg", "-i", wav_file],
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        text=True
    )
    for line in p.stdout.readlines():
        if line.strip().startswith("Stream #"):
            if "Audio: pcm_s16le" in line:
                return
    sys.exit(f"{wav_file} is not pcm_s16le; use ffmpeg to convert the format")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("wav_file")
    parser.add_argument("--plot", action="store_true")
    args = parser.parse_args()

    samplerate, data = wavfile.read(args.wav_file)
    num_channels = data.shape[1]

    # We need this check since our HD Audio driver hard-codes the format of
    # PCM data.
    check_bits_per_sample(args.wav_file)
    if num_channels != 2:
        sys.exit(f" # of channels must be 2; use ffmpeg to convert the format")
    if samplerate != 44100:
        sys.exit(f"sample rate must be 44.1KHz; use ffmpeg to convert the format")

    left = data[:, 0].tolist()
    right = data[:, 1].tolist()
    length = data.shape[0] / samplerate
    print(f"{args.wav_file}: {num_channels} channels, {samplerate} Hz, {round(length, 4)}s")

    with open("sound_data.h", "w") as f:
        f.write(generate_c_file(left, right))

    if args.plot:
        time = np.linspace(0., length, data.shape[0])
        plt.plot(time, left, label="Left channel")
        plt.plot(time, right, label="Right channel")
        plt.legend()
        plt.xlabel("Time [s]")
        plt.ylabel("Amplitude")
        plt.show()

if __name__ == "__main__":
    main()