# Zyri — Undo/Redo Engine Design

## Problem: Memory Explosion

A naive 100-state undo history storing full tile snapshots for a 4K multi-layer
document can easily consume **16 GB+ of RAM**.

Example: 1 layer × 255 tiles × 256 KB × 100 states = **6.5 GB** uncompressed.

## Solution: XOR + zlib Delta Compression

`DeltaCompressor` compresses the *difference* between tile states:

```
delta = zlib_deflate(before XOR after)
```

For a typical brush stroke that modifies < 5% of a tile's pixels:
- The XOR buffer is ~95% zero.
- zlib deflates it to near-nothing.
- **Typical result: 256 KB tile → < 2 KB delta.**

100-state stack for a 4K canvas: ~50 MB worst case (still manageable).

## PaintTileCommand Structure

```cpp
struct TileDelta {
    TileKey    key;
    QByteArray beforeRaw;  // uncompressed pre-stroke snapshot
    QByteArray delta;      // compressed XOR(before, after)
};
```

On `execute()` (redo): `apply(beforeRaw, delta)` → reconstructed `after`.
On `undo()`: restore `beforeRaw` directly (no decompression needed).

## Stroke Coalescing

Multiple `PaintTileCommand` objects from a single brush stroke are wrapped in a
`MacroCommand` so they appear as a single undo step.

`CommandBase::mergeWith()` may further coalesce certain consecutive commands
(e.g. opacity slider adjustments) to avoid polluting the history.

## Ring Buffer with Capacity Limit

`UndoStack` uses a `std::vector` as a ring buffer. When the 100-entry limit is
reached, the oldest entry is removed from `m_commands[0]` (O(n) erase, but
only triggered once per ~100 strokes — negligible).

## Scratch Disk Overflow (Future)

If the in-memory undo stack exceeds a configurable threshold (default 256 MB),
a `TileSwapFile` pages least-recently-used deltas to:
```
~/Library/Caches/Zyri/undo-<pid>.tmp
```
This is a Phase 7 enhancement — not required for v1.
