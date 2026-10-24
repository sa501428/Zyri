#pragma once
#include "Session.h"
#include <QCache>
#include <QWidget>
namespace Zyri {
enum class Tool {
    Brush,
    Pencil,
    Eraser,
    Fill,
    Picker,
    RectangleSelect,
    EllipseSelect,
    Lasso,
    Move,
    Rectangle,
    Ellipse,
    RoundedRectangle,
    Line,
    Polygon,
    Path,
    Node,
    Text,
    Crop
};
class Canvas : public QWidget {
    Q_OBJECT
  public:
    explicit Canvas(Session* session, QWidget* parent = nullptr);
    Session* session() const {
        return m_session;
    }
    Tool tool = Tool::Brush;
    int fillTolerance = 20;
    bool sampleVisible = false;
    ViewTransform view;
    void setTool(Tool value);
    void fit();
    void zoom(double factor);
    void rotate(double degrees);
    void finishPath();
    void cancelInteraction();
    void editText();
    void deleteNode();
    QSize sizeHint() const override {
        return {1000, 700};
    }
  signals:
    void fillRequested(QPoint point, int tolerance, bool visible);
    void cancelJobRequested();
    void positionChanged(QPointF position, double zoom);
    void toolChanged(Tool tool);

  protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;
    void focusOutEvent(QFocusEvent*) override;
    void tabletEvent(QTabletEvent*) override;
    bool event(QEvent*) override;

  private:
    Session* m_session;
    QCache<quint64, QImage> m_tiles{128 * 1024};
    bool m_drag = false, m_panning = false, m_space = false, m_first = true, m_penActive = false,
         m_tablet = false;
    QPointF m_start, m_current, m_lastView, m_penPrevious;
    double m_inputPressure = 1;
    QPainterPath m_preview, m_pen;
    Document m_dragDocument;
    QImage m_moveBase, m_movePixels;
    int m_node = -1, m_handle = -1;
    QUuid m_dragObject;
    void press(QPointF viewPoint, double pressure, Qt::KeyboardModifiers mods);
    void move(QPointF viewPoint, double pressure, Qt::MouseButtons buttons);
    void release(QPointF viewPoint, Qt::KeyboardModifiers mods);
    void updatePreview(QPointF point);
    Object makeObject() const;
};
} // namespace Zyri
