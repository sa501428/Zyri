#pragma once
#include "../ToolBase.h"
namespace Zyri {
// Stub — implemented in Phase 5
class EllipseTool : public ToolBase {
    Q_OBJECT
public:
    explicit EllipseTool(QObject* p = nullptr) : ToolBase(p) {}
    [[nodiscard]] QString name() const override { return QStringLiteral("Ellipse"); }
};
} // namespace Zyri
