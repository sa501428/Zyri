#include "VectorScene.h"
#include "VectorObject.h"

namespace Zyri {

// Destructor defined here where VectorObject is fully defined.
VectorScene::~VectorScene() = default;

void VectorScene::addObject(std::unique_ptr<VectorObject> obj)
{
    m_objects.push_back(std::move(obj));
}

void VectorScene::removeObject(int index)
{
    m_objects.erase(m_objects.begin() + index);
}

void VectorScene::paintTo(QPainter& painter) const
{
    for (const auto& obj : m_objects)
        obj->paintTo(painter);
}

} // namespace Zyri
