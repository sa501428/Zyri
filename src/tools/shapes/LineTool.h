#pragma once
#include "../ToolBase.h"
namespace Zyri {
// Stub — implemented in Phase 5
class LineTool : public ToolBase {
    Q_OBJECT
public:
    explicit LineTool(QObject* p = nullptr) : ToolBase(p) {}
    [[nodiscard]] QString name() const override { return QStringLiteral("Line"); }
};
} // namespace Zyri
