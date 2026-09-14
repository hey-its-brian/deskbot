#!/usr/bin/env python3
"""Turn raw 128x64 preview frames into PNGs (contact sheet or filmstrip).

Pure stdlib - zlib is all a PNG writer really needs.

  frames_to_png.py frames.bin out.png [--cols 4] [--scale 3] [--labels a,b,c]
"""
import struct
import sys
import zlib

W, H = 128, 64


def png(path, w, h, rgb_rows):
    raw = b"".join(b"\x00" + row for row in rgb_rows)

    def chunk(tag, data):
        c = tag + data
        return struct.pack(">I", len(data)) + c + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF)

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(raw, 9)))
        f.write(chunk(b"IEND", b""))


# 5x7 pixel font, enough for the emotion captions.
FONT = {
    "A": ["01110", "10001", "10001", "11111", "10001", "10001", "10001"],
    "B": ["11110", "10001", "11110", "10001", "10001", "10001", "11110"],
    "C": ["01110", "10001", "10000", "10000", "10000", "10001", "01110"],
    "D": ["11110", "10001", "10001", "10001", "10001", "10001", "11110"],
    "E": ["11111", "10000", "11110", "10000", "10000", "10000", "11111"],
    "F": ["11111", "10000", "11110", "10000", "10000", "10000", "10000"],
    "G": ["01110", "10001", "10000", "10111", "10001", "10001", "01111"],
    "H": ["10001", "10001", "11111", "10001", "10001", "10001", "10001"],
    "I": ["11111", "00100", "00100", "00100", "00100", "00100", "11111"],
    "L": ["10000", "10000", "10000", "10000", "10000", "10000", "11111"],
    "M": ["10001", "11011", "10101", "10001", "10001", "10001", "10001"],
    "N": ["10001", "11001", "10101", "10011", "10001", "10001", "10001"],
    "O": ["01110", "10001", "10001", "10001", "10001", "10001", "01110"],
    "P": ["11110", "10001", "10001", "11110", "10000", "10000", "10000"],
    "R": ["11110", "10001", "10001", "11110", "10100", "10010", "10001"],
    "S": ["01111", "10000", "10000", "01110", "00001", "00001", "11110"],
    "T": ["11111", "00100", "00100", "00100", "00100", "00100", "00100"],
    "U": ["10001", "10001", "10001", "10001", "10001", "10001", "01110"],
    "V": ["10001", "10001", "10001", "10001", "10001", "01010", "00100"],
    "W": ["10001", "10001", "10001", "10101", "10101", "11011", "10001"],
    "X": ["10001", "10001", "01010", "00100", "01010", "10001", "10001"],
    "Y": ["10001", "10001", "01010", "00100", "00100", "00100", "00100"],
    "Z": ["11111", "00001", "00010", "00100", "01000", "10000", "11111"],
    " ": ["00000"] * 7,
    "-": ["00000", "00000", "00000", "11111", "00000", "00000", "00000"],
    "0": ["01110", "10001", "10011", "10101", "11001", "10001", "01110"],
    "1": ["00100", "01100", "00100", "00100", "00100", "00100", "01110"],
    "2": ["01110", "10001", "00001", "00110", "01000", "10000", "11111"],
    "3": ["11111", "00010", "00100", "00010", "00001", "10001", "01110"],
    "4": ["00010", "00110", "01010", "10010", "11111", "00010", "00010"],
    "5": ["11111", "10000", "11110", "00001", "00001", "10001", "01110"],
    "6": ["00110", "01000", "10000", "11110", "10001", "10001", "01110"],
    "7": ["11111", "00001", "00010", "00100", "01000", "01000", "01000"],
    "8": ["01110", "10001", "10001", "01110", "10001", "10001", "01110"],
    "9": ["01110", "10001", "10001", "01111", "00001", "00010", "01100"],
}


def blit_text(canvas, cw, x, y, text, colour):
    for ch in text.upper():
        glyph = FONT.get(ch)
        if glyph:
            for gy, row in enumerate(glyph):
                for gx, bit in enumerate(row):
                    if bit == "1":
                        px, py = x + gx, y + gy
                        if 0 <= px < cw and 0 <= py < len(canvas) // cw:
                            canvas[py * cw + px] = colour
        x += 6
    return x


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    src, dst = sys.argv[1], sys.argv[2]
    cols, scale, labels = 4, 3, []
    args = sys.argv[3:]
    for i, a in enumerate(args):
        if i + 1 >= len(args):
            break
        if a == "--cols":
            cols = max(1, int(args[i + 1]))
        elif a == "--scale":
            scale = max(1, int(args[i + 1]))
        elif a == "--labels":
            labels = args[i + 1].split(",")

    data = open(src, "rb").read()
    n = len(data) // (W * H)
    if n == 0:
        print("no frames")
        return 1
    cols = min(cols, n)
    rows = (n + cols - 1) // cols

    pad, label_h = 6, (10 if labels else 0)
    cell_w, cell_h = W * scale + pad, H * scale + pad + label_h
    cw, ch = cols * cell_w + pad, rows * cell_h + pad

    BG, FRAME, ON, TXT = 24, 60, 255, 150
    canvas = [BG] * (cw * ch)

    for idx in range(n):
        fr = data[idx * W * H:(idx + 1) * W * H]
        cx = pad + (idx % cols) * cell_w
        cy = pad + (idx // cols) * cell_h
        # panel border
        for x in range(-1, W * scale + 1):
            for yy in (-1, H * scale):
                canvas[(cy + yy) * cw + cx + x] = FRAME
        for y in range(-1, H * scale + 1):
            for xx in (-1, W * scale):
                canvas[(cy + y) * cw + cx + xx] = FRAME
        for y in range(H):
            for x in range(W):
                if fr[y * W + x]:
                    for sy in range(scale):
                        row = (cy + y * scale + sy) * cw
                        for sx in range(scale):
                            canvas[row + cx + x * scale + sx] = ON
        if labels and idx < len(labels):
            blit_text(canvas, cw, cx, cy + H * scale + 3, labels[idx], TXT)

    rgb = []
    for y in range(ch):
        row = bytearray()
        for x in range(cw):
            v = canvas[y * cw + x]
            row += bytes((v, v, v))
        rgb.append(bytes(row))
    png(dst, cw, ch, rgb)
    print("wrote %s (%d frames, %dx%d)" % (dst, n, cw, ch))
    return 0


if __name__ == "__main__":
    sys.exit(main())
