#pragma once
#include "../ToolBase.h"
namespace Zyri {
// Stub — implemented in Phase 5
class PolygonTool : public ToolBase {
    Q_OBJECT
public:
    explicit PolygonTool(QObject* p = nullptr) : ToolBase(p) {}
    [[nodiscard]] QString name() const override { return QStringLiteral("Polygon"); }
};
} // namespace Zyri
