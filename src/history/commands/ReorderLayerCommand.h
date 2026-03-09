#pragma once
#include "../CommandBase.h"
namespace Zyri {
class DocumentController;
class ReorderLayerCommand : public CommandBase {
public:
    ReorderLayerCommand(DocumentController* ctrl, int from, int to);
    void execute() override;
    void undo()    override;
    [[nodiscard]] QString description() const override { return QStringLiteral("Reorder Layer"); }
private:
    DocumentController* m_ctrl;
    int m_from, m_to;
};
} // namespace Zyri
