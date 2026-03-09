#pragma once

#include "Layer.h"
#include <QRectF>
#include <QString>

namespace Zyri {

class TextLayer : public Layer
{
public:
    explicit TextLayer(const QString& name = {});

    [[nodiscard]] QString  html()        const { return m_html; }
    [[nodiscard]] QRectF   boundingRect() const { return m_rect; }

    void setHtml(const QString& html)           { m_html = html; }
    void setBoundingRect(const QRectF& rect)    { m_rect = rect; }

private:
    QString m_html;
    QRectF  m_rect;
};

} // namespace Zyri
