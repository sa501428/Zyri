#include "PaintTileCommand.h"
#include "../DeltaCompressor.h"
#include "../../core/DocumentController.h"
#include "../../core/RasterLayer.h"

namespace Zyri {

PaintTileCommand::PaintTileCommand(DocumentController* controller,
                                    const QUuid& layerId,
                                    std::vector<TileDelta> deltas)
    : m_controller(controller)
    , m_layerId(layerId)
    , m_deltas(std::move(deltas))
{
}

void PaintTileCommand::execute() { applyDeltas(true); }
void PaintTileCommand::undo()    { applyDeltas(false); }

void PaintTileCommand::applyDeltas(bool forward)
{
    auto* rawLayer = m_controller->document()->layers()->layerById(m_layerId);
    if (!rawLayer || rawLayer->type() != LayerType::Raster) return;
    auto* layer = static_cast<RasterLayer*>(rawLayer);

    for (const auto& td : m_deltas) {
        if (forward) {
            QByteArray afterRaw = DeltaCompressor::apply(td.beforeRaw, td.delta);
            QImage tile(reinterpret_cast<const uchar*>(afterRaw.constData()),
                        kTileSize, kTileSize,
                        QImage::Format_ARGB32_Premultiplied);
            layer->setTile(td.key.tx, td.key.ty, tile.copy());
        } else {
            QImage tile(reinterpret_cast<const uchar*>(td.beforeRaw.constData()),
                        kTileSize, kTileSize,
                        QImage::Format_ARGB32_Premultiplied);
            layer->setTile(td.key.tx, td.key.ty, tile.copy());
        }
    }
}

} // namespace Zyri
