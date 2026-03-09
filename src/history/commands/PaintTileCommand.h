#pragma once

#include "../CommandBase.h"
#include "../../core/RasterLayer.h"
#include <QByteArray>
#include <QUuid>
#include <vector>

namespace Zyri {

class DocumentController;

// Stores the before/after state of one or more tiles modified during a
// single paint stroke, using delta compression for memory efficiency.
class PaintTileCommand : public CommandBase
{
public:
    struct TileDelta {
        TileKey    key;
        QByteArray beforeRaw;  // uncompressed before-snapshot (kTileSize*kTileSize*4 bytes)
        QByteArray delta;      // compressed XOR delta (before → after)
    };

    explicit PaintTileCommand(DocumentController* controller,
                               const QUuid& layerId,
                               std::vector<TileDelta> deltas);

    void execute() override;
    void undo()    override;
    [[nodiscard]] QString description() const override { return QStringLiteral("Paint"); }

private:
    void applyDeltas(bool forward);

    DocumentController*    m_controller;
    QUuid                  m_layerId;
    std::vector<TileDelta> m_deltas;
};

} // namespace Zyri
