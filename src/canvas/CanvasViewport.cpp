#include "CanvasViewport.h"
#include "../core/Document.h"
#include "../core/DocumentController.h"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QNativeGestureEvent>
#include <QWheelEvent>
#include <QOpenGLContext>

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
    doneCurrent();
}

void CanvasViewport::setDocument(Document* doc)
{
    m_document = doc;
    update();
}

void CanvasViewport::setController(DocumentController* ctrl)
{
    m_controller = ctrl;
}

void CanvasViewport::invalidateTile(int tx, int ty)
{
    m_tileCache.markDirty({tx, ty});
    // Compute the screen rect for this tile and schedule a partial repaint.
    const QPointF tl = m_viewTransform.toScreen(QPointF(tx * kTileSize, ty * kTileSize));
    const QPointF br = m_viewTransform.toScreen(QPointF((tx + 1) * kTileSize, (ty + 1) * kTileSize));
    update(QRect(tl.toPoint(), br.toPoint()).adjusted(-1, -1, 1, 1));
}

void CanvasViewport::initializeGL()
{
    m_tileCache.initialize();
    m_viewTransform.setDevicePixelRatio(devicePixelRatio());
    initShaders();
}

void CanvasViewport::resizeGL(int /*w*/, int /*h*/)
{
    m_viewTransform.setDevicePixelRatio(devicePixelRatio());
}

void CanvasViewport::paintGL()
{
    auto* f = QOpenGLContext::currentContext()->functions();
    f->glClearColor(0.18f, 0.18f, 0.18f, 1.0f); // dark grey surround
    f->glClear(GL_COLOR_BUFFER_BIT);

    if (!m_document) return;

    drawCheckerboard();
    drawLayers();
    drawOverlay();
}

void CanvasViewport::initShaders()
{
    // Shaders are loaded from Qt resources in Phase 2.
    // Stubs intentionally left empty for Phase 0 build.
}

void CanvasViewport::drawCheckerboard()
{
    // Implemented in Phase 2 using checkerboard.vert/.frag shaders.
}

void CanvasViewport::drawLayers()
{
    // Implemented in Phase 2/3 using composite.vert/.frag shaders.
}

void CanvasViewport::drawOverlay()
{
    // Implemented in Phase 2 using overlay.vert/.frag shaders.
}

bool CanvasViewport::event(QEvent* e)
{
    // IMPORTANT: Intercept QTabletEvent here, not in tabletEvent(),
    // because tabletEvent() is not reliably delivered on macOS + Qt6.
    if (e->type() == QEvent::TabletPress ||
        e->type() == QEvent::TabletMove  ||
        e->type() == QEvent::TabletRelease)
    {
        // Tablet handling implemented in Phase 3 when tools are wired.
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
        // Zoom
        const qreal delta = e->angleDelta().y() / 120.0;
        const qreal factor = std::pow(1.15, delta);
        m_viewTransform.setZoom(m_viewTransform.zoom() * factor);
    } else {
        // Pan
        m_viewTransform.pan(-QPointF(e->pixelDelta().x(), e->pixelDelta().y()));
    }
    update();
    e->accept();
}

void CanvasViewport::mousePressEvent(QMouseEvent* e)
{
    if (m_spaceHeld || e->button() == Qt::MiddleButton) {
        m_panning = true;
        m_lastPanPos = e->position();
        e->accept();
        return;
    }
    QOpenGLWidget::mousePressEvent(e);
}

void CanvasViewport::mouseMoveEvent(QMouseEvent* e)
{
    const QPointF canvasPos = m_viewTransform.toCanvas(
        e->position() - QPointF(width() / 2.0, height() / 2.0));
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
