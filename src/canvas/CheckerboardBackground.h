#pragma once

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QSize>

namespace Zyri {

class ViewTransform;

// Draws the grey/white transparency checkerboard behind the canvas.
// Only the canvas rect receives the checker; the surround stays the dark-grey
// clear colour.  The pattern is canvas-relative: squares are always
// `checkerCanvasPixels` wide in canvas space, so zooming in makes them bigger.
class CheckerboardBackground : protected QOpenGLFunctions
{
public:
    void initialize(); // call once after GL context is current
    void destroy();    // call in ~CanvasViewport while context is current

    // vpSize is the logical (widget) pixel size.
    void draw(const QSize& canvasSize,
              const ViewTransform& vt,
              const QSize& vpSize,
              float checkerCanvasPixels = 16.0f);

private:
    QOpenGLShaderProgram     m_shader;
    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer            m_vbo{QOpenGLBuffer::VertexBuffer};
    int m_locCheckerSize = -1;
};

} // namespace Zyri
