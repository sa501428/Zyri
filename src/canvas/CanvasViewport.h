#pragma once
#include "ViewTransform.h"
#include "TileCache.h"
#include <QOpenGLWidget>
#include <QOpenGLShaderProgram>
#include <memory>

namespace Zyri {

class Document;
class DocumentController;

// The central OpenGL canvas widget.
//
// Rendering pipeline:
//   1. Checkerboard background (procedural shader)
//   2. Layer stack, bottom-to-top (tile textures composited via blend-mode shader)
//   3. Canvas overlay (selection, guides, tool preview, cursor)
//
// Input:
//   - wheelEvent: scroll = pan, ctrl+scroll = zoom
//   - QNativeGestureEvent (macOS): pinch-to-zoom
//   - QTabletEvent: pressure-sensitive input for tools
//   - key Spacebar: temporary pan mode
//
// IMPORTANT: QTabletEvent is intercepted in event() not tabletEvent() due to
// inconsistent delivery of tabletEvent() on macOS + Qt6.
class CanvasViewport : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit CanvasViewport(QWidget* parent = nullptr);
    ~CanvasViewport() override;

    void setDocument(Document* doc);
    void setController(DocumentController* ctrl);

    [[nodiscard]] ViewTransform& viewTransform() { return m_viewTransform; }

    // Invalidate (re-upload + repaint) specific tiles.
    void invalidateTile(int tx, int ty);

signals:
    void canvasPosChanged(const QPointF& canvasPos);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    bool event(QEvent* e) override; // intercepts tablet events here
    void wheelEvent(QWheelEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;

private:
    void initShaders();
    void drawCheckerboard();
    void drawLayers();
    void drawOverlay();

    Document*           m_document   = nullptr;
    DocumentController* m_controller = nullptr;
    ViewTransform       m_viewTransform;
    TileCache           m_tileCache;

    QOpenGLShaderProgram m_checkerShader;
    QOpenGLShaderProgram m_compositeShader;
    QOpenGLShaderProgram m_overlayShader;

    bool  m_spaceHeld   = false; // spacebar pan mode
    bool  m_panning     = false;
    QPointF m_lastPanPos;
};

} // namespace Zyri
