#include "FillEngine.h"
#include "SelectionMask.h"
#include <QStack>
#include <cmath>

namespace Zyri {

static inline int colorDiff(QRgb a, QRgb b)
{
    return std::abs(qRed(a)   - qRed(b))
         + std::abs(qGreen(a) - qGreen(b))
         + std::abs(qBlue(a)  - qBlue(b))
         + std::abs(qAlpha(a) - qAlpha(b));
}

void FillEngine::fill(QImage& image, const QPoint& seed,
                       const QColor& fillColor, int tolerance,
                       const SelectionMask* mask)
{
    if (image.isNull()) return;
    if (!image.rect().contains(seed)) return;

    QImage img = image.convertToFormat(QImage::Format_ARGB32);
    const int w = img.width(), h = img.height();
    const QRgb targetColor = img.pixel(seed);
    const QRgb newColor    = fillColor.rgba();

    if (targetColor == newColor) return;

    QVector<bool> visited(w * h, false);
    QStack<QPoint> stack;
    stack.push(seed);

    while (!stack.isEmpty()) {
        const QPoint p = stack.pop();
        if (p.x() < 0 || p.x() >= w || p.y() < 0 || p.y() >= h) continue;

        const int idx = p.y() * w + p.x();
        if (visited[idx]) continue;
        visited[idx] = true;

        // Check selection mask constraint.
        if (mask && !mask->isEmpty() && mask->at(p.x(), p.y()) == 0) continue;

        const QRgb pixel = img.pixel(p);
        if (colorDiff(pixel, targetColor) > tolerance) continue;

        img.setPixel(p, newColor);

        stack.push({p.x() + 1, p.y()});
        stack.push({p.x() - 1, p.y()});
        stack.push({p.x(), p.y() + 1});
        stack.push({p.x(), p.y() - 1});
    }

    image = img.convertToFormat(QImage::Format_ARGB32_Premultiplied);
}

} // namespace Zyri
