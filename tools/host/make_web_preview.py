#!/usr/bin/env python3
"""Render every clip and pack them into one PNG sprite sheet for the web
preview. Prints the clip index as JSON.

  python3 tools/host/make_web_preview.py preview/web
"""
import json
import os
import struct
import subprocess
import sys
import zlib

W, H = 128, 64
BIN = "preview/preview"

EMOTIONS = ["neutral", "happy", "excited", "sad", "angry", "surprised",
            "sleepy", "love", "curious", "suspicious", "dizzy", "bored"]


def run(args, out):
    subprocess.run([BIN] + args + [out], check=True, capture_output=True)
    return open(out, "rb").read()


def png(path, w, h, gray_rows):
    raw = b"".join(b"\x00" + row for row in gray_rows)

    def chunk(tag, data):
        c = tag + data
        return struct.pack(">I", len(data)) + c + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF)

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 0, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(raw, 9)))
        f.write(chunk(b"IEND", b""))


def main():
    outdir = sys.argv[1] if len(sys.argv) > 1 else "preview/web"
    os.makedirs(outdir, exist_ok=True)
    tmp = os.path.join(outdir, "_tmp.bin")

    clips, frames = [], []

    def add(name, label, data):
        n = len(data) // (W * H)
        clips.append({"id": name, "label": label, "start": len(frames), "count": n})
        for i in range(n):
            frames.append(data[i * W * H:(i + 1) * W * H])

    add("auto", "Left alone", run(["anim", "26"], tmp))
    add("poke", "Poked", run(["poke", "110"], tmp))
    add("pet", "Petted", run(["pet", "170"], tmp))
    add("doze", "Falling asleep", run(["doze", "150"], tmp))
    for e in EMOTIONS:
        add(e, e.capitalize(), run(["arc", e], tmp))
    for w, label in [("clear", "Sunny"), ("clear-night", "Clear night"), ("cloudy", "Overcast"),
                     ("rain", "Rain"), ("snow", "Snow"), ("storm", "Thunderstorm")]:
        add("wx-" + w, label, run(["weather", w, "150"], tmp))

    os.remove(tmp)

    cols = 32
    rows = (len(frames) + cols - 1) // cols
    sw, sh = cols * W, rows * H
    sheet = bytearray(sw * sh)
    for i, fr in enumerate(frames):
        ox, oy = (i % cols) * W, (i // cols) * H
        for y in range(H):
            row = (oy + y) * sw + ox
            line = fr[y * W:(y + 1) * W]
            for x in range(W):
                if line[x]:
                    sheet[row + x] = 255

    png(os.path.join(outdir, "sheet.png"), sw, sh,
        [bytes(sheet[y * sw:(y + 1) * sw]) for y in range(sh)])

    index = {"frameW": W, "frameH": H, "cols": cols,
             "total": len(frames), "clips": clips}
    with open(os.path.join(outdir, "clips.json"), "w") as f:
        json.dump(index, f, indent=1)
    print(json.dumps(index))


if __name__ == "__main__":
    main()
