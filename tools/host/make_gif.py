#!/usr/bin/env python3
"""Stitch the host renderer's raw frames into the animated GIF used in README.md.

Runs preview/preview for each clip (autonomous loop, poke, pet, and a few
expression arcs), keeps every other frame to land at 15 fps, scales the 128x64
panel up with nearest-neighbour so the pixels stay crisp, and drops it inside a
thin dark-grey frame.

    make gif                                   # builds preview/preview first
    python3 tools/host/make_gif.py [out.gif]

Needs Pillow: pip3 install pillow
"""
import os
import shutil
import subprocess
import sys
import tempfile

from PIL import Image

W, H = 128, 64
BIN = "preview/preview"

SCALE = 4          # nearest-neighbour zoom of the 128x64 panel
PAD = 8            # black margin inside the frame, in output pixels
BORDER = 3         # dark-grey frame, in output pixels
STRIDE = 2         # keep every Nth source frame (renderer runs at 30 fps)

# GIF delays live in hundredths of a second, so 15 fps (66.7 ms) has no exact
# spelling. Alternating 70/60 ms averages out close enough to keep the timing
# honest instead of drifting 10% fast or slow.
DELAYS = [70, 60]

BLACK = (0, 0, 0)
WHITE = (255, 255, 255)
GREY = (58, 58, 58)

# (renderer arguments, first frame to keep, number of frames to keep)
CLIPS = [
    (["anim", "8"], 30, 120),      # left alone: gazes around, blinks, changes mood
    (["poke", "110"], 0, 84),      # poked at frame 12: startle, then cheer up
    (["pet", "170"], 0, 160),      # petted: squints up, hearts, heart-eyes, let go
    (["arc", "angry"], 0, 45),
    (["arc", "sad"], 0, 45),
    (["arc", "dizzy"], 0, 45),
    (["arc", "love"], 0, 45),
]


def render(args, tmp):
    """Run the host renderer and return its frames as a list of 128*64 buffers."""
    out = os.path.join(tmp, "clip.bin")
    subprocess.run([BIN] + args + [out], check=True, capture_output=True)
    data = open(out, "rb").read()
    return [data[i * W * H:(i + 1) * W * H] for i in range(len(data) // (W * H))]


def panel(buf):
    """One raw frame -> a bordered, scaled, palettised GIF frame."""
    eye = Image.frombytes("P", (W, H), bytes(buf))
    eye = eye.resize((W * SCALE, H * SCALE), Image.NEAREST)

    edge = PAD + BORDER
    frame = Image.new("P", (W * SCALE + 2 * edge, H * SCALE + 2 * edge), 2)
    frame.paste(Image.new("P", (W * SCALE + 2 * PAD, H * SCALE + 2 * PAD), 0),
                (BORDER, BORDER))
    frame.paste(eye, (edge, edge))
    frame.putpalette(list(BLACK) + list(WHITE) + list(GREY))
    return frame


def main():
    out_path = sys.argv[1] if len(sys.argv) > 1 else "preview/deskbuddy.gif"
    if not os.path.exists(BIN):
        sys.exit("%s not found - run `make gif` (or build the host renderer first)" % BIN)

    tmp = tempfile.mkdtemp(prefix="deskbuddy-gif-")
    try:
        frames = []
        for args, start, count in CLIPS:
            clip = render(args, tmp)[start:start + count]
            frames += [panel(f) for f in clip[::STRIDE]]
    finally:
        shutil.rmtree(tmp, ignore_errors=True)

    os.makedirs(os.path.dirname(out_path) or ".", exist_ok=True)
    delays = [DELAYS[i % len(DELAYS)] for i in range(len(frames))]
    frames[0].save(out_path, save_all=True, append_images=frames[1:],
                   duration=delays, loop=0, optimize=True, disposal=1)

    size = os.path.getsize(out_path)
    print("%s: %dx%d, %d frames, %.1fs, %.0f kB"
          % (out_path, frames[0].width, frames[0].height, len(frames),
             sum(delays) / 1000.0, size / 1024.0))


if __name__ == "__main__":
    main()
