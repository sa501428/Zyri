#include "Document.h"

namespace Zyri {

Document::Document(QObject* parent)
    : QObject(parent)
    , m_colorProfile(ColorProfile::sRGB())
{
}

Document::Document(const QSize& canvasSize, int dpi, const QColor& background, QObject* parent)
    : QObject(parent)
    , m_canvasSize(canvasSize)
    , m_dpi(dpi)
    , m_background(background)
    , m_colorProfile(ColorProfile::sRGB())
{
}

void Document::setCanvasSize(const QSize& sz)
{
    if (m_canvasSize == sz) return;
    m_canvasSize = sz;
    setDirty(true);
    emit documentChanged();
}

void Document::setDpi(int dpi)
{
    if (m_dpi == dpi) return;
    m_dpi = dpi;
    setDirty(true);
}

void Document::setBackground(const QColor& c)
{
    if (m_background == c) return;
    m_background = c;
    setDirty(true);
    emit documentChanged();
}

void Document::setFilePath(const QString& path)
{
    m_filePath = path;
}

void Document::setDirty(bool dirty)
{
    m_dirty = dirty;
}

} // namespace Zyri
