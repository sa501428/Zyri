#pragma once
#include "../ToolBase.h"
namespace Zyri {
// Stub — implemented in Phase 5
class LassoTool : public ToolBase {
    Q_OBJECT
public:
    explicit LassoTool(QObject* p = nullptr) : ToolBase(p) {}
    [[nodiscard]] QString name() const override { return QStringLiteral("Lasso"); }
};
} // namespace Zyri
