#pragma once
#include "../ToolBase.h"
namespace Zyri {
// Stub — implemented in Phase 5
class MagicWandTool : public ToolBase {
    Q_OBJECT
public:
    explicit MagicWandTool(QObject* p = nullptr) : ToolBase(p) {}
    [[nodiscard]] QString name() const override { return QStringLiteral("MagicWand"); }
};
} // namespace Zyri
