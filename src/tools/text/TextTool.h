#pragma once
#include "../ToolBase.h"
namespace Zyri {
// Stub — implemented in Phase 5
class TextTool : public ToolBase {
    Q_OBJECT
public:
    explicit TextTool(QObject* p = nullptr) : ToolBase(p) {}
    [[nodiscard]] QString name() const override { return QStringLiteral("Text"); }
};
} // namespace Zyri
