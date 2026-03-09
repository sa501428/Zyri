#pragma once
#include <QColor>
#include <QPainterPath>
#include <QPainter>
#include <QUuid>

namespace Zyri {

class VectorObject {
public:
    explicit VectorObject(const QPainterPath& path = {});

    [[nodiscard]] QUuid          id()          const { return m_id; }
    [[nodiscard]] QPainterPath   path()        const { return m_path; }
    [[nodiscard]] QColor         fillColor()   const { return m_fill; }
    [[nodiscard]] QColor         strokeColor() const { return m_stroke; }
    [[nodiscard]] float          strokeWidth() const { return m_strokeWidth; }
    [[nodiscard]] float          opacity()     const { return m_opacity; }

    void setPath(const QPainterPath& p)  { m_path = p; }
    void setFillColor(const QColor& c)   { m_fill = c; }
    void setStrokeColor(const QColor& c) { m_stroke = c; }
    void setStrokeWidth(float w)         { m_strokeWidth = w; }
    void setOpacity(float o)             { m_opacity = qBound(0.0f, o, 1.0f); }

    void paintTo(QPainter& painter) const;

private:
    QUuid         m_id{QUuid::createUuid()};
    QPainterPath  m_path;
    QColor        m_fill{Qt::transparent};
    QColor        m_stroke{Qt::black};
    float         m_strokeWidth = 1.0f;
    float         m_opacity     = 1.0f;
};

} // namespace Zyri
