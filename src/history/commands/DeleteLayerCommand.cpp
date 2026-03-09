#include "DeleteLayerCommand.h"
#include "../../core/DocumentController.h"

namespace Zyri {

DeleteLayerCommand::DeleteLayerCommand(DocumentController* ctrl, int index)
    : m_ctrl(ctrl), m_index(index)
{}

void DeleteLayerCommand::execute()
{
    m_layer = m_ctrl->document()->layers()->removeLayer(m_index);
}

void DeleteLayerCommand::undo()
{
    m_ctrl->document()->layers()->insertLayer(m_index, std::move(m_layer));
}

} // namespace Zyri
