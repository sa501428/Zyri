#pragma once

#include "Layer.h"
#include <memory>

namespace Zyri {

// Forward declaration — VectorScene definition is in src/engine/vector/
class VectorScene;

class VectorLayer : public Layer
{
public:
    explicit VectorLayer(const QString& name = {});
    // Destructor is defined in VectorLayer.cpp where VectorScene is complete.
    ~VectorLayer() override;

    [[nodiscard]] VectorScene*       scene()       { return m_scene.get(); }
    [[nodiscard]] const VectorScene* scene() const { return m_scene.get(); }

private:
    // unique_ptr with incomplete type is valid as long as destructor/reset are
    // only instantiated where VectorScene is fully defined (i.e. in VectorLayer.cpp).
    std::unique_ptr<VectorScene> m_scene;
};

} // namespace Zyri
