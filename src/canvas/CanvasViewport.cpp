#include "CanvasViewport.h"
#include "../core/Document.h"
#include "../core/DocumentController.h"
#include "../core/RasterLayer.h"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QNativeGestureEvent>
#include <QWheelEvent>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <cmath>

namespace Zyri {

CanvasViewport::CanvasViewport(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_AcceptTouchEvents);
    grabGesture(Qt::PinchGesture);
}

CanvasViewport::~CanvasViewport()
{
    makeCurrent();
    m_tileCache.clear();
    m_background.destroy();
    m_compositor.destroy();
    m_overlay.destroy();
    doneCurrent();
}

void CanvasViewport::setDocument(Document* doc)
{
    m_document = doc;
    if (doc) {
        // Centre the view on the canvas so it appears in the middle of the
        // viewport on first show.
        const QSizeF cs = doc->canvasSize();
        m_viewTransform.setPan({cs.width() / 2.0, cs.height() / 2.0});
    }
    update();
}

void CanvasViewport::setController(DocumentController* ctrl)
{
    m_controller = ctrl;
}

void CanvasViewport::invalidateTile(int tx, int ty)
{
    m_tileCache.markDirty({tx, ty});

    // Compute the dirty screen rect in widget (logical) coordinates.
    // toScreen() returns position relative to the viewport centre, so we
    // shift by the widget centre to get widget-origin coordinates.
    const QPointF centre(width() / 2.0, height() / 2.0);
    const QPointF tl = centre + m_viewTransform.toScreen(
                           {double(tx * kTileSize), double(ty * kTileSize)});
    const QPointF br = centre + m_viewTransform.toScreen(
                           {double((tx + 1) * kTileSize), double((ty + 1) * kTileSize)});
    update(QRect(tl.toPoint(), br.toPoint()).normalized().adjusted(-1, -1, 1, 1));
}

// ── GL lifecycle ─────────────────────────────────────────────────────────────

void CanvasViewport::initializeGL()
{
    auto* f = QOpenGLContext::currentContext()->functions();
    f->glDisable(GL_DEPTH_TEST);
    f->glDisable(GL_CULL_FACE);

    m_tileCache.initialize();
    m_background.initialize();
    m_compositor.initialize();
    m_overlay.initialize();

    m_viewTransform.setDevicePixelRatio(devicePixelRatio());
}

void CanvasViewport::resizeGL(int /*w*/, int /*h*/)
{
    m_viewTransform.setDevicePixelRatio(devicePixelRatio());
}

void CanvasViewport::paintGL()
{
    auto* f = QOpenGLContext::currentContext()->functions();
    f->glClearColor(0.18f, 0.18f, 0.18f, 1.0f); // dark-grey surround
    f->glClear(GL_COLOR_BUFFER_BIT);

    if (!m_document) return;

    const QSize vpSize(width(), height());
    const QSize canvasSize = m_document->canvasSize();

    m_background.draw(canvasSize, m_viewTransform, vpSize);
    m_compositor.composite(m_document->layers(), m_viewTransform, vpSize, m_tileCache);
    m_overlay.draw(canvasSize, m_viewTransform, vpSize);
}

// ── Input ─────────────────────────────────────────────────────────────────────

bool CanvasViewport::event(QEvent* e)
{
    // IMPORTANT: Intercept QTabletEvent here, not in tabletEvent(),
    // because tabletEvent() is not reliably delivered on macOS + Qt 6.
    if (e->type() == QEvent::TabletPress  ||
        e->type() == QEvent::TabletMove   ||
        e->type() == QEvent::TabletRelease)
    {
        // Tool dispatch implemented in Phase 3.
        e->accept();
        return true;
    }

    if (e->type() == QEvent::NativeGesture) {
        auto* ge = static_cast<QNativeGestureEvent*>(e);
        if (ge->gestureType() == Qt::ZoomNativeGesture) {
            const qreal factor = 1.0 + ge->value();
            m_viewTransform.setZoom(m_viewTransform.zoom() * factor);
            update();
            return true;
        }
    }

    return QOpenGLWidget::event(e);
}

void CanvasViewport::wheelEvent(QWheelEvent* e)
{
    if (e->modifiers() & Qt::ControlModifier) {
        // Zoom to cursor: keep the canvas point under the cursor fixed.
        const QPointF cursorLocal = e->position() - QPointF(width() / 2.0, height() / 2.0);
        const QPointF canvasPt    = m_viewTransform.toCanvas(cursorLocal);

        const qreal delta  = e->angleDelta().y() / 120.0;
        const qreal factor = std::pow(1.15, delta);
        m_viewTransform.setZoom(m_viewTransform.zoom() * factor);

        // Re-anchor: after zoom, push the canvas point back under the cursor.
        const QPointF newScreen = m_viewTransform.toScreen(canvasPt);
        const qreal   dpr       = m_viewTransform.dpr();
        m_viewTransform.pan((newScreen - cursorLocal) / dpr);
    } else {
        // Pan — prefer pixel delta (trackpad), fall back to angle delta (wheel).
        QPointF delta = e->pixelDelta();
        if (delta.isNull())
            delta = QPointF(e->angleDelta()) * (1.0 / 3.0);
        m_viewTransform.pan(-delta);
    }
    update();
    e->accept();
}

void CanvasViewport::mousePressEvent(QMouseEvent* e)
{
    if (m_spaceHeld || e->button() == Qt::MiddleButton) {
        m_panning    = true;
        m_lastPanPos = e->position();
        setCursor(Qt::ClosedHandCursor);
        e->accept();
        return;
    }
    QOpenGLWidget::mousePressEvent(e);
}

void CanvasViewport::mouseMoveEvent(QMouseEvent* e)
{
    // Emit canvas coordinate for the status bar.
    const QPointF local     = e->position() - QPointF(width() / 2.0, height() / 2.0);
    const QPointF canvasPos = m_viewTransform.toCanvas(local);
    emit canvasPosChanged(canvasPos);

    if (m_panning) {
        m_viewTransform.pan(m_lastPanPos - e->position());
        m_lastPanPos = e->position();
        update();
        e->accept();
        return;
    }
    QOpenGLWidget::mouseMoveEvent(e);
}

void CanvasViewport::mouseReleaseEvent(QMouseEvent* e)
{
    if (m_panning) {
        m_panning = false;
        setCursor(m_spaceHeld ? Qt::OpenHandCursor : Qt::ArrowCursor);
        e->accept();
        return;
    }
    QOpenGLWidget::mouseReleaseEvent(e);
}

void CanvasViewport::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Space && !e->isAutoRepeat()) {
        m_spaceHeld = true;
        setCursor(Qt::OpenHandCursor);
    }
    QOpenGLWidget::keyPressEvent(e);
}

void CanvasViewport::keyReleaseEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Space && !e->isAutoRepeat()) {
        m_spaceHeld = false;
        m_panning   = false;
        unsetCursor();
    }
    QOpenGLWidget::keyReleaseEvent(e);
}

} // namespace Zyri
