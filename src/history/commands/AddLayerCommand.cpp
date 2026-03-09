#include "AddLayerCommand.h"
#include "../../core/DocumentController.h"

namespace Zyri {

AddLayerCommand::AddLayerCommand(DocumentController* ctrl,
                                  std::unique_ptr<Layer> layer,
                                  int index)
    : m_ctrl(ctrl), m_layer(std::move(layer)), m_index(index)
{}

void AddLayerCommand::execute()
{
    // Move the layer back into the stack (after undo it was removed).
    auto* stack = m_ctrl->document()->layers();
    stack->insertLayer(m_index, std::move(m_layer));
}

void AddLayerCommand::undo()
{
    auto* stack = m_ctrl->document()->layers();
    m_layer = stack->removeLayer(m_index);
}

} // namespace Zyri
