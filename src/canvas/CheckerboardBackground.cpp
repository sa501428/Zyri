#include "CheckerboardBackground.h"
#include "ViewTransform.h"
#include <QPointF>

namespace Zyri {

void CheckerboardBackground::initialize()
{
    initializeOpenGLFunctions();

    m_shader.addShaderFromSourceFile(QOpenGLShader::Vertex,
                                     QStringLiteral(":/shaders/shaders/checkerboard.vert"));
    m_shader.addShaderFromSourceFile(QOpenGLShader::Fragment,
                                     QStringLiteral(":/shaders/shaders/checkerboard.frag"));
    if (!m_shader.link())
        qWarning("CheckerboardBackground: shader link failed: %s",
                 m_shader.log().toUtf8().constData());

    m_locCheckerSize = m_shader.uniformLocation("uCheckerSize");

    // 4 vertices × (ndcX, ndcY, canvasX, canvasY) — updated every frame.
    m_vao.create();
    m_vao.bind();

    m_vbo.create();
    m_vbo.bind();
    m_vbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    m_vbo.allocate(4 * 4 * sizeof(float));

    glEnableVertexAttribArray(0); // aPos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);

    glEnableVertexAttribArray(1); // aCanvasPos
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<const void*>(2 * sizeof(float)));

    m_vao.release();
    m_vbo.release();
}

void CheckerboardBackground::destroy()
{
    m_vbo.destroy();
    m_vao.destroy();
}

void CheckerboardBackground::draw(const QSize&       canvasSize,
                                   const ViewTransform& vt,
                                   const QSize&       vpSize,
                                   float              checkerCanvasPixels)
{
    const double w  = vpSize.width();
    const double h  = vpSize.height();
    const float  cw = float(canvasSize.width());
    const float  ch = float(canvasSize.height());

    // Convert a canvas point to NDC.
    // ViewTransform::toScreen() returns coordinates relative to the viewport
    // centre in logical pixels (y down).  NDC conversion flips y.
    auto toNDC = [&](double cx, double cy, float& nx, float& ny) {
        const QPointF s = vt.toScreen({cx, cy});
        nx = float(s.x() * 2.0 / w);
        ny = float(-s.y() * 2.0 / h);
    };

    // Four canvas corners, TRIANGLE_STRIP order: BL, BR, TL, TR.
    float x[4], y[4];
    toNDC(0,  ch, x[0], y[0]); // bottom-left  (canvas BL)
    toNDC(cw, ch, x[1], y[1]); // bottom-right (canvas BR)
    toNDC(0,  0,  x[2], y[2]); // top-left     (canvas TL)
    toNDC(cw, 0,  x[3], y[3]); // top-right    (canvas TR)

    // canvas Y is down, matching the canvas-coord system in the frag shader.
    const float verts[] = {
        x[0], y[0],  0.0f, ch,   // BL
        x[1], y[1],  cw,   ch,   // BR
        x[2], y[2],  0.0f, 0.0f, // TL
        x[3], y[3],  cw,   0.0f, // TR
    };

    m_vbo.bind();
    m_vbo.write(0, verts, sizeof(verts));
    m_vbo.release();

    m_shader.bind();
    m_shader.setUniformValue(m_locCheckerSize, checkerCanvasPixels);

    m_vao.bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    m_vao.release();

    m_shader.release();
}

} // namespace Zyri
