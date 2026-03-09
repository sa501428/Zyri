#pragma once
#include "BrushPreset.h"
#include "../../core/RasterLayer.h"
#include <QColor>
#include <QPointF>
#include <QUuid>
#include <vector>

namespace Zyri {

class DocumentController;

class PaintEngine {
public:
    explicit PaintEngine(DocumentController* controller);

    void beginStroke(const QUuid& layerId, const QColor& color,
                     const BrushPreset& preset, bool eraser = false);
    void addPoint(const QPointF& canvasPos, float pressure = 1.0f);
    void endStroke();

    [[nodiscard]] bool isActive() const { return m_active; }

private:
    void stampAt(const QPointF& canvasPos, float pressure);

    DocumentController* m_controller;

    bool        m_active    = false;
    bool        m_eraser    = false;
    QUuid       m_layerId;
    QColor      m_color;
    BrushPreset m_preset;
    QPointF     m_lastStampPos;
    float       m_distanceAccum = 0.0f;

    struct TileSnapshot {
        TileKey    key;
        QByteArray beforeRaw;
    };
    std::vector<TileSnapshot> m_snapshots;
    std::vector<TileKey>      m_dirtyTiles;
};

} // namespace Zyri
