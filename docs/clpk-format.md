# CLPK — clang-ldl glyph pack format (v1)

Binary template pack for logical OCR. Built by `tools/pack_builder.py`, loaded by `native/src/pack_loader.cpp`.

## File layout (little-endian)

| Offset | Type | Field |
|--------|------|-------|
| 0 | char[4] | Magic `CLPK` |
| 4 | uint32 | Version (= 1) |
| 8 | uint16 | `pack_id` byte length |
| 10 | bytes | `pack_id` UTF-8 (e.g. `latin`) |
| | uint16 | `grid_w` |
| | uint16 | `grid_h` |
| | float32 | `threshold` — minimum fused score |
| | float32 | `w_ncc` |
| | float32 | `w_struct` |
| | float32 | `w_aspect` |
| | uint32 | `glyph_count` |
| | repeat `glyph_count` | Glyph record (below) |

### Glyph record

| Type | Field |
|------|-------|
| uint32 | `codepoint` |
| uint8 | `holes` |
| uint8 | `endpoints` |
| uint8 | `junctions` |
| uint8 | padding |
| float32 | `aspect` |
| uint32 | `bitmap_len` (= grid_w × grid_h) |
| bytes | `bitmap` — row-major 0/255 |

## Default Latin pack

- Grid: 20×28
- Codepoints: ASCII 32–126
- Source: embedded 5×7 terminal font @ 4× scale (parity with legacy matcher)
