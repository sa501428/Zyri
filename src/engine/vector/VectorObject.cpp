#include "VectorObject.h"

namespace Zyri {

VectorObject::VectorObject(const QPainterPath& path)
    : m_path(path)
{}

void VectorObject::paintTo(QPainter& painter) const
{
    painter.save();
    painter.setOpacity(static_cast<qreal>(m_opacity));
    if (m_fill.alpha() > 0)
        painter.fillPath(m_path, m_fill);
    if (m_stroke.alpha() > 0 && m_strokeWidth > 0.0f) {
        QPen pen(m_stroke, static_cast<qreal>(m_strokeWidth));
        painter.strokePath(m_path, pen);
    }
    painter.restore();
}

} // namespace Zyri
