#pragma once
#include "../ToolBase.h"
namespace Zyri {
// Stub — implemented in Phase 5
class EllipseSelectTool : public ToolBase {
    Q_OBJECT
public:
    explicit EllipseSelectTool(QObject* p = nullptr) : ToolBase(p) {}
    [[nodiscard]] QString name() const override { return QStringLiteral("EllipseSelect"); }
};
} // namespace Zyri
