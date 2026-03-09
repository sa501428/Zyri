#include "ReorderLayerCommand.h"
#include "../../core/DocumentController.h"
namespace Zyri {
ReorderLayerCommand::ReorderLayerCommand(DocumentController* ctrl, int from, int to)
    : m_ctrl(ctrl), m_from(from), m_to(to) {}
void ReorderLayerCommand::execute() { m_ctrl->document()->layers()->moveLayer(m_from, m_to); }
void ReorderLayerCommand::undo()    { m_ctrl->document()->layers()->moveLayer(m_to, m_from); }
} // namespace Zyri
