#include "ToolRegistry.h"

namespace Zyri {

ToolRegistry::ToolRegistry(QObject* parent) : QObject(parent) {}

void ToolRegistry::registerTool(ToolId id, std::unique_ptr<ToolBase> tool)
{
    m_tools[static_cast<int>(id)] = std::move(tool);
}

ToolBase* ToolRegistry::tool(ToolId id) const
{
    auto it = m_tools.find(static_cast<int>(id));
    return it != m_tools.end() ? it->second.get() : nullptr;
}

void ToolRegistry::setActiveTool(ToolId id, DocumentController* controller)
{
    if (m_activeTool)
        m_activeTool->deactivate();

    m_activeId   = id;
    m_activeTool = tool(id);

    if (m_activeTool)
        m_activeTool->activate(controller);

    emit activeToolChanged(id);
}

} // namespace Zyri
