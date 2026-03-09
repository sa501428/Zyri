#pragma once

#include "Layer.h"
#include <QObject>
#include <memory>
#include <vector>

namespace Zyri {

class LayerStack : public QObject
{
    Q_OBJECT
public:
    explicit LayerStack(QObject* parent = nullptr);

    void addLayer(std::unique_ptr<Layer> layer);
    void insertLayer(int index, std::unique_ptr<Layer> layer);
    std::unique_ptr<Layer> removeLayer(int index);
    void moveLayer(int fromIndex, int toIndex);

    [[nodiscard]] int          count()              const { return static_cast<int>(m_layers.size()); }
    [[nodiscard]] Layer*       layerAt(int index)         { return m_layers[index].get(); }
    [[nodiscard]] const Layer* layerAt(int index)   const { return m_layers[index].get(); }
    [[nodiscard]] Layer*       layerById(const QUuid& id);
    [[nodiscard]] int          indexOfId(const QUuid& id) const;

signals:
    void stackChanged();

private:
    std::vector<std::unique_ptr<Layer>> m_layers;
};

} // namespace Zyri
