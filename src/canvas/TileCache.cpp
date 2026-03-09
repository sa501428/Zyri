#include "TileCache.h"
#include <QOpenGLContext>

namespace Zyri {

TileCache::~TileCache()
{
    clear();
}

void TileCache::initialize()
{
    initializeOpenGLFunctions();
}

GLuint TileCache::uploadTile(const TileKey& key, const QImage& tile)
{
    GLuint tex = m_textures.value(key, 0);
    if (tex == 0) {
        glGenTextures(1, &tex);
        m_textures[key] = tex;
    }

    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Qt stores ARGB32_Premultiplied as BGRA on little-endian; use GL_BGRA.
    QImage img = tile.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                 img.width(), img.height(), 0,
                 GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
                 img.constBits());

    m_dirty.remove(key);
    return tex;
}

void TileCache::markDirty(const TileKey& key)
{
    m_dirty.insert(key);
}

GLuint TileCache::textureFor(const TileKey& key) const
{
    return m_textures.value(key, 0);
}

bool TileCache::isDirty(const TileKey& key) const
{
    return m_dirty.contains(key);
}

void TileCache::evict(const TileKey& key)
{
    auto it = m_textures.find(key);
    if (it != m_textures.end()) {
        glDeleteTextures(1, &it.value());
        m_textures.erase(it);
    }
    m_dirty.remove(key);
}

void TileCache::clear()
{
    for (auto tex : m_textures)
        glDeleteTextures(1, &tex);
    m_textures.clear();
    m_dirty.clear();
}

} // namespace Zyri
