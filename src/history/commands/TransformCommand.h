#pragma once
#include "../CommandBase.h"
namespace Zyri {
// Stub — implemented in Phase 5 (transform tool)
class TransformCommand : public CommandBase {
public:
    void execute() override {}
    void undo()    override {}
    [[nodiscard]] QString description() const override { return QStringLiteral("Transform"); }
};
} // namespace Zyri
