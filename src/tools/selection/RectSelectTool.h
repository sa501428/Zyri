#pragma once
#include "../ToolBase.h"
namespace Zyri {
// Stub — implemented in Phase 5
class RectSelectTool : public ToolBase {
    Q_OBJECT
public:
    explicit RectSelectTool(QObject* p = nullptr) : ToolBase(p) {}
    [[nodiscard]] QString name() const override { return QStringLiteral("RectSelect"); }
};
} // namespace Zyri
