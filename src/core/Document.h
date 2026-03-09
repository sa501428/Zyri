#pragma once

#include "LayerStack.h"
#include "ColorProfile.h"
#include <QColor>
#include <QObject>
#include <QSize>
#include <QString>

namespace Zyri {

class Document : public QObject
{
    Q_OBJECT
public:
    explicit Document(QObject* parent = nullptr);
    explicit Document(const QSize& canvasSize, int dpi = 96,
                      const QColor& background = Qt::white,
                      QObject* parent = nullptr);

    [[nodiscard]] QSize       canvasSize()    const { return m_canvasSize; }
    [[nodiscard]] int         dpi()           const { return m_dpi; }
    [[nodiscard]] QColor      background()    const { return m_background; }
    [[nodiscard]] LayerStack* layers()              { return &m_layers; }
    [[nodiscard]] const LayerStack* layers()  const { return &m_layers; }
    [[nodiscard]] bool        isDirty()       const { return m_dirty; }
    [[nodiscard]] QString     filePath()      const { return m_filePath; }
    [[nodiscard]] ColorProfile& colorProfile()      { return m_colorProfile; }

    void setCanvasSize(const QSize& sz);
    void setDpi(int dpi);
    void setBackground(const QColor& c);
    void setFilePath(const QString& path);
    void setDirty(bool dirty);

signals:
    void documentChanged();

private:
    QSize         m_canvasSize{1920, 1080};
    int           m_dpi       = 96;
    QColor        m_background{Qt::white};
    LayerStack    m_layers;
    bool          m_dirty     = false;
    QString       m_filePath;
    ColorProfile  m_colorProfile;
};

} // namespace Zyri
