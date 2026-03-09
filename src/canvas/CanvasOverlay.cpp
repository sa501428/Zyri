#include "CanvasOverlay.h"
#include "ViewTransform.h"
#include <QMatrix4x4>
#include <QVector4D>
#include <QPointF>

namespace Zyri {

void CanvasOverlay::initialize()
{
    initializeOpenGLFunctions();

    m_shader.addShaderFromSourceFile(QOpenGLShader::Vertex,
                                     QStringLiteral(":/shaders/shaders/overlay.vert"));
    m_shader.addShaderFromSourceFile(QOpenGLShader::Fragment,
                                     QStringLiteral(":/shaders/shaders/overlay.frag"));
    if (!m_shader.link())
        qWarning("CanvasOverlay: shader link failed: %s",
                 m_shader.log().toUtf8().constData());

    m_locMVP   = m_shader.uniformLocation("uMVP");
    m_locColor = m_shader.uniformLocation("uColor");

    // 4 vertices of the canvas border (LINE_LOOP), each (ndcX, ndcY).
    m_vao.create();
    m_vao.bind();

    m_vbo.create();
    m_vbo.bind();
    m_vbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    m_vbo.allocate(4 * 2 * sizeof(float));

    glEnableVertexAttribArray(0); // aPos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

    m_vao.release();
    m_vbo.release();
}

void CanvasOverlay::destroy()
{
    m_vbo.destroy();
    m_vao.destroy();
}

void CanvasOverlay::draw(const QSize&       canvasSize,
                          const ViewTransform& vt,
                          const QSize&       vpSize)
{
    const double w  = vpSize.width();
    const double h  = vpSize.height();
    const float  cw = float(canvasSize.width());
    const float  ch = float(canvasSize.height());

    auto toNDC = [&](double cx, double cy, float& nx, float& ny) {
        const QPointF s = vt.toScreen({cx, cy});
        nx = float(s.x() * 2.0 / w);
        ny = float(-s.y() * 2.0 / h);
    };

    // Canvas border: TL → TR → BR → BL (LINE_LOOP closes it).
    float x[4], y[4];
    toNDC(0,  0,  x[0], y[0]); // TL
    toNDC(cw, 0,  x[1], y[1]); // TR
    toNDC(cw, ch, x[2], y[2]); // BR
    toNDC(0,  ch, x[3], y[3]); // BL

    const float verts[] = { x[0],y[0], x[1],y[1], x[2],y[2], x[3],y[3] };

    m_vbo.bind();
    m_vbo.write(0, verts, sizeof(verts));
    m_vbo.release();

    // Identity MVP — positions are already in NDC.
    QMatrix4x4 identity;
    identity.setToIdentity();

    m_shader.bind();
    m_shader.setUniformValue(m_locMVP, identity);
    // Subtle dark border — visible against both checker and dark surround.
    m_shader.setUniformValue(m_locColor, QVector4D(0.0f, 0.0f, 0.0f, 0.6f));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_vao.bind();
    glLineWidth(1.0f);
    glDrawArrays(GL_LINE_LOOP, 0, 4);
    m_vao.release();

    m_shader.release();
    glDisable(GL_BLEND);
}

} // namespace Zyri
