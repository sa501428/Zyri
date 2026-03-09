#pragma once
#include "../ToolBase.h"
namespace Zyri {
// Stub — implemented in Phase 5
class RectTool : public ToolBase {
    Q_OBJECT
public:
    explicit RectTool(QObject* p = nullptr) : ToolBase(p) {}
    [[nodiscard]] QString name() const override { return QStringLiteral("Rect"); }
};
} // namespace Zyri
