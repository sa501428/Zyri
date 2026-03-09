#pragma once
#include "../ToolBase.h"
namespace Zyri {
// Stub — implemented in Phase 5
class MoveTool : public ToolBase {
    Q_OBJECT
public:
    explicit MoveTool(QObject* p = nullptr) : ToolBase(p) {}
    [[nodiscard]] QString name() const override { return QStringLiteral("Move"); }
};
} // namespace Zyri
