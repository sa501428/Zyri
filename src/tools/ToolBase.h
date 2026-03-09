#pragma once
#include <QCursor>
#include <QPointF>
#include <QObject>
#include <QString>

namespace Zyri {
class DocumentController;

// Abstract base for all interactive tools.
class ToolBase : public QObject {
    Q_OBJECT
public:
    explicit ToolBase(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~ToolBase() = default;

    [[nodiscard]] virtual QString name()   const = 0;
    [[nodiscard]] virtual QCursor cursor() const { return Qt::CrossCursor; }

    // Called when this tool becomes active/inactive.
    virtual void activate(DocumentController* controller)   { m_controller = controller; }
    virtual void deactivate()                               { m_controller = nullptr; }

    // Pointer events (canvas coordinates, pressure in [0,1]).
    virtual void pointerDown(const QPointF& /*pos*/, qreal /*pressure*/ = 1.0) {}
    virtual void pointerMove(const QPointF& /*pos*/, qreal /*pressure*/ = 1.0) {}
    virtual void pointerUp(const QPointF& /*pos*/)                             {}

    // Returns a widget to show in the tool options bar, or nullptr.
    virtual QWidget* createOptionsWidget() { return nullptr; }

protected:
    DocumentController* m_controller = nullptr;
};
} // namespace Zyri
