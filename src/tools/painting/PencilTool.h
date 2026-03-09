#pragma once
#include "BrushTool.h"
namespace Zyri {
// Pencil = brush with hardness=1, pressure-independent size.
class PencilTool : public BrushTool {
    Q_OBJECT
public:
    explicit PencilTool(QObject* parent = nullptr) : BrushTool(parent) {
        preset().hardness = 1.0f;
        preset().pressureSize = false;
        preset().pressureOpacity = false;
    }
    [[nodiscard]] QString name() const override { return QStringLiteral("Pencil"); }
};
} // namespace Zyri
