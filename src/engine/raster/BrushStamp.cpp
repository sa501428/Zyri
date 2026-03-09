#include "BrushStamp.h"
#include <QPainter>
#include <QRadialGradient>

namespace Zyri {

void BrushStamp::render(QImage& tile,
                         const QPointF& center,
                         float radius,
                         float hardness,
                         float opacity,
                         const QColor& color,
                         bool eraser)
{
    if (radius <= 0.0f) return;

    QPainter p(&tile);
    p.setRenderHint(QPainter::Antialiasing);

    if (eraser) {
        p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
    } else {
        p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    }

    // Build a radial gradient to simulate brush hardness.
    // hardness=1 → hard edge; hardness=0 → fully feathered to transparent.
    QRadialGradient grad(center, static_cast<qreal>(radius));

    QColor centerColor = eraser ? Qt::black : color;
    centerColor.setAlphaF(static_cast<qreal>(opacity));

    QColor edgeColor = centerColor;
    edgeColor.setAlphaF(0.0);

    // The hardness parameter controls where the fade begins:
    // at hardness=1.0 the fade starts at 100% of the radius (sharp),
    // at hardness=0.0 the fade starts at 0% (fully soft).
    const qreal fadeStart = static_cast<qreal>(hardness);
    if (fadeStart < 1.0) {
        grad.setColorAt(0,          centerColor);
        grad.setColorAt(fadeStart,  centerColor);
        grad.setColorAt(1.0,        edgeColor);
    } else {
        grad.setColorAt(0,   centerColor);
        grad.setColorAt(1.0, centerColor);
    }

    p.setBrush(QBrush(grad));
    p.setPen(Qt::NoPen);
    const qreal r = static_cast<qreal>(radius);
    p.drawEllipse(center, r, r);
}

} // namespace Zyri
