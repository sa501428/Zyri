#include "VectorLayer.h"
#include "../engine/vector/VectorScene.h"
#include "../engine/vector/VectorObject.h"  // needed for complete type in vector destructor

namespace Zyri {

VectorLayer::VectorLayer(const QString& name)
    : Layer(LayerType::Vector, name.isEmpty() ? QStringLiteral("Vector Layer") : name)
    , m_scene(std::make_unique<VectorScene>())
{
}

VectorLayer::~VectorLayer() = default;

} // namespace Zyri
