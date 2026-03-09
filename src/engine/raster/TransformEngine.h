#pragma once
#include <QImage>
#include <QTransform>

namespace Zyri {
// Applies geometric transforms (scale, rotate, flip) to raster regions.
class TransformEngine {
public:
    static QImage applyTransform(const QImage& src, const QTransform& transform,
                                  Qt::TransformationMode mode = Qt::SmoothTransformation);
    static QImage flipHorizontal(const QImage& src);
    static QImage flipVertical(const QImage& src);
};
} // namespace Zyri
