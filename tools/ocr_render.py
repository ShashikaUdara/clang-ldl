"""Shared TTF glyph rendering for pack building and synthetic OCR tests."""

from __future__ import annotations

from pathlib import Path

GRID_W = 20
GRID_H = 28
TIER_A_GRID_W = 28
TIER_A_GRID_H = 40
TIER_B_SYLLABLE_GRID_W = 40
TIER_B_SYLLABLE_GRID_H = 56
TIER_C_GRID_W = 32
TIER_C_GRID_H = 48
FONT_SIZE = 56
LETTER_SPACING = 14
INK_THRESHOLD = 180


def _hard_binarize(img):
    gray = img.convert("L")
    return gray.point(lambda p: 0 if p < INK_THRESHOLD else 255)

_FONT_CANDIDATES = [
    "/usr/share/fonts/truetype/noto/{name}",
    "/usr/share/fonts/truetype/dejavu/{name}",
    "/usr/share/fonts/truetype/liberation/{name}",
]


def resolve_font(filename: str) -> Path:
    for pattern in _FONT_CANDIDATES:
        path = Path(pattern.format(name=filename))
        if path.is_file():
            return path
    raise FileNotFoundError(f"font not found: {filename} (install Noto fonts)")


def _line_metrics(font) -> int:
    from PIL import Image, ImageDraw

    probe = Image.new("RGB", (1, 1))
    draw = ImageDraw.Draw(probe)
    bbox = draw.textbbox((0, 0), "Mg", font=font)
    return bbox[3] - bbox[1] + FONT_SIZE // 4


def normalize_to_grid(
    bitmap: list[int],
    gw: int,
    gh: int,
    *,
    grid_w: int = GRID_W,
    grid_h: int = GRID_H,
) -> list[int]:
    """Mirror of C++ normalize_glyph_bitmap output (fixed grid, centered)."""
    out = [0] * (grid_w * grid_h)
    if gw <= 0 or gh <= 0:
        return out
    scale = min(grid_w / gw, grid_h / gh)
    nw = max(1, int(gw * scale))
    nh = max(1, int(gh * scale))
    ox = (grid_w - nw) // 2
    oy = (grid_h - nh) // 2
    for y in range(nh):
        for x in range(nw):
            sx = min(gw - 1, int(x / scale))
            sy = min(gh - 1, int(y / scale))
            if bitmap[sy * gw + sx] > 127:
                out[(oy + y) * grid_w + (ox + x)] = 255
    return out


def render_cell_raw_bitmap(
    codepoint: int,
    font_path: Path | str,
    *,
    font_size: int = FONT_SIZE,
    letter_spacing: int = LETTER_SPACING,
) -> tuple[list[int], int, int]:
    """Render one fixed-width cell (matches OCR test line layout)."""
    from PIL import Image, ImageDraw, ImageFont

    font = ImageFont.truetype(str(font_path), font_size)
    adv = font_size + letter_spacing
    line_h = _line_metrics(font)
    img = Image.new("L", (adv, line_h), 255)
    draw = ImageDraw.Draw(img)
    draw.text((0, 0), chr(codepoint), fill=0, font=font)
    img = _hard_binarize(img)
    w, h = adv, line_h
    pix = [255 if p > 127 else 0 for p in img.getdata()]

    def at(x: int, y: int) -> int:
        return pix[y * w + x]

    line_miny, line_maxy = h, 0
    for y in range(h):
        for x in range(w):
            if at(x, y) > 127:
                line_miny = min(line_miny, y)
                line_maxy = max(line_maxy, y)
    if line_maxy < line_miny:
        return [], 0, 0

    col_sum = [sum(1 for y in range(h) if at(x, y) > 127) for x in range(w)]
    spans: list[tuple[int, int]] = []
    x = 0
    while x < w:
        while x < w and col_sum[x] == 0:
            x += 1
        if x >= w:
            break
        x0 = x
        while x < w and col_sum[x] > 0:
            x += 1
        spans.append((x0, x - x0))
    if not spans:
        return [], 0, 0

    x0, gw = spans[0]
    lh = line_maxy - line_miny + 1
    raw: list[int] = []
    for yy in range(lh):
        for xx in range(gw):
            raw.append(at(x0 + xx, line_miny + yy))
    return raw, gw, lh


def extract_first_glyph_bitmap(
    image_path: Path,
    *,
    grid_w: int = GRID_W,
    grid_h: int = GRID_H,
) -> list[int]:
    """Extract first projection glyph from a line image (matches C++ segment_glyphs)."""
    from PIL import Image

    img = Image.open(image_path).convert("L")
    w, h = img.size
    px = img.load()
    col = [sum(1 for y in range(h) if px[x, y] < 128) for x in range(w)]
    spans: list[tuple[int, int]] = []
    x = 0
    while x < w:
        while x < w and col[x] == 0:
            x += 1
        if x >= w:
            break
        x0 = x
        while x < w and col[x] > 0:
            x += 1
        if x - x0 >= 2:
            spans.append((x0, x - x0))
    if not spans:
        return [0] * (grid_w * grid_h)
    widths = sorted(span[1] for span in spans)
    min_span_w = max(6, widths[len(widths) // 2] // 2)
    spans = [s for s in spans if s[1] >= min_span_w]
    if not spans:
        return [0] * (grid_w * grid_h)
    line_miny, line_maxy = h, 0
    for y in range(h):
        for x in range(w):
            if px[x, y] < 128:
                line_miny = min(line_miny, y)
                line_maxy = max(line_maxy, y)
    lh = line_maxy - line_miny + 1
    x0, gw = spans[0]
    raw = [255 if px[x0 + xx, line_miny + yy] < 128 else 0 for yy in range(lh) for xx in range(gw)]
    return normalize_to_grid(raw, gw, lh, grid_w=grid_w, grid_h=grid_h)


def render_cell_glyph_bitmap(
    codepoint: int,
    font_path: Path | str,
    *,
    grid_w: int = GRID_W,
    grid_h: int = GRID_H,
    font_size: int = FONT_SIZE,
    letter_spacing: int = LETTER_SPACING,
    rtl: bool = False,
) -> list[int]:
    """Pack template via the same line renderer used in OCR tests."""
    import tempfile

    with tempfile.TemporaryDirectory() as td:
        img_path = Path(td) / "glyph.png"
        render_ocr_png(
            chr(codepoint),
            img_path,
            font_path,
            font_size=font_size,
            letter_spacing=letter_spacing,
            rtl=rtl,
        )
        return extract_first_glyph_bitmap(img_path, grid_w=grid_w, grid_h=grid_h)


def render_ocr_png(
    text: str,
    out_path: Path,
    font_path: Path | str,
    *,
    font_size: int = FONT_SIZE,
    letter_spacing: int = LETTER_SPACING,
    margin: int = 24,
    rtl: bool = False,
) -> Path:
    """Render a line with fixed advance width for reliable column-gap segmentation."""
    from PIL import Image, ImageDraw, ImageFont

    font = ImageFont.truetype(str(font_path), font_size)
    line_h = _line_metrics(font)
    adv = font_size + letter_spacing
    width = margin * 2 + max(adv, len(text) * adv)
    height = margin * 2 + line_h

    img = Image.new("RGB", (width, height), color="white")
    draw = ImageDraw.Draw(img)
    y = margin
    if rtl:
        x = width - margin - adv
        for ch in text:
            draw.text((x, y), ch, fill="black", font=font)
            x -= adv
    else:
        x = margin
        for ch in text:
            draw.text((x, y), ch, fill="black", font=font)
            x += adv

    gray = _hard_binarize(img)
    img = Image.merge("RGB", (gray, gray, gray))

    out_path.parent.mkdir(parents=True, exist_ok=True)
    img.save(out_path)
    return out_path
