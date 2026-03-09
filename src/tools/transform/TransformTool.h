#pragma once
#include "../ToolBase.h"
namespace Zyri {
// Stub — implemented in Phase 5
class TransformTool : public ToolBase {
    Q_OBJECT
public:
    explicit TransformTool(QObject* p = nullptr) : ToolBase(p) {}
    [[nodiscard]] QString name() const override { return QStringLiteral("Transform"); }
};
} // namespace Zyri
