#pragma once
#include "../core/RasterLayer.h"
#include <QHash>
#include <QOpenGLFunctions>
#include <QSet>

namespace Zyri {

// Manages OpenGL texture handles for raster tiles.
// Tracks which tiles are dirty and need re-upload to the GPU.
class TileCache : protected QOpenGLFunctions {
public:
    TileCache() = default;
    ~TileCache();

    void initialize(); // call after GL context is current

    // Upload (or re-upload) a tile to the GPU. Returns the GL texture name.
    GLuint uploadTile(const TileKey& key, const QImage& tile);

    // Mark a tile as dirty — will be re-uploaded on next access.
    void markDirty(const TileKey& key);

    // Returns the GL texture handle, or 0 if not yet uploaded.
    [[nodiscard]] GLuint textureFor(const TileKey& key) const;

    [[nodiscard]] bool isDirty(const TileKey& key) const;

    // Remove a tile's texture from the cache (e.g. layer deleted).
    void evict(const TileKey& key);

    void clear();

private:
    QHash<TileKey, GLuint> m_textures;
    QSet<TileKey>          m_dirty;
};

} // namespace Zyri
