#include "LayerPropertyCommand.h"
#include "../../core/DocumentController.h"
#include "../../core/Layer.h"

namespace Zyri {

LayerPropertyCommand::LayerPropertyCommand(DocumentController* ctrl, const QUuid& layerId,
                                            Property prop, Value oldVal, Value newVal)
    : m_ctrl(ctrl), m_layerId(layerId), m_prop(prop), m_old(oldVal), m_new(newVal)
{}

void LayerPropertyCommand::execute() { apply(m_new); }
void LayerPropertyCommand::undo()    { apply(m_old); }

void LayerPropertyCommand::apply(const Value& v)
{
    auto* layer = m_ctrl->document()->layers()->layerById(m_layerId);
    if (!layer) return;
    switch (m_prop) {
    case Property::Opacity:   layer->setOpacity(std::get<float>(v)); break;
    case Property::BlendMode: layer->setBlendMode(std::get<Zyri::BlendMode>(v)); break;
    case Property::Visible:   layer->setVisible(std::get<bool>(v)); break;
    case Property::Locked:    layer->setLocked(std::get<bool>(v)); break;
    case Property::Name:      layer->setName(std::get<QString>(v)); break;
    }
}

QString LayerPropertyCommand::description() const
{
    switch (m_prop) {
    case Property::Opacity:   return QStringLiteral("Change Opacity");
    case Property::BlendMode: return QStringLiteral("Change Blend Mode");
    case Property::Visible:   return QStringLiteral("Toggle Visibility");
    case Property::Locked:    return QStringLiteral("Toggle Lock");
    case Property::Name:      return QStringLiteral("Rename Layer");
    }
    return {};
}

} // namespace Zyri
