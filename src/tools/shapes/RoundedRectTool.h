#pragma once
#include "../ToolBase.h"
namespace Zyri {
// Stub — implemented in Phase 5
class RoundedRectTool : public ToolBase {
    Q_OBJECT
public:
    explicit RoundedRectTool(QObject* p = nullptr) : ToolBase(p) {}
    [[nodiscard]] QString name() const override { return QStringLiteral("RoundedRect"); }
};
} // namespace Zyri
