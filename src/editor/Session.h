#pragma once
#include "Document.h"
#include <QObject>
#include <QTemporaryDir>
#include <functional>
#include <memory>
#include <optional>

namespace Zyri {
struct Brush {
    double size = 20, hardness = .8, opacity = 1, flow = 1, spacing = .15;
    bool pressure = true;
};
class Session : public QObject {
    Q_OBJECT
  public:
    explicit Session(Document document, QObject* parent = nullptr);
    [[nodiscard]] const Document& document() const {
        return m_document;
    }
    [[nodiscard]] QUuid stateId() const {
        return m_state;
    }
    [[nodiscard]] quint64 revision() const {
        return m_revision;
    }
    [[nodiscard]] bool dirty() const {
        return m_state != m_saved;
    }
    [[nodiscard]] bool editing() const {
        return m_before.has_value();
    }
    [[nodiscard]] bool canUndo() const {
        return !editing() && m_cursor > 0;
    }
    [[nodiscard]] bool canRedo() const {
        return !editing() && m_cursor + 1 < int(m_history.size());
    }
    [[nodiscard]] QString undoLabel() const;
    [[nodiscard]] QString redoLabel() const;
    QString filePath;
    const QUuid sessionId = QUuid::createUuid();
    std::function<bool()> cancelRequested = [] { return false; };
    int activeLayer = 0;
    QUuid selectedObject;
    QColor foreground{Qt::black}, background{Qt::white};
    Brush brush;
    QPainterPath selection;
    bool hasSelection = false;
    qint64 historyBudget = 256LL * 1024 * 1024;
    qint64 spillBudget = 2LL * 1024 * 1024 * 1024;
    int historyLimit = 100;
    void markSaved(QUuid state);
    void markUnsaved();
    [[nodiscard]] bool begin(const QString& label);
    void mutate(const std::function<void(Document&)>& operation, QRect dirty = {});
    bool commit();
    void cancel();
    bool apply(const QString& label, const std::function<void(Document&)>& operation);
    void undo();
    void redo();
    int addLayer(LayerKind kind);
    void deleteLayer();
    void duplicateLayer();
    void reorderLayer(int offset);
    void setLayerProperty(const QString& label, const std::function<void(Layer&)>& operation);
    void select(QPainterPath path, Qt::KeyboardModifiers modifiers = {});
    void deselect();
    bool beginStroke(QPointF point, double pressure = 1, bool erase = false, bool pencil = false);
    void strokeTo(QPointF point, double pressure = 1);
    void endStroke();
    void fill(QPoint point, int tolerance = 20, bool sampleVisible = false);
    void clearPixels();
    void paste(const QImage& image);
    [[nodiscard]] QImage copyPixels(bool merged = false) const;
    void transformActive(const QTransform& transform, bool selectedPixelsOnly = true);
    void resize(QSize size, bool resample, QPoint offset = {});
    void rasterize();
    void addObject(Object object);
    void editObject(const QString& label, const std::function<void(Object&)>& operation);
    void deleteObject();
    [[nodiscard]] int objectAt(QPointF point, double tolerance = 4) const;
    [[nodiscard]] const Object* object() const;
  signals:
    void changed(QRect region);
    void historyChanged();
    void errorOccurred(QString message);

  private:
    struct Entry {
        std::shared_ptr<const Document> document;
        QString label, spill;
        QUuid state;
        qint64 spillBytes = 0;
    };
    Document m_document;
    std::optional<Document> m_before;
    QString m_label;
    bool m_changed = false;
    QUuid m_state = QUuid::createUuid(), m_saved = m_state;
    quint64 m_revision = 0;
    std::vector<Entry> m_history;
    int m_cursor = 0;
    QTemporaryDir m_scratch;
    bool m_stroking = false, m_erase = false;
    Brush m_strokeBrush;
    QColor m_strokeColor;
    QPointF m_lastPoint;
    double m_lastPressure = 1, m_remaining = 0;
    QMap<quint64, QImage> m_coverage;
    void stamp(QPointF point, double pressure);
    void enforceBudget();
    bool restore(int index);
};
} // namespace Zyri
