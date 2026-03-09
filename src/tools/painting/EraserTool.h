#pragma once
#include "BrushTool.h"
namespace Zyri {
// Eraser = brush with CompositionMode_DestinationOut.
class EraserTool : public BrushTool {
    Q_OBJECT
public:
    explicit EraserTool(QObject* parent = nullptr) : BrushTool(parent) {}
    [[nodiscard]] QString name() const override { return QStringLiteral("Eraser"); }
    void pointerDown(const QPointF& pos, qreal pressure) override;
};
} // namespace Zyri
