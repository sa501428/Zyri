#include "VectorRasterizer.h"
#include "VectorScene.h"
#include "../../core/RasterLayer.h"
#include <QPainter>

namespace Zyri {

QImage VectorRasterizer::rasterizeTile(const VectorScene& scene,
                                        const QPoint& tileOrigin,
                                        float devicePixelRatio)
{
    QImage tile(kTileSize, kTileSize, QImage::Format_ARGB32_Premultiplied);
    tile.fill(Qt::transparent);

    QPainter painter(&tile);
    painter.setRenderHint(QPainter::Antialiasing);
    // Translate so that canvas coordinates map to tile-local pixels.
    painter.translate(-tileOrigin.x() * devicePixelRatio,
                      -tileOrigin.y() * devicePixelRatio);
    painter.scale(devicePixelRatio, devicePixelRatio);

    scene.paintTo(painter);
    return tile;
}

} // namespace Zyri
