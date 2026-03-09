#pragma once
#include "../ToolBase.h"
namespace Zyri {
// Stub — implemented in Phase 5
class FillBucketTool : public ToolBase {
    Q_OBJECT
public:
    explicit FillBucketTool(QObject* p = nullptr) : ToolBase(p) {}
    [[nodiscard]] QString name() const override { return QStringLiteral("FillBucket"); }
};
} // namespace Zyri
