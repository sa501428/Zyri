#pragma once
#include <QPointF>
#include <QRectF>
#include <QTransform>

namespace Zyri {

// Manages the canvas-to-screen transform (pan, zoom, Retina DPR).
//
// All canvas coordinates are in logical canvas pixels.
// Screen coordinates are in physical device pixels (accounting for DPR).
class ViewTransform {
public:
    ViewTransform() = default;

    void setDevicePixelRatio(qreal dpr) { m_dpr = dpr; }
    void setZoom(qreal zoom);
    void setPan(const QPointF& canvasCenter);
    void pan(const QPointF& delta); // delta in screen pixels

    [[nodiscard]] qreal     zoom()            const { return m_zoom; }
    [[nodiscard]] QPointF   canvasCenter()    const { return m_canvasCenter; }
    [[nodiscard]] qreal     dpr()             const { return m_dpr; }

    // Map a screen point to canvas coordinates.
    [[nodiscard]] QPointF   toCanvas(const QPointF& screenPt)  const;

    // Map a canvas point to screen coordinates.
    [[nodiscard]] QPointF   toScreen(const QPointF& canvasPt)  const;

    // The full canvas→screen transform (for GL matrix upload).
    [[nodiscard]] QTransform canvasToScreen() const;

    // Returns the rectangle of canvas coordinates visible in the given viewport size.
    [[nodiscard]] QRectF    visibleCanvasRect(const QSize& viewportSize) const;

private:
    qreal   m_zoom          = 1.0;
    qreal   m_dpr           = 1.0;
    QPointF m_canvasCenter  = {0, 0};
};

} // namespace Zyri
