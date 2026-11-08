"""Turn the LED matrix dumps in a Wokwi serial log into PNG files.

Wokwi cannot take screenshots of its NeoPixel canvas, so the firmware built by
scripts/wokwi.sh prints the 32x8 LED colors as a "WOKWI_SCREEN <hex>" line when
it receives the "screen" serial command (see src/WokwiConsole.h). Scenarios
send that command; this script saves every dump in the log as a PNG and can
print it as text.

Usage:
    python3 scripts/wokwi_screen.py logs/wokwi.log [--out-dir logs/wokwi_screens] [--ascii]

The files are named screen_01.png, screen_02.png, ... in log order. --ascii
also prints each dump with one character per LED: "." off, otherwise the first
letter of the nearest basic color (R, G, B, Y, C, M, W).
"""
import argparse
import struct
import zlib
from pathlib import Path

WIDTH, HEIGHT = 32, 8
SCALE = 12  # Output pixels per LED, including a dark gap around each LED
GAP = 2
MARKER = "WOKWI_SCREEN "
BASIC_COLORS = {
    "R": (255, 0, 0), "G": (0, 255, 0), "B": (0, 0, 255), "Y": (255, 255, 0),
    "C": (0, 255, 255), "M": (255, 0, 255), "W": (255, 255, 255),
}


def read_screens(log_path):
    screens = []
    for line in Path(log_path).read_text(errors="replace").splitlines():
        position = line.find(MARKER)
        if position < 0:
            continue
        data = line[position + len(MARKER):].strip()
        if len(data) != WIDTH * HEIGHT * 6:
            print(f"Skipping a truncated dump ({len(data)} characters)")
            continue
        screens.append([tuple(bytes.fromhex(data[i:i + 6])) for i in range(0, len(data), 6)])
    return screens


def write_png(pixels, path):
    rows = []
    for y in range(HEIGHT * SCALE):
        row = bytearray([0])  # PNG filter type "none"
        for x in range(WIDTH * SCALE):
            inside = x % SCALE >= GAP and y % SCALE >= GAP
            row += bytes(pixels[(y // SCALE) * WIDTH + x // SCALE]) if inside else b"\x10\x10\x10"
        rows.append(bytes(row))

    def chunk(kind, body):
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body))

    header = struct.pack(">IIBBBBB", WIDTH * SCALE, HEIGHT * SCALE, 8, 2, 0, 0, 0)
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header)
    png += chunk(b"IDAT", zlib.compress(b"".join(rows))) + chunk(b"IEND", b"")
    path.write_bytes(png)


def letter(color):
    if not any(color):
        return "."
    peak = max(color)
    normalized = tuple(c * 255 // peak for c in color)
    return min(BASIC_COLORS, key=lambda k: sum((a - b) ** 2 for a, b in zip(BASIC_COLORS[k], normalized)))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("log")
    parser.add_argument("--out-dir", default="logs/wokwi_screens")
    parser.add_argument("--ascii", action="store_true")
    args = parser.parse_args()

    screens = read_screens(args.log)
    if not screens:
        raise SystemExit(f"No {MARKER.strip()} lines in {args.log}")
    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    for number, pixels in enumerate(screens, 1):
        path = out_dir / f"screen_{number:02d}.png"
        write_png(pixels, path)
        print(f"Saved {path}")
        if args.ascii:
            for y in range(HEIGHT):
                print("".join(letter(pixels[y * WIDTH + x]) for x in range(WIDTH)))


if __name__ == "__main__":
    main()
