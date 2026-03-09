#pragma once
#include "../ToolBase.h"
namespace Zyri {
// Stub — implemented in Phase 5
class EyedropperTool : public ToolBase {
    Q_OBJECT
public:
    explicit EyedropperTool(QObject* p = nullptr) : ToolBase(p) {}
    [[nodiscard]] QString name() const override { return QStringLiteral("Eyedropper"); }
};
} // namespace Zyri
