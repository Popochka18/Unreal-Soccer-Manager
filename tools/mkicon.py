#!/usr/bin/env python3
"""Generate the Tauri shell's placeholder application icons.

The icons are build assets, not game assets — they do not go in the
content-addressed store (§10), and they are committed because the Rust build
needs them present at compile time: `tauri::generate_context!` embeds the
default window icon and fails the build if it is missing.

They are generated rather than hand-drawn so that (a) nobody has to wonder
where a binary in the tree came from, and (b) the palette is read from
app/design/themes/default.json instead of being duplicated here. Re-run after
changing the theme:

    python3 tools/mkicon.py

Rendered at 8x and downsampled with Lanczos: the 32px icon is mostly a colour
blob at native resolution, and supersampling is what keeps its edges clean.
"""

from __future__ import annotations

import json
from pathlib import Path

from PIL import Image, ImageDraw

REPO_ROOT = Path(__file__).resolve().parent.parent
THEME_PATH = REPO_ROOT / "app/design/themes/default.json"
OUTPUT_DIR = REPO_ROOT / "app/tauri/icons"

# Tauri looks for icon.png as the default window icon; the sized variants are
# what the bundler will want at M6.
SIZES = {
    "icon.png": 512,
    "128x128@2x.png": 256,
    "128x128.png": 128,
    "32x32.png": 32,
}

SUPERSAMPLE = 8


def hex_to_rgb(value: str) -> tuple[int, int, int]:
    value = value.lstrip("#")
    return tuple(int(value[i : i + 2], 16) for i in (0, 2, 4))  # type: ignore[return-value]


def render(size: int, palette: dict[str, str]) -> Image.Image:
    """Draw a stylised pitch: rounded dark plate, green field, centre mark."""
    s = size * SUPERSAMPLE
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    plate = hex_to_rgb(palette["bgSunken"])
    field = hex_to_rgb(palette["positive"])
    line = hex_to_rgb(palette["text"])

    # Rounded plate covering the full canvas.
    draw.rounded_rectangle([(0, 0), (s - 1, s - 1)], radius=int(s * 0.22), fill=(*plate, 255))

    # The pitch, inset. Deliberately not full-bleed: at 32px the plate's corner
    # radius is the only thing distinguishing this from a green square.
    margin = int(s * 0.16)
    pitch = [(margin, int(s * 0.24)), (s - margin, s - int(s * 0.24))]
    draw.rounded_rectangle(pitch, radius=int(s * 0.03), fill=(*field, 255))

    stroke = max(1, int(s * 0.018))
    cx, cy = s // 2, s // 2

    # Halfway line and centre circle.
    draw.line([(cx, pitch[0][1]), (cx, pitch[1][1])], fill=(*line, 210), width=stroke)
    r = int(s * 0.11)
    draw.ellipse([(cx - r, cy - r), (cx + r, cy + r)], outline=(*line, 210), width=stroke)

    # Penalty areas, one per end.
    box_h = int(s * 0.19)
    box_w = int(s * 0.12)
    for x0, x1 in ((pitch[0][0], pitch[0][0] + box_w), (pitch[1][0] - box_w, pitch[1][0])):
        draw.rectangle([(x0, cy - box_h), (x1, cy + box_h)], outline=(*line, 210), width=stroke)

    return img.resize((size, size), Image.LANCZOS)


def main() -> None:
    theme = json.loads(THEME_PATH.read_text(encoding="utf-8"))
    palette = theme["color"]

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    for name, size in SIZES.items():
        path = OUTPUT_DIR / name
        render(size, palette).save(path, "PNG", optimize=True)
        print(f"mkicon: {path.relative_to(REPO_ROOT)} ({size}x{size}, {path.stat().st_size} B)")


if __name__ == "__main__":
    main()
