#pragma once
#include <QImage>
#include <QRect>

namespace Zyri {
class VectorScene;

// Rasterizes a VectorScene into a 256x256 tile image for GL upload.
class VectorRasterizer {
public:
    // Render the scene into a tile covering canvas pixels [tileOrigin, tileOrigin+kTileSize).
    static QImage rasterizeTile(const VectorScene& scene,
                                  const QPoint& tileOrigin,
                                  float devicePixelRatio = 1.0f);
};
} // namespace Zyri
