#pragma once

#include "Document.h"
#include <QColor>
#include <QObject>

namespace Zyri {

// DocumentController is the single mutation entry-point for a Document.
// All structural changes go through here so that undo commands can be
// constructed and pushed to the UndoStack before the document is altered.
class DocumentController : public QObject
{
    Q_OBJECT
public:
    explicit DocumentController(Document* document, QObject* parent = nullptr);

    [[nodiscard]] Document* document() const { return m_document; }

    // Color state
    [[nodiscard]] QColor foregroundColor() const { return m_foreground; }
    [[nodiscard]] QColor backgroundColor() const { return m_background; }
    void setForegroundColor(const QColor& c);
    void setBackgroundColor(const QColor& c);

signals:
    void foregroundColorChanged(const QColor& c);
    void backgroundColorChanged(const QColor& c);

private:
    Document* m_document;
    QColor    m_foreground{Qt::black};
    QColor    m_background{Qt::white};
};

} // namespace Zyri
