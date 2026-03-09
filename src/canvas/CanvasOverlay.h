#pragma once

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QSize>

namespace Zyri {

class ViewTransform;

// Renders canvas-space overlays on top of the composited layer stack.
// Phase 2: canvas border outline only.
// Phase 5+ will add: selection marching ants, guides, tool preview cursors.
class CanvasOverlay : protected QOpenGLFunctions
{
public:
    void initialize(); // call once after GL context is current
    void destroy();    // call in ~CanvasViewport while context is current

    // Draw all active overlays.  vpSize is the logical (widget) size.
    void draw(const QSize& canvasSize,
              const ViewTransform& vt,
              const QSize& vpSize);

private:
    QOpenGLShaderProgram     m_shader;
    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer            m_vbo{QOpenGLBuffer::VertexBuffer};
    int m_locMVP   = -1;
    int m_locColor = -1;
};

} // namespace Zyri
