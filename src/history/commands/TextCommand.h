#pragma once
#include "../CommandBase.h"
namespace Zyri {
// Stub — implemented in Phase 5 (text tool)
class TextCommand : public CommandBase {
public:
    void execute() override {}
    void undo()    override {}
    [[nodiscard]] QString description() const override { return QStringLiteral("Edit Text"); }
};
} // namespace Zyri
