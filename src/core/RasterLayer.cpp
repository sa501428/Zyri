#include "RasterLayer.h"

namespace Zyri {

RasterLayer::RasterLayer(const QString& name)
    : Layer(LayerType::Raster, name.isEmpty() ? QStringLiteral("Raster Layer") : name)
{
}

QImage RasterLayer::tileAt(int tx, int ty) const
{
    auto it = m_tiles.find({tx, ty});
    if (it == m_tiles.end()) {
        // Return a transparent tile without caching it.
        QImage blank(kTileSize, kTileSize, QImage::Format_ARGB32_Premultiplied);
        blank.fill(Qt::transparent);
        return blank;
    }
    return it->copy(); // detached copy — caller may modify freely
}

void RasterLayer::setTile(int tx, int ty, const QImage& img)
{
    Q_ASSERT(img.width() == kTileSize && img.height() == kTileSize);
    Q_ASSERT(img.format() == QImage::Format_ARGB32_Premultiplied);
    m_tiles[{tx, ty}] = img;
}

void RasterLayer::clearTile(int tx, int ty)
{
    m_tiles.remove({tx, ty});
}

bool RasterLayer::hasTile(int tx, int ty) const
{
    return m_tiles.contains({tx, ty});
}

} // namespace Zyri
