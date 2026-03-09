#include "Layer.h"

namespace Zyri {

Layer::Layer(LayerType type, const QString& name)
    : m_id(QUuid::createUuid())
    , m_type(type)
    , m_name(name.isEmpty() ? QStringLiteral("Layer") : name)
{
}

} // namespace Zyri
