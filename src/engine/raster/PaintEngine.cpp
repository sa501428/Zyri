#include "PaintEngine.h"
#include "BrushStamp.h"
#include "../../core/DocumentController.h"
#include "../../core/RasterLayer.h"
#include "../../history/DeltaCompressor.h"
#include <cmath>

namespace Zyri {

PaintEngine::PaintEngine(DocumentController* controller)
    : m_controller(controller)
{
}

void PaintEngine::beginStroke(const QUuid& layerId, const QColor& color,
                               const BrushPreset& preset, bool eraser)
{
    if (m_active) endStroke();
    m_active   = true;
    m_eraser   = eraser;
    m_layerId  = layerId;
    m_color    = color;
    m_preset   = preset;
    m_distanceAccum = 0.0f;
    m_snapshots.clear();
    m_dirtyTiles.clear();
}

void PaintEngine::addPoint(const QPointF& canvasPos, float pressure)
{
    if (!m_active) return;
    stampAt(canvasPos, pressure);
    m_lastStampPos = canvasPos;
}

static RasterLayer* toRasterLayer(Layer* layer)
{
    if (!layer || layer->type() != LayerType::Raster) return nullptr;
    return static_cast<RasterLayer*>(layer);
}

void PaintEngine::stampAt(const QPointF& canvasPos, float pressure)
{
    auto* doc   = m_controller->document();
    auto* layer = toRasterLayer(doc->layers()->layerById(m_layerId));
    if (!layer) return;

    const float radius = (m_preset.pressureSize
                          ? m_preset.size * pressure
                          : m_preset.size) * 0.5f;
    const float opacity = m_preset.pressureOpacity
                          ? m_preset.opacity * pressure
                          : m_preset.opacity;

    const int x0 = static_cast<int>(std::floor((canvasPos.x() - radius) / kTileSize));
    const int y0 = static_cast<int>(std::floor((canvasPos.y() - radius) / kTileSize));
    const int x1 = static_cast<int>(std::floor((canvasPos.x() + radius) / kTileSize));
    const int y1 = static_cast<int>(std::floor((canvasPos.y() + radius) / kTileSize));

    for (int ty = y0; ty <= y1; ++ty) {
        for (int tx = x0; tx <= x1; ++tx) {
            TileKey key{tx, ty};

            bool alreadySnapped = false;
            for (const auto& s : m_snapshots)
                if (s.key == key) { alreadySnapped = true; break; }

            if (!alreadySnapped) {
                QImage before = layer->tileAt(tx, ty);
                QByteArray raw(reinterpret_cast<const char*>(before.constBits()),
                               before.sizeInBytes());
                m_snapshots.push_back({key, raw});
            }

            QImage tile = layer->tileAt(tx, ty);
            QPointF localPos(canvasPos.x() - tx * kTileSize,
                              canvasPos.y() - ty * kTileSize);
            BrushStamp::render(tile, localPos, radius,
                                m_preset.hardness, opacity, m_color, m_eraser);
            layer->setTile(tx, ty, tile);

            bool alreadyDirty = false;
            for (const auto& d : m_dirtyTiles)
                if (d == key) { alreadyDirty = true; break; }
            if (!alreadyDirty)
                m_dirtyTiles.push_back(key);
        }
    }
}

void PaintEngine::endStroke()
{
    if (!m_active) return;
    m_active = false;

    if (m_snapshots.empty()) return;

    auto* layer = toRasterLayer(
        m_controller->document()->layers()->layerById(m_layerId));

    // Build tile deltas for undo (Phase 3: push to UndoStack via DocumentController).
    for (const auto& snap : m_snapshots) {
        QImage after  = layer ? layer->tileAt(snap.key.tx, snap.key.ty)
                               : QImage(kTileSize, kTileSize, QImage::Format_ARGB32_Premultiplied);
        QByteArray afterRaw(reinterpret_cast<const char*>(after.constBits()), after.sizeInBytes());
        Q_UNUSED(DeltaCompressor::compress(snap.beforeRaw, afterRaw)); // wired up in Phase 3
    }
}

} // namespace Zyri
