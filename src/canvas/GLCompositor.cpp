#include "GLCompositor.h"
#include "TileCache.h"
#include "ViewTransform.h"
#include "../core/LayerStack.h"
#include "../core/Layer.h"
#include "../core/RasterLayer.h"
#include <QMatrix4x4>
#include <QPointF>

namespace Zyri {

// Unit quad in NDC: positions in [-1,1]² + texture coordinates.
// Texture coord v=0 corresponds to the first scanline of the QImage (top of
// image), which OpenGL places at the texture's bottom — so no explicit flip is
// needed: the canvas top-left naturally appears at screen top-left.
static constexpr float kQuadVerts[] = {
    // ndcX   ndcY   u     v
    -1.0f, -1.0f,  0.0f, 1.0f,   // NDC bottom-left  → image bottom-left
     1.0f, -1.0f,  1.0f, 1.0f,   // NDC bottom-right → image bottom-right
    -1.0f,  1.0f,  0.0f, 0.0f,   // NDC top-left     → image top-left
     1.0f,  1.0f,  1.0f, 0.0f,   // NDC top-right    → image top-right
};

void GLCompositor::initialize()
{
    initializeOpenGLFunctions();

    m_shader.addShaderFromSourceFile(QOpenGLShader::Vertex,
                                     QStringLiteral(":/shaders/shaders/composite.vert"));
    m_shader.addShaderFromSourceFile(QOpenGLShader::Fragment,
                                     QStringLiteral(":/shaders/shaders/composite.frag"));
    if (!m_shader.link())
        qWarning("GLCompositor: shader link failed: %s",
                 m_shader.log().toUtf8().constData());

    m_locMVP     = m_shader.uniformLocation("uMVP");
    m_locTex     = m_shader.uniformLocation("uLayerTex");
    m_locOpacity = m_shader.uniformLocation("uOpacity");

    m_vao.create();
    m_vao.bind();

    m_vbo.create();
    m_vbo.bind();
    m_vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    m_vbo.allocate(kQuadVerts, sizeof(kQuadVerts));

    glEnableVertexAttribArray(0); // aPos (vec2)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);

    glEnableVertexAttribArray(1); // aTexCoord (vec2)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<const void*>(2 * sizeof(float)));

    m_vao.release();
    m_vbo.release();
}

void GLCompositor::destroy()
{
    m_vbo.destroy();
    m_vao.destroy();
}

void GLCompositor::composite(const LayerStack*    layers,
                              const ViewTransform& vt,
                              const QSize&         vpSize,
                              TileCache&           cache)
{
    if (!layers || layers->count() == 0) return;

    // Premultiplied src-over: out = src + dst*(1-src.a)
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    m_shader.bind();
    m_shader.setUniformValue(m_locTex, 0);

    m_vao.bind();
    glActiveTexture(GL_TEXTURE0);

    const double w = vpSize.width();
    const double h = vpSize.height();

    // Helper: canvas point → NDC (logical viewport, y-flipped).
    auto toNDC = [&](double cx, double cy, float& nx, float& ny) {
        const QPointF s = vt.toScreen({cx, cy});
        nx = float(s.x() * 2.0 / w);
        ny = float(-s.y() * 2.0 / h); // flip: canvas y-down → NDC y-up
    };

    // Draw bottom-to-top so layers stack correctly under normal src-over.
    for (int i = 0; i < layers->count(); ++i) {
        const Layer* layer = layers->layerAt(i);
        if (!layer->visible() || layer->type() != LayerType::Raster) continue;

        const auto*  raster  = static_cast<const RasterLayer*>(layer);
        const float  opacity = layer->opacity();

        for (auto it = raster->tiles().cbegin(); it != raster->tiles().cend(); ++it) {
            const TileKey& key = it.key();

            // Upload (or re-upload if dirty).
            GLuint tex = cache.textureFor(key);
            if (tex == 0 || cache.isDirty(key))
                tex = cache.uploadTile(key, it.value());
            if (tex == 0) continue;

            // Canvas-space corners of this tile.
            const double cx0 = double(key.tx) * kTileSize;
            const double cy0 = double(key.ty) * kTileSize;
            const double cx1 = cx0 + kTileSize;
            const double cy1 = cy0 + kTileSize;

            // Canvas TL maps to NDC high-y (top); canvas BR maps to NDC low-y (bottom).
            float ndcX0, ndcYTop, ndcX1, ndcYBottom;
            toNDC(cx0, cy0, ndcX0, ndcYTop);
            toNDC(cx1, cy1, ndcX1, ndcYBottom);

            drawTileQuad(tex, ndcX0, ndcYBottom, ndcX1, ndcYTop, opacity);
        }
    }

    m_vao.release();
    m_shader.release();
    glDisable(GL_BLEND);
}

void GLCompositor::drawTileQuad(GLuint tex,
                                 float ndcL, float ndcB,
                                 float ndcR, float ndcT,
                                 float opacity)
{
    // Scale and translate the unit quad [-1,1]² to the NDC tile rect.
    const float cx = (ndcL + ndcR) * 0.5f;
    const float cy = (ndcB + ndcT) * 0.5f;
    const float sx = (ndcR - ndcL) * 0.5f;
    const float sy = (ndcT - ndcB) * 0.5f;

    QMatrix4x4 mvp;
    mvp.setToIdentity();
    mvp.translate(cx, cy, 0.0f);
    mvp.scale(sx, sy, 1.0f);

    m_shader.setUniformValue(m_locMVP, mvp);
    m_shader.setUniformValue(m_locOpacity, opacity);

    glBindTexture(GL_TEXTURE_2D, tex);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

} // namespace Zyri
