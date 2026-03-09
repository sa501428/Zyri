#pragma once

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QSize>

namespace Zyri {

class LayerStack;
class TileCache;
class ViewTransform;

// Uploads raster-layer tiles to the GPU and composites them bottom-to-top.
//
// Phase 2: Normal blend mode only — uses hardware src-over blending
// (glBlendFunc GL_ONE / GL_ONE_MINUS_SRC_ALPHA on ARGB32-premultiplied data).
// Phase 3+ will add an FBO accumulator for Multiply / Screen / Overlay.
class GLCompositor : protected QOpenGLFunctions
{
public:
    void initialize(); // call once after GL context is current
    void destroy();    // call in ~CanvasViewport while context is current

    // Draw all visible raster layers.  vpSize is the logical (widget) size.
    void composite(const LayerStack* layers,
                   const ViewTransform& vt,
                   const QSize& vpSize,
                   TileCache& cache);

private:
    // Draw the unit quad scaled/translated to the NDC rect [ndcL,ndcB]-[ndcR,ndcT].
    void drawTileQuad(GLuint tex,
                      float ndcL, float ndcB,
                      float ndcR, float ndcT,
                      float opacity);

    QOpenGLShaderProgram     m_shader;
    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer            m_vbo{QOpenGLBuffer::VertexBuffer};
    int m_locMVP     = -1;
    int m_locTex     = -1;
    int m_locOpacity = -1;
};

} // namespace Zyri
