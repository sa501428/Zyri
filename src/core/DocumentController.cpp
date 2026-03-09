#include "DocumentController.h"

namespace Zyri {

DocumentController::DocumentController(Document* document, QObject* parent)
    : QObject(parent)
    , m_document(document)
{
    Q_ASSERT(document);
}

void DocumentController::setForegroundColor(const QColor& c)
{
    if (m_foreground == c) return;
    m_foreground = c;
    emit foregroundColorChanged(c);
}

void DocumentController::setBackgroundColor(const QColor& c)
{
    if (m_background == c) return;
    m_background = c;
    emit backgroundColorChanged(c);
}

} // namespace Zyri
