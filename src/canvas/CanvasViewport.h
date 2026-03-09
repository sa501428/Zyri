#pragma once

#include "ViewTransform.h"
#include "TileCache.h"
#include "CheckerboardBackground.h"
#include "GLCompositor.h"
#include "CanvasOverlay.h"
#include <QOpenGLWidget>
#include <memory>

namespace Zyri {

class Document;
class DocumentController;

// The central OpenGL canvas widget.
//
// Rendering pipeline (paintGL):
//   1. glClear — dark-grey surround
//   2. CheckerboardBackground — canvas rect, canvas-relative pattern
//   3. GLCompositor — raster layers bottom-to-top, premultiplied src-over
//   4. CanvasOverlay — canvas border outline; Phase 5+ adds selection/guides
//
// Input handling:
//   - wheelEvent: scroll = pan, Ctrl+scroll = zoom
//   - QNativeGestureEvent (macOS): pinch-to-zoom
//   - QTabletEvent: pressure input for tools (Phase 3)
//   - Spacebar: temporary pan mode
//
// IMPORTANT: QTabletEvent is intercepted in event(), NOT tabletEvent().
// tabletEvent() is not reliably delivered on macOS + Qt 6.
class CanvasViewport : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit CanvasViewport(QWidget* parent = nullptr);
    ~CanvasViewport() override;

    void setDocument(Document* doc);
    void setController(DocumentController* ctrl);

    [[nodiscard]] ViewTransform& viewTransform() { return m_viewTransform; }

    // Mark a tile dirty and schedule a partial repaint of its screen rect.
    void invalidateTile(int tx, int ty);

signals:
    void canvasPosChanged(const QPointF& canvasPos);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    bool event(QEvent* e) override; // tablet + native gesture interception
    void wheelEvent(QWheelEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;

private:
    Document*           m_document   = nullptr;
    DocumentController* m_controller = nullptr;
    ViewTransform       m_viewTransform;
    TileCache           m_tileCache;

    CheckerboardBackground m_background;
    GLCompositor           m_compositor;
    CanvasOverlay          m_overlay;

    bool    m_spaceHeld  = false;
    bool    m_panning    = false;
    QPointF m_lastPanPos;
};

} // namespace Zyri
