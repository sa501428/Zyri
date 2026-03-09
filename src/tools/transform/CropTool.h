#pragma once
#include "../ToolBase.h"
namespace Zyri {
// Stub — implemented in Phase 5
class CropTool : public ToolBase {
    Q_OBJECT
public:
    explicit CropTool(QObject* p = nullptr) : ToolBase(p) {}
    [[nodiscard]] QString name() const override { return QStringLiteral("Crop"); }
};
} // namespace Zyri
