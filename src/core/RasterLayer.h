#pragma once

#include "Layer.h"
#include <QHash>
#include <QImage>
#include <QPoint>

namespace Zyri {

// Tile size used throughout the engine (256x256 ARGB32_Premultiplied).
static constexpr int kTileSize = 256;

struct TileKey {
    int tx, ty;
    bool operator==(const TileKey& o) const noexcept { return tx == o.tx && ty == o.ty; }
};

inline size_t qHash(const TileKey& k, size_t seed = 0) noexcept
{
    return qHashMulti(seed, k.tx, k.ty);
}

class RasterLayer : public Layer
{
public:
    explicit RasterLayer(const QString& name = {});

    // Returns the tile image at grid position (tx, ty).
    // If the tile has never been written to, returns a transparent tile.
    // The returned QImage is a detached copy — safe to modify.
    [[nodiscard]] QImage tileAt(int tx, int ty) const;

    // Write a complete tile back. The image must be kTileSize x kTileSize
    // ARGB32_Premultiplied.
    void setTile(int tx, int ty, const QImage& img);

    // Remove a tile (sets it back to fully transparent).
    void clearTile(int tx, int ty);

    // True if the tile at (tx, ty) has ever been written to.
    [[nodiscard]] bool hasTile(int tx, int ty) const;

    [[nodiscard]] const QHash<TileKey, QImage>& tiles() const { return m_tiles; }

private:
    QHash<TileKey, QImage> m_tiles;
};

} // namespace Zyri
