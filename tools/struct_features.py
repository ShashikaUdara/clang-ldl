"""Structural features for glyph bitmaps (holes, skeleton stats)."""

from __future__ import annotations


def count_holes(bitmap: list[int], width: int, height: int) -> int:
    """Count enclosed background regions (holes) in a binary ink mask."""
    padded_w = width + 2
    padded_h = height + 2
    grid = [0] * (padded_w * padded_h)
    for y in range(height):
        for x in range(width):
            if bitmap[y * width + x] > 127:
                grid[(y + 1) * padded_w + (x + 1)] = 1

    visited = [False] * (padded_w * padded_h)
    holes = 0

    def idx(x: int, y: int) -> int:
        return y * padded_w + x

    def flood(x: int, y: int) -> bool:
        stack = [(x, y)]
        touches_border = False
        while stack:
            cx, cy = stack.pop()
            i = idx(cx, cy)
            if visited[i] or grid[i] != 0:
                continue
            visited[i] = True
            if cx == 0 or cy == 0 or cx == padded_w - 1 or cy == padded_h - 1:
                touches_border = True
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                nx, ny = cx + dx, cy + dy
                if 0 <= nx < padded_w and 0 <= ny < padded_h:
                    stack.append((nx, ny))
        return not touches_border

    for y in range(padded_h):
        for x in range(padded_w):
            if grid[idx(x, y)] == 0 and not visited[idx(x, y)]:
                if flood(x, y):
                    holes += 1
    return holes


def _skeletonize(bitmap: list[int], width: int, height: int) -> list[int]:
    """Simple morphological skeleton approximation."""
    src = bitmap[:]
    dst = [0] * len(src)
    changed = True
    while changed:
        changed = False
        dst = src[:]
        for y in range(1, height - 1):
            for x in range(1, width - 1):
                if src[y * width + x] <= 127:
                    continue
                neighbors = sum(
                    1
                    for dx, dy in ((-1, 0), (1, 0), (0, -1), (0, 1), (-1, -1), (1, -1), (-1, 1), (1, 1))
                    if src[(y + dy) * width + (x + dx)] > 127
                )
                if neighbors < 2:
                    dst[y * width + x] = 0
                    changed = True
        src = dst[:]
        for y in range(1, height - 1):
            for x in range(1, width - 1):
                if src[y * width + x] <= 127:
                    continue
                neighbors = sum(
                    1
                    for dx, dy in ((-1, 0), (1, 0), (0, -1), (0, 1))
                    if src[(y + dy) * width + (x + dx)] > 127
                )
                if neighbors == 4:
                    dst[y * width + x] = 0
                    changed = True
        src = dst[:]
    return src


def skeleton_stats(bitmap: list[int], width: int, height: int) -> tuple[int, int]:
    skel = _skeletonize(bitmap, width, height)
    endpoints = 0
    junctions = 0
    for y in range(height):
        for x in range(width):
            if skel[y * width + x] <= 127:
                continue
            n = sum(
                1
                for dx, dy in ((-1, 0), (1, 0), (0, -1), (0, 1), (-1, -1), (1, -1), (-1, 1), (1, 1))
                if 0 <= x + dx < width
                and 0 <= y + dy < height
                and skel[(y + dy) * width + (x + dx)] > 127
            )
            if n <= 1:
                endpoints += 1
            elif n >= 3:
                junctions += 1
    return endpoints, junctions


def compute_features(bitmap: list[int], width: int, height: int) -> dict[str, float | int]:
    ink = sum(1 for v in bitmap if v > 127)
    ys = [y for y in range(height) for x in range(width) if bitmap[y * width + x] > 127]
    xs = [x for y in range(height) for x in range(width) if bitmap[y * width + x] > 127]
    if ink == 0:
        return {"holes": 0, "endpoints": 0, "junctions": 0, "aspect": 1.0}
    bw = max(xs) - min(xs) + 1
    bh = max(ys) - min(ys) + 1
    holes = count_holes(bitmap, width, height)
    endpoints, junctions = skeleton_stats(bitmap, width, height)
    return {
        "holes": holes,
        "endpoints": min(endpoints, 255),
        "junctions": min(junctions, 255),
        "aspect": bw / max(bh, 1),
    }
