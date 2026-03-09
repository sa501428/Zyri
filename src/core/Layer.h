#pragma once

#include "BlendMode.h"
#include <QString>
#include <QUuid>

namespace Zyri {

enum class LayerType { Raster, Vector, Text };

class Layer
{
public:
    explicit Layer(LayerType type, const QString& name = {});
    virtual ~Layer() = default;

    // Non-copyable, movable
    Layer(const Layer&) = delete;
    Layer& operator=(const Layer&) = delete;

    [[nodiscard]] QUuid       id()        const { return m_id; }
    [[nodiscard]] LayerType   type()      const { return m_type; }
    [[nodiscard]] QString     name()      const { return m_name; }
    [[nodiscard]] bool        visible()   const { return m_visible; }
    [[nodiscard]] bool        locked()    const { return m_locked; }
    [[nodiscard]] float       opacity()   const { return m_opacity; }
    [[nodiscard]] BlendMode   blendMode() const { return m_blendMode; }

    void setName(const QString& name)        { m_name = name; }
    void setVisible(bool v)                  { m_visible = v; }
    void setLocked(bool l)                   { m_locked = l; }
    void setOpacity(float o)                 { m_opacity = qBound(0.0f, o, 1.0f); }
    void setBlendMode(BlendMode m)           { m_blendMode = m; }

private:
    QUuid     m_id;
    LayerType m_type;
    QString   m_name;
    bool      m_visible  = true;
    bool      m_locked   = false;
    float     m_opacity  = 1.0f;
    BlendMode m_blendMode = BlendMode::Normal;
};

} // namespace Zyri
