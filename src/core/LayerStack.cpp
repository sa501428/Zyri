#include "LayerStack.h"
#include <algorithm>

namespace Zyri {

LayerStack::LayerStack(QObject* parent) : QObject(parent) {}

void LayerStack::addLayer(std::unique_ptr<Layer> layer)
{
    m_layers.push_back(std::move(layer));
    emit stackChanged();
}

void LayerStack::insertLayer(int index, std::unique_ptr<Layer> layer)
{
    m_layers.insert(m_layers.begin() + index, std::move(layer));
    emit stackChanged();
}

std::unique_ptr<Layer> LayerStack::removeLayer(int index)
{
    auto it = m_layers.begin() + index;
    auto layer = std::move(*it);
    m_layers.erase(it);
    emit stackChanged();
    return layer;
}

void LayerStack::moveLayer(int fromIndex, int toIndex)
{
    if (fromIndex == toIndex) return;
    auto layer = std::move(m_layers[fromIndex]);
    m_layers.erase(m_layers.begin() + fromIndex);
    m_layers.insert(m_layers.begin() + toIndex, std::move(layer));
    emit stackChanged();
}

Layer* LayerStack::layerById(const QUuid& id)
{
    for (auto& l : m_layers)
        if (l->id() == id) return l.get();
    return nullptr;
}

int LayerStack::indexOfId(const QUuid& id) const
{
    for (int i = 0; i < static_cast<int>(m_layers.size()); ++i)
        if (m_layers[i]->id() == id) return i;
    return -1;
}

} // namespace Zyri
