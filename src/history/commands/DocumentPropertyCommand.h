#pragma once
#include "../CommandBase.h"
namespace Zyri {
// Stub — canvas resize, DPI change, etc.
class DocumentPropertyCommand : public CommandBase {
public:
    void execute() override {}
    void undo()    override {}
    [[nodiscard]] QString description() const override { return QStringLiteral("Document Change"); }
};
} // namespace Zyri
