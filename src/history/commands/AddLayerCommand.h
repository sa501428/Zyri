#pragma once
#include "../CommandBase.h"
#include "../../core/Layer.h"
#include <memory>

namespace Zyri {
class DocumentController;
class AddLayerCommand : public CommandBase {
public:
    AddLayerCommand(DocumentController* ctrl, std::unique_ptr<Layer> layer, int index);
    void execute() override;
    void undo()    override;
    [[nodiscard]] QString description() const override { return QStringLiteral("Add Layer"); }
private:
    DocumentController*    m_ctrl;
    std::unique_ptr<Layer> m_layer;
    int                    m_index;
};
} // namespace Zyri
