#include "Canvas.h"
#include <QApplication>
#include <QFocusEvent>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QNativeGestureEvent>
#include <QPainter>
#include <QTabletEvent>
#include <QWheelEvent>
#include <cmath>
#include <numbers>

namespace Zyri {
Canvas::Canvas(Session* s, QWidget* parent) : QWidget(parent), m_session(s) {
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent);
    view.center = QPointF(s->document().size.width() / 2., s->document().size.height() / 2.);
    connect(s, &Session::changed, this, [this](QRect region) {
        if (region.isEmpty())
            m_tiles.clear();
        else
            for (int y = region.top() / TileSize; y <= region.bottom() / TileSize; ++y)
                for (int x = region.left() / TileSize; x <= region.right() / TileSize; ++x)
                    m_tiles.remove(tileKey(x, y));
        update();
    });
}
void Canvas::setTool(Tool value) {
    cancelInteraction();
    tool = value;
    setCursor(tool == Tool::Move ? Qt::SizeAllCursor : Qt::CrossCursor);
    emit toolChanged(tool);
    update();
}
void Canvas::fit() {
    QSize s = m_session->document().size;
    view.center = QPointF(s.width() / 2., s.height() / 2.);
    view.rotation = 0;
    view.zoom =
        std::clamp(std::min((width() - 64.) / s.width(), (height() - 64.) / s.height()), .02, 64.);
    update();
    emit positionChanged({}, view.zoom);
}
void Canvas::zoom(double factor) {
    view.zoomAt(factor, QPointF(width() / 2., height() / 2.), size());
    update();
    emit positionChanged({}, view.zoom);
}
void Canvas::rotate(double degrees) {
    view.rotation = std::fmod(view.rotation + degrees, 360.);
    update();
}
void Canvas::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    if (m_first && width() > 100) {
        m_first = false;
        fit();
    }
}
void Canvas::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor(37, 39, 44));
    p.setTransform(view.matrix(size()));
    const QSize docSize = m_session->document().size;
    QRect canvas(QPoint(), docSize);
    p.setClipRect(canvas);
    QImage checker(16, 16, QImage::Format_RGB32);
    checker.fill(QColor(224, 225, 228));
    QPainter cp(&checker);
    cp.fillRect(0, 0, 8, 8, QColor(190, 192, 198));
    cp.fillRect(8, 8, 8, 8, QColor(190, 192, 198));
    cp.end();
    p.fillRect(canvas, QBrush(checker));
    QRect visible =
        view.matrix(size()).inverted().mapRect(QRectF(rect())).toAlignedRect().intersected(canvas);
    p.setRenderHint(QPainter::SmoothPixmapTransform, view.zoom < 1);
    if (!visible.isEmpty())
        for (int y = visible.top() / TileSize; y <= visible.bottom() / TileSize; ++y)
            for (int x = visible.left() / TileSize; x <= visible.right() / TileSize; ++x) {
                quint64 key = tileKey(x, y);
                auto image = m_tiles.object(key);
                if (!image) {
                    image = new QImage(
                        renderRegion(m_session->document(),
                                     QRect(x * TileSize, y * TileSize, TileSize, TileSize)));
                    m_tiles.insert(key, image, int(image->sizeInBytes() / 1024) + 1);
                }
                p.drawImage(QPoint(x * TileSize, y * TileSize), *image);
            }
    p.setRenderHint(QPainter::Antialiasing);
    QPen outline(QColor(40, 140, 255), 0, Qt::DashLine);
    outline.setCosmetic(true);
    p.setPen(outline);
    p.setBrush(Qt::NoBrush);
    if (m_session->hasSelection)
        p.drawPath(m_session->selection);
    if (m_drag && !m_preview.isEmpty()) {
        p.fillPath(m_preview, QColor(60, 140, 255, 35));
        p.drawPath(m_preview);
    }
    if (m_penActive)
        p.drawPath(m_pen);
    if (auto object = m_session->object()) {
        QPainterPath bounds;
        bounds.addRect(object->geometry().boundingRect());
        p.drawPath(object->transform.map(bounds));
        double handle = 4 / view.zoom;
        p.setBrush(Qt::white);
        auto polygon = object->transform.map(QPolygonF(object->geometry().boundingRect()));
        for (auto point : polygon)
            p.drawRect(QRectF(point - QPointF(handle, handle), QSizeF(2 * handle, 2 * handle)));
        if (tool == Tool::Node) {
            p.setBrush(QColor(30, 160, 255));
            for (int i = 0; i < object->path.elementCount(); ++i) {
                auto e = object->path.elementAt(i);
                QPointF pt = object->transform.map(QPointF(e.x, e.y));
                p.drawEllipse(pt, handle, handle);
            }
        }
    }
    p.setClipping(false);
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(10, 10, 10, 150), 0));
    p.drawRect(canvas);
}
void Canvas::cancelInteraction() {
    m_session->cancel();
    m_drag = false;
    m_panning = false;
    m_node = -1;
    m_handle = -1;
    m_preview = {};
    m_pen = {};
    m_penActive = false;
    update();
}
void Canvas::mousePressEvent(QMouseEvent* e) {
    setFocus();
    if (e->button() == Qt::MiddleButton || (m_space && e->button() == Qt::LeftButton)) {
        m_panning = true;
        m_lastView = e->position();
        setCursor(Qt::ClosedHandCursor);
        return;
    }
    if (e->button() == Qt::RightButton) {
        if (m_penActive)
            finishPath();
        return;
    }
    if (e->button() == Qt::LeftButton && !m_tablet)
        press(e->position(), 1, e->modifiers());
}
void Canvas::mouseMoveEvent(QMouseEvent* e) {
    if (m_panning) {
        QPointF a = view.toDocument(m_lastView, size()), b = view.toDocument(e->position(), size());
        view.center += a - b;
        m_lastView = e->position();
        update();
        return;
    }
    if (!m_tablet)
        move(e->position(), 1, e->buttons());
}
void Canvas::mouseReleaseEvent(QMouseEvent* e) {
    if (m_panning) {
        m_panning = false;
        setCursor(Qt::CrossCursor);
        return;
    }
    if (e->button() == Qt::LeftButton && !m_tablet)
        release(e->position(), e->modifiers());
}
void Canvas::press(QPointF vp, double pressure, Qt::KeyboardModifiers) {
    m_inputPressure = pressure;
    m_start = m_current = view.toDocument(vp, size());
    m_drag = true;
    m_preview = {};
    m_handle = -1;
    if (tool == Tool::Brush || tool == Tool::Pencil || tool == Tool::Eraser) {
        m_drag =
            m_session->beginStroke(m_start, pressure, tool == Tool::Eraser, tool == Tool::Pencil);
        return;
    }
    if (tool == Tool::Fill) {
        emit fillRequested(m_start.toPoint(), fillTolerance, sampleVisible);
        m_drag = false;
        return;
    }
    if (tool == Tool::Picker) {
        QImage pixel = renderRegion(m_session->document(), QRect(m_start.toPoint(), QSize(1, 1)));
        if (!pixel.isNull())
            m_session->foreground = pixel.pixelColor(0, 0);
        emit m_session->changed({});
        m_drag = false;
        return;
    }
    if (tool == Tool::Path) {
        if (!m_penActive) {
            m_pen = {};
            m_pen.moveTo(m_start);
            m_penPrevious = m_start;
            m_penActive = true;
        }
        return;
    }
    if (tool == Tool::Lasso) {
        m_preview.moveTo(m_start);
        return;
    }
    if (tool == Tool::Move || tool == Tool::Node) {
        auto selected = m_session->object();
        if (selected && tool == Tool::Move) {
            QPolygonF corners =
                selected->transform.map(QPolygonF(selected->geometry().boundingRect()));
            for (int i = 0; i < 4 && i < corners.size(); ++i)
                if (QLineF(corners[i], m_start).length() < 8 / view.zoom)
                    m_handle = i;
        }
        if (m_handle < 0) {
            int index = m_session->objectAt(m_start, 5 / view.zoom);
            if (index >= 0)
                m_session->selectedObject = m_session->document()
                                                .layers[size_t(m_session->activeLayer)]
                                                .objects[size_t(index)]
                                                .id;
            else if (tool != Tool::Node)
                m_session->selectedObject = {};
        }
        m_dragObject = m_session->selectedObject;
        m_dragDocument = m_session->document();
        if (tool == Tool::Node) {
            m_node = -1;
            if (auto o = m_session->object())
                for (int i = 0; i < o->path.elementCount(); ++i) {
                    auto e = o->path.elementAt(i);
                    if (QLineF(o->transform.map(QPointF(e.x, e.y)), m_start).length() <
                        8 / view.zoom) {
                        m_node = i;
                        break;
                    }
                }
            if (m_node < 0) {
                m_drag = false;
                update();
                return;
            }
        }
        if (m_session->document().layers[size_t(m_session->activeLayer)].locked) {
            m_drag = false;
            return;
        }
        if (!m_session->begin(tool == Tool::Node ? tr("Move node") : tr("Move / scale"))) {
            m_drag = false;
            return;
        }
        if (!m_session->object() &&
            m_session->document().layers[size_t(m_session->activeLayer)].kind ==
                LayerKind::Raster) {
            m_movePixels = renderLayer(m_dragDocument, m_session->activeLayer);
            m_moveBase = QImage(m_movePixels.size(), m_movePixels.format());
            m_moveBase.fill(Qt::transparent);
            if (m_session->hasSelection) {
                QImage mask(m_movePixels.size(), m_movePixels.format());
                mask.fill(Qt::transparent);
                QPainter p(&mask);
                p.fillPath(m_session->selection, Qt::white);
                p.end();
                m_moveBase = m_movePixels;
                QPainter cut(&m_moveBase);
                cut.setCompositionMode(QPainter::CompositionMode_DestinationOut);
                cut.drawImage(0, 0, mask);
                cut.end();
                QPainter keep(&m_movePixels);
                keep.setCompositionMode(QPainter::CompositionMode_DestinationIn);
                keep.drawImage(0, 0, mask);
            }
        }
        update();
    }
}
void Canvas::updatePreview(QPointF point) {
    QRectF r = QRectF(m_start, point).normalized();
    m_preview = {};
    if (tool == Tool::EllipseSelect || tool == Tool::Ellipse)
        m_preview.addEllipse(r);
    else if (tool == Tool::Line) {
        m_preview.moveTo(m_start);
        m_preview.lineTo(point);
    } else if (tool == Tool::RoundedRectangle)
        m_preview.addRoundedRect(r, 12, 12);
    else if (tool == Tool::Polygon) {
        QPolygonF polygon;
        for (int i = 0; i < 5; ++i) {
            double angle = -std::numbers::pi / 2 + i * 2 * std::numbers::pi / 5;
            polygon << r.center() + QPointF(std::cos(angle) * r.width() / 2,
                                            std::sin(angle) * r.height() / 2);
        }
        m_preview.addPolygon(polygon);
        m_preview.closeSubpath();
    } else
        m_preview.addRect(r);
}
void Canvas::move(QPointF vp, double pressure, Qt::MouseButtons buttons) {
    m_inputPressure = pressure;
    m_current = view.toDocument(vp, size());
    emit positionChanged(m_current, view.zoom);
    if (!m_drag || !buttons.testFlag(Qt::LeftButton))
        return;
    if (tool == Tool::Brush || tool == Tool::Pencil || tool == Tool::Eraser) {
        m_session->strokeTo(m_current, pressure);
        return;
    }
    if (tool == Tool::Lasso)
        m_preview.lineTo(m_current);
    else if (tool == Tool::Path) {
        update();
        return;
    } else if ((tool == Tool::Move || tool == Tool::Node) && m_session->editing()) {
        QPointF delta = m_current - m_start;
        Document base = m_dragDocument;
        int layer = m_session->activeLayer;
        m_session->mutate([&](Document& d) {
            d = base;
            auto& l = d.layers[size_t(layer)];
            if (!m_dragObject.isNull()) {
                for (auto& o : l.objects)
                    if (o.id == m_dragObject) {
                        if (tool == Tool::Node) {
                            QPointF local = o.transform.inverted().map(m_current);
                            o.path.setElementPositionAt(m_node, local.x(), local.y());
                        } else if (m_handle >= 0) {
                            QPolygonF corners =
                                o.transform.map(QPolygonF(o.geometry().boundingRect()));
                            QPointF pivot = corners[(m_handle + 2) % 4];
                            auto scale = [](double distance, double original) {
                                if (std::abs(original) < .01)
                                    return 1.;
                                double factor = std::clamp(distance / original, -20., 20.);
                                return std::abs(factor) < .01 ? .01 : factor;
                            };
                            double sx = scale(m_current.x() - pivot.x(), m_start.x() - pivot.x());
                            double sy = scale(m_current.y() - pivot.y(), m_start.y() - pivot.y());
                            QTransform t;
                            t.translate(pivot.x(), pivot.y());
                            t.scale(sx, sy);
                            t.translate(-pivot.x(), -pivot.y());
                            o.transform = o.transform * t;
                        } else {
                            QTransform t;
                            t.translate(delta.x(), delta.y());
                            o.transform = o.transform * t;
                        }
                    }
            } else if (l.kind == LayerKind::Vector) {
                for (auto& o : l.objects) {
                    QTransform t;
                    t.translate(delta.x(), delta.y());
                    o.transform = o.transform * t;
                }
            } else {
                QImage result = m_moveBase;
                QPainter p(&result);
                p.drawImage(delta, m_movePixels);
                p.end();
                replaceRaster(l, result);
            }
        });
    } else
        updatePreview(m_current);
    update();
}
Object Canvas::makeObject() const {
    Object o;
    o.rect = QRectF(m_start, m_current).normalized();
    o.fill = m_session->foreground;
    o.stroke = m_session->background;
    o.strokeWidth = 2;
    switch (tool) {
    case Tool::Ellipse:
        o.kind = ObjectKind::Ellipse;
        break;
    case Tool::RoundedRectangle:
        o.kind = ObjectKind::RoundedRectangle;
        break;
    case Tool::Line:
        o.kind = ObjectKind::Path;
        o.path.moveTo(m_start);
        o.path.lineTo(m_current);
        o.fill = Qt::transparent;
        o.stroke = m_session->foreground;
        break;
    case Tool::Polygon:
        o.kind = ObjectKind::Polygon;
        o.path = m_preview;
        break;
    default:
        o.kind = ObjectKind::Rectangle;
        break;
    }
    return o;
}
void Canvas::release(QPointF vp, Qt::KeyboardModifiers mods) {
    if (!m_drag)
        return;
    m_current = view.toDocument(vp, size());
    m_drag = false;
    if (tool == Tool::Brush || tool == Tool::Pencil || tool == Tool::Eraser) {
        m_session->strokeTo(m_current, m_inputPressure);
        m_session->endStroke();
    } else if (tool == Tool::Move || tool == Tool::Node) {
        m_session->commit();
        if (tool == Tool::Move && m_dragObject.isNull() && m_session->hasSelection) {
            QTransform t;
            t.translate(m_current.x() - m_start.x(), m_current.y() - m_start.y());
            m_session->selection = t.map(m_session->selection);
        }
    } else if (tool == Tool::RectangleSelect || tool == Tool::EllipseSelect ||
               tool == Tool::Lasso) {
        if (tool == Tool::Lasso) {
            m_preview.lineTo(m_current);
            m_preview.closeSubpath();
        } else
            updatePreview(m_current);
        m_session->select(m_preview, mods);
    } else if (tool == Tool::Crop) {
        QRect crop = QRectF(m_start, m_current)
                         .normalized()
                         .toAlignedRect()
                         .intersected(QRect(QPoint(), m_session->document().size));
        if (!crop.isEmpty())
            m_session->resize(crop.size(), false, -crop.topLeft());
    } else if (tool == Tool::Path) {
        if (QLineF(m_penPrevious, m_start).length() > 0.1) {
            QPointF handle = m_current - m_start;
            if (handle.manhattanLength() > 2)
                m_pen.cubicTo(m_penPrevious + handle, m_start - handle, m_start);
            else
                m_pen.lineTo(m_start);
        }
        m_penPrevious = m_start;
    } else if (tool == Tool::Text) {
        Object o;
        o.kind = ObjectKind::Text;
        o.rect = QRectF(m_start, m_current).normalized();
        if (o.rect.width() < 20)
            o.rect = QRectF(m_start, QSizeF(400, 160));
        o.fill = m_session->foreground;
        bool ok;
        o.text = QInputDialog::getMultiLineText(this, tr("Text"), tr("Text content"), {}, &ok);
        if (ok && !o.text.isEmpty())
            m_session->addObject(o);
    } else if (QLineF(m_start, m_current).length() > 1) {
        updatePreview(m_current);
        m_session->addObject(makeObject());
    }
    m_preview = {};
    m_moveBase = {};
    m_movePixels = {};
    update();
}
void Canvas::finishPath() {
    if (!m_penActive)
        return;
    if (m_pen.elementCount() > 1) {
        Object o;
        o.kind = ObjectKind::Path;
        o.path = m_pen;
        o.fill = Qt::transparent;
        o.stroke = m_session->foreground;
        o.strokeWidth = 2;
        m_session->addObject(o);
    }
    m_pen = {};
    m_penActive = false;
    m_drag = false;
    update();
}
void Canvas::editText() {
    if (auto o = m_session->object(); o && o->kind == ObjectKind::Text) {
        bool ok;
        QString text =
            QInputDialog::getMultiLineText(this, tr("Edit text"), tr("Content"), o->text, &ok);
        if (ok)
            m_session->editObject(tr("Edit text"), [&](Object& object) { object.text = text; });
    }
}
void Canvas::deleteNode() {
    if (tool != Tool::Node || m_node < 0 || !m_session->object())
        return;
    int node = m_node;
    m_session->editObject(tr("Delete node"), [=](Object& o) {
        QPainterPath result;
        for (int i = 0; i < o.path.elementCount(); ++i) {
            auto e = o.path.elementAt(i);
            if (e.type == QPainterPath::CurveToElement) {
                auto b = o.path.elementAt(i + 1), c = o.path.elementAt(i + 2);
                if (node < i || node > i + 2)
                    result.cubicTo(e.x, e.y, b.x, b.y, c.x, c.y);
                i += 2;
            } else if (i != node) {
                if (e.type == QPainterPath::MoveToElement || result.isEmpty())
                    result.moveTo(e.x, e.y);
                else
                    result.lineTo(e.x, e.y);
            }
        }
        o.path = result;
    });
    m_node = -1;
}
void Canvas::mouseDoubleClickEvent(QMouseEvent* e) {
    if (tool == Tool::Path) {
        finishPath();
        return;
    }
    if (tool == Tool::Node && m_session->object()) {
        QPointF p =
            m_session->object()->transform.inverted().map(view.toDocument(e->position(), size()));
        m_session->editObject(tr("Append node"), [&](Object& o) { o.path.lineTo(p); });
        return;
    }
    editText();
}
void Canvas::wheelEvent(QWheelEvent* e) {
    if (e->modifiers().testFlag(Qt::ControlModifier) || e->modifiers().testFlag(Qt::MetaModifier)) {
        view.zoomAt(std::pow(1.15, e->angleDelta().y() / 120.), e->position(), size());
    } else {
        QPointF delta = e->pixelDelta();
        if (delta.isNull())
            delta = QPointF(e->angleDelta()) / 3.;
        QPointF before = view.toDocument({}, size()), after = view.toDocument(delta, size());
        view.center += before - after;
    }
    update();
    emit positionChanged(view.toDocument(e->position(), size()), view.zoom);
    e->accept();
}
void Canvas::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Space) {
        m_space = true;
        setCursor(Qt::OpenHandCursor);
        e->accept();
    } else if (e->key() == Qt::Key_Escape) {
        cancelInteraction();
        emit cancelJobRequested();
    } else if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter)
        finishPath();
    else if (e->key() == Qt::Key_Backspace || e->key() == Qt::Key_Delete) {
        if (tool == Tool::Node)
            deleteNode();
        else
            m_session->clearPixels();
    } else if (e->modifiers() == Qt::NoModifier) {
        switch (e->key()) {
        case Qt::Key_B:
            setTool(Tool::Brush);
            break;
        case Qt::Key_N:
            setTool(Tool::Pencil);
            break;
        case Qt::Key_E:
            setTool(Tool::Eraser);
            break;
        case Qt::Key_G:
            setTool(Tool::Fill);
            break;
        case Qt::Key_I:
            setTool(Tool::Picker);
            break;
        case Qt::Key_M:
            setTool(Tool::RectangleSelect);
            break;
        case Qt::Key_V:
            setTool(Tool::Move);
            break;
        case Qt::Key_P:
            setTool(Tool::Path);
            break;
        case Qt::Key_T:
            setTool(Tool::Text);
            break;
        default:
            QWidget::keyPressEvent(e);
            break;
        }
    } else
        QWidget::keyPressEvent(e);
}
void Canvas::keyReleaseEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Space) {
        m_space = false;
        m_panning = false;
        setCursor(Qt::CrossCursor);
    } else
        QWidget::keyReleaseEvent(e);
}
void Canvas::focusOutEvent(QFocusEvent* e) {
    if (m_drag)
        cancelInteraction();
    m_tablet = false;
    m_space = false;
    m_panning = false;
    QWidget::focusOutEvent(e);
}
void Canvas::tabletEvent(QTabletEvent* e) {
    if (e->type() == QEvent::TabletPress) {
        m_tablet = true;
        press(e->position(), e->pressure(), e->modifiers());
    } else if (e->type() == QEvent::TabletMove)
        move(e->position(), e->pressure(), e->buttons());
    else if (e->type() == QEvent::TabletRelease) {
        release(e->position(), e->modifiers());
        m_tablet = false;
    }
    e->accept();
}
bool Canvas::event(QEvent* e) {
    if (e->type() == QEvent::NativeGesture) {
        auto* gesture = static_cast<QNativeGestureEvent*>(e);
        if (gesture->gestureType() == Qt::ZoomNativeGesture) {
            view.zoomAt(1 + gesture->value(), gesture->position(), size());
            update();
            return true;
        }
    }
    return QWidget::event(e);
}
} // namespace Zyri
