#include "TransformEngine.h"

namespace Zyri {

QImage TransformEngine::applyTransform(const QImage& src, const QTransform& transform,
                                        Qt::TransformationMode mode)
{
    return src.transformed(transform, mode);
}

QImage TransformEngine::flipHorizontal(const QImage& src)
{
    return src.mirrored(true, false);
}

QImage TransformEngine::flipVertical(const QImage& src)
{
    return src.mirrored(false, true);
}

} // namespace Zyri
