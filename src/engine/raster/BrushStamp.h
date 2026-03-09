#pragma once
#include <QColor>
#include <QImage>
#include <QPointF>

namespace Zyri {

// Renders a single circular brush stamp onto a tile image using QPainter.
// The stamp uses a radial gradient to implement hardness (soft/hard edges).
class BrushStamp {
public:
    // Paint a stamp centered at (cx, cy) in tile-local coordinates.
    // The tile must be ARGB32_Premultiplied.
    static void render(QImage& tile,
                       const QPointF& center,
                       float radius,
                       float hardness,
                       float opacity,
                       const QColor& color,
                       bool eraser = false);
};

} // namespace Zyri
