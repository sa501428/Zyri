# Zyri — Tiled Rendering System

## Why Tiles?

A 4K canvas (3840×2160) at 2× Retina = **8K physical pixels** to push per frame.
Redrawing the full canvas on every brush stroke event would make the UI choke on
even a fast Mac. The tiled system ensures **only modified regions are re-rasterized
and re-uploaded to the GPU**.

## Tile Size: 256×256

- 256×256 ARGB32_Premultiplied = **256 KB per tile** (uncompressed).
- A 4K canvas has ~255 tiles.
- 64×64 tiles → too many GL draw calls (4080 quads per layer at 4K).
- 512×512 tiles → dirtying too much area per brush stamp.
- **256 is the Krita/Photoshop industry standard.**

## Tile Coordinate System

Canvas pixel `(x, y)` maps to tile `(tx, ty)` as:
```
tx = floor(x / 256)
ty = floor(y / 256)
```

Within a tile, the local pixel is:
```
lx = x - tx * 256
ly = y - ty * 256
```

## RasterLayer Tile Storage

`RasterLayer` stores tiles as `QHash<TileKey, QImage>`.
- **Missing tile** = fully transparent (avoids allocating tiles for empty areas).
- **Copy-on-write before mutation**: always call `layer->tileAt()` (returns a
  detached copy), mutate it, then call `layer->setTile()`.

## Dirty Region Updates

When a brush stamp is applied:
1. `PaintEngine::stampAt()` calls `TileCache::markDirty(tx, ty)` for each
   intersected tile.
2. `CanvasViewport::invalidateTile()` converts the tile to a screen rect via
   `ViewTransform` and calls `QOpenGLWidget::update(screenRect)`.
3. Qt restricts the GL repaint scissor to that rect — **tiles outside the
   dirty region are not redrawn**.

## GPU Texture Upload

`TileCache::uploadTile()` calls `glTexImage2D` with `GL_BGRA` /
`GL_UNSIGNED_INT_8_8_8_8_REV` to match Qt's ARGB32_Premultiplied memory layout
on little-endian systems. This avoids a byte-swap copy.

## Retina / HiDPI

`ViewTransform` stores `devicePixelRatio` (DPR = 2.0 on Retina).
- Canvas coordinates are **logical pixels** throughout the codebase.
- The GL viewport and tile upload are in **physical pixels** (logical × DPR).
- `ViewTransform::canvasToScreen()` multiplies by `zoom × DPR`.
