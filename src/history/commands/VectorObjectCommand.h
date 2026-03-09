#pragma once
#include "../CommandBase.h"
namespace Zyri {
// Stub — implemented in Phase 5 (vector tools)
class VectorObjectCommand : public CommandBase {
public:
    void execute() override {}
    void undo()    override {}
    [[nodiscard]] QString description() const override { return QStringLiteral("Vector Edit"); }
};
} // namespace Zyri
