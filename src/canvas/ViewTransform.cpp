#include "ViewTransform.h"
#include <algorithm>

namespace Zyri {

void ViewTransform::setZoom(qreal zoom)
{
    m_zoom = std::clamp(zoom, 0.05, 64.0);
}

void ViewTransform::setPan(const QPointF& canvasCenter)
{
    m_canvasCenter = canvasCenter;
}

void ViewTransform::pan(const QPointF& delta)
{
    // delta is in screen pixels; convert to canvas pixels before subtracting.
    m_canvasCenter -= QPointF(delta.x() / (m_zoom * m_dpr),
                               delta.y() / (m_zoom * m_dpr));
}

QPointF ViewTransform::toCanvas(const QPointF& screenPt) const
{
    // Inverse of canvasToScreen():
    // screenPt = (canvasPt - center) * zoom * dpr + viewportCenter
    // → canvasPt = (screenPt - viewportCenter) / (zoom * dpr) + center
    // NOTE: viewportCenter is not stored here; callers must offset screenPt themselves.
    const qreal scale = m_zoom * m_dpr;
    return QPointF(screenPt.x() / scale + m_canvasCenter.x(),
                   screenPt.y() / scale + m_canvasCenter.y());
}

QPointF ViewTransform::toScreen(const QPointF& canvasPt) const
{
    const qreal scale = m_zoom * m_dpr;
    return QPointF((canvasPt.x() - m_canvasCenter.x()) * scale,
                   (canvasPt.y() - m_canvasCenter.y()) * scale);
}

QTransform ViewTransform::canvasToScreen() const
{
    const qreal scale = m_zoom * m_dpr;
    QTransform t;
    t.scale(scale, scale);
    t.translate(-m_canvasCenter.x(), -m_canvasCenter.y());
    return t;
}

QRectF ViewTransform::visibleCanvasRect(const QSize& viewportSize) const
{
    const qreal scale = m_zoom * m_dpr;
    const qreal hw = viewportSize.width()  / (2.0 * scale);
    const qreal hh = viewportSize.height() / (2.0 * scale);
    return QRectF(m_canvasCenter.x() - hw, m_canvasCenter.y() - hh,
                  hw * 2.0, hh * 2.0);
}

} // namespace Zyri
