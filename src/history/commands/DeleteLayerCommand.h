#pragma once
#include "../CommandBase.h"
#include "../../core/Layer.h"
#include <memory>

namespace Zyri {
class DocumentController;
class DeleteLayerCommand : public CommandBase {
public:
    DeleteLayerCommand(DocumentController* ctrl, int index);
    void execute() override;
    void undo()    override;
    [[nodiscard]] QString description() const override { return QStringLiteral("Delete Layer"); }
private:
    DocumentController*    m_ctrl;
    int                    m_index;
    std::unique_ptr<Layer> m_layer; // holds removed layer during redo availability
};
} // namespace Zyri
