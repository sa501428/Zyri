#include "Session.h"
#include "ProjectIO.h"
#include <QBitArray>
#include <QFile>
#include <QPainter>
#include <QPainterPathStroker>
#include <QRadialGradient>
#include <QSaveFile>
#include <QtEndian>
#include <algorithm>
#include <cmath>
#include <set>

namespace Zyri {
Session::Session(Document d, QObject* parent) : QObject(parent), m_document(std::move(d)) {
    if (!validSize(m_document.size))
        m_document.size = QSize(1920, 1080);
    if (m_document.layers.empty())
        m_document.layers.emplace_back();
    m_history.push_back({std::make_shared<Document>(m_document), {}, {}, m_state, 0});
}
QString Session::undoLabel() const {
    return canUndo() ? m_history[size_t(m_cursor)].label : QString();
}
QString Session::redoLabel() const {
    return canRedo() ? m_history[size_t(m_cursor + 1)].label : QString();
}
void Session::markSaved(QUuid state) {
    m_saved = state;
    emit historyChanged();
}
void Session::markUnsaved() {
    m_saved = {};
    emit historyChanged();
}
bool Session::begin(const QString& label) {
    if (editing())
        return false;
    m_before = m_document;
    m_label = label;
    m_changed = false;
    return true;
}
void Session::mutate(const std::function<void(Document&)>& op, QRect region) {
    if (!editing())
        return;
    op(m_document);
    m_changed = true;
    ++m_revision;
    emit changed(region);
}
bool Session::commit() {
    if (!editing())
        return false;
    QString validationError = validateDocument(m_document);
    if (!validationError.isEmpty()) {
        cancel();
        emit errorOccurred(validationError);
        return false;
    }
    if (!m_changed || m_document == *m_before) {
        m_before.reset();
        m_coverage.clear();
        emit historyChanged();
        return false;
    }
    for (int i = m_cursor + 1; i < int(m_history.size()); ++i)
        if (!m_history[size_t(i)].spill.isEmpty())
            QFile::remove(m_history[size_t(i)].spill);
    m_history.erase(m_history.begin() + m_cursor + 1, m_history.end());
    m_state = QUuid::createUuid();
    m_history.push_back({std::make_shared<Document>(m_document), m_label, {}, m_state, 0});
    m_cursor = int(m_history.size()) - 1;
    m_before.reset();
    m_coverage.clear();
    enforceBudget();
    emit historyChanged();
    return true;
}
void Session::cancel() {
    if (!editing())
        return;
    m_document = std::move(*m_before);
    m_before.reset();
    m_stroking = false;
    m_coverage.clear();
    ++m_revision;
    emit changed({});
    emit historyChanged();
}
bool Session::apply(const QString& label, const std::function<void(Document&)>& op) {
    if (!begin(label))
        return false;
    mutate(op);
    return commit();
}
void Session::enforceBudget() {
    const int limit = std::max(1, historyLimit);
    auto drop = [&] {
        if (!m_history.front().spill.isEmpty())
            QFile::remove(m_history.front().spill);
        m_history.erase(m_history.begin());
        --m_cursor;
    };
    while (int(m_history.size()) > limit + 1 && m_cursor > 0)
        drop();
    auto resident = [&] {
        std::set<const uchar*> buffers;
        qint64 total = 0;
        for (const auto& e : m_history)
            if (e.document) {
                qint64 tileBytes = 0;
                for (const auto& l : e.document->layers)
                    for (const auto& image : l.tiles)
                        tileBytes += image.sizeInBytes();
                total += e.document->bytes() - tileBytes;
                for (const auto& l : e.document->layers)
                    for (const auto& image : l.tiles)
                        if (buffers.insert(image.constBits()).second)
                            total += image.sizeInBytes();
            }
        return total;
    };
    for (size_t i = 0; i < m_history.size() && resident() > std::max(qint64(0), historyBudget);
         ++i) {
        auto& entry = m_history[i];
        if (int(i) == m_cursor || !entry.document || !m_scratch.isValid())
            continue;
        QByteArray payload = qCompress(encodeState(*entry.document), 1);
        QString path = m_scratch.filePath(entry.state.toString(QUuid::WithoutBraces));
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(payload) != payload.size() ||
            !file.commit()) {
            emit errorOccurred(tr("History storage is full; older undo entries may be dropped."));
            break;
        }
        entry.spill = path;
        entry.spillBytes = payload.size();
        entry.document.reset();
    }
    qint64 spilled = 0;
    for (const auto& e : m_history)
        spilled += e.spillBytes;
    while (m_cursor > 0 &&
           (spilled > spillBudget || resident() > std::max(historyBudget, m_document.bytes()))) {
        spilled -= m_history.front().spillBytes;
        drop();
    }
}
bool Session::restore(int index) {
    if (editing() || index < 0 || index >= int(m_history.size()))
        return false;
    auto& e = m_history[size_t(index)];
    Document restored;
    if (e.document)
        restored = *e.document;
    else {
        QFile file(e.spill);
        if (!file.open(QIODevice::ReadOnly) || file.size() > MaxDocumentBytes * 2) {
            emit errorOccurred(tr("Cannot read undo history."));
            return false;
        }
        QByteArray compressed = file.readAll();
        if (compressed.size() < 4 || qFromBigEndian<quint32>(compressed.constData()) >
                                         quint64(MaxDocumentBytes + 32 * 1024 * 1024)) {
            emit errorOccurred(tr("Invalid undo payload size."));
            return false;
        }
        auto loaded = decodeState(qUncompress(compressed));
        if (!loaded) {
            emit errorOccurred(loaded.error);
            return false;
        }
        restored = std::move(loaded.document);
    }
    m_document = std::move(restored);
    m_cursor = index;
    m_state = e.state;
    ++m_revision;
    activeLayer = std::clamp(activeLayer, 0, int(m_document.layers.size()) - 1);
    selectedObject = {};
    emit changed({});
    emit historyChanged();
    return true;
}
void Session::undo() {
    if (canUndo())
        restore(m_cursor - 1);
}
void Session::redo() {
    if (canRedo())
        restore(m_cursor + 1);
}
int Session::addLayer(LayerKind kind) {
    int index = std::clamp(activeLayer + 1, 0, int(m_document.layers.size()));
    if (apply(tr("Add layer"), [&](Document& d) {
            Layer l;
            l.kind = kind;
            l.name = kind == LayerKind::Raster ? tr("Paint layer") : tr("Vector layer");
            d.layers.insert(d.layers.begin() + index, std::move(l));
        }))
        activeLayer = index;
    emit changed({});
    return activeLayer;
}
void Session::deleteLayer() {
    if (m_document.layers.size() < 2 || activeLayer < 0)
        return;
    int i = activeLayer;
    if (apply(tr("Delete layer"), [i](Document& d) { d.layers.erase(d.layers.begin() + i); }))
        activeLayer = std::min(i, int(m_document.layers.size()) - 1);
    selectedObject = {};
    emit changed({});
}
void Session::duplicateLayer() {
    int i = activeLayer;
    if (apply(tr("Duplicate layer"), [i](Document& d) {
            Layer l = d.layers[size_t(i)];
            l.id = QUuid::createUuid();
            l.name += QStringLiteral(" copy");
            for (auto& o : l.objects)
                o.id = QUuid::createUuid();
            d.layers.insert(d.layers.begin() + i + 1, std::move(l));
        }))
        activeLayer = i + 1;
    emit changed({});
}
void Session::reorderLayer(int offset) {
    int to = activeLayer + offset, from = activeLayer;
    if (to < 0 || to >= int(m_document.layers.size()))
        return;
    if (apply(tr("Reorder layer"),
              [=](Document& d) { std::swap(d.layers[size_t(from)], d.layers[size_t(to)]); }))
        activeLayer = to;
    emit changed({});
}
void Session::setLayerProperty(const QString& label, const std::function<void(Layer&)>& op) {
    int i = activeLayer;
    if (i >= 0 && i < int(m_document.layers.size()))
        apply(label, [&](Document& d) { op(d.layers[size_t(i)]); });
}
void Session::select(QPainterPath p, Qt::KeyboardModifiers mods) {
    ++m_revision;
    if (mods.testFlag(Qt::ShiftModifier) && mods.testFlag(Qt::AltModifier))
        selection = hasSelection ? selection.intersected(p) : p;
    else if (mods.testFlag(Qt::ShiftModifier))
        selection = hasSelection ? selection.united(p) : p;
    else if (mods.testFlag(Qt::AltModifier))
        selection = hasSelection ? selection.subtracted(p) : QPainterPath();
    else
        selection = std::move(p);
    hasSelection = true;
    emit changed({});
}
void Session::deselect() {
    ++m_revision;
    hasSelection = false;
    selection = {};
    emit changed({});
}
bool Session::beginStroke(QPointF point, double pressure, bool erase, bool pencil) {
    if (activeLayer < 0 || activeLayer >= int(m_document.layers.size()))
        return false;
    const auto& l = m_document.layers[size_t(activeLayer)];
    if (l.locked || l.kind != LayerKind::Raster) {
        emit errorOccurred(tr("Choose an unlocked raster layer for painting."));
        return false;
    }
    if (!std::isfinite(point.x()) || !std::isfinite(point.y()) || std::abs(point.x()) > 1e7 ||
        std::abs(point.y()) > 1e7 || !std::isfinite(pressure) || !std::isfinite(brush.size) ||
        !std::isfinite(brush.spacing) || !std::isfinite(brush.hardness) ||
        !std::isfinite(brush.opacity) || !std::isfinite(brush.flow) ||
        !begin(erase ? tr("Erase") : tr("Paint")))
        return false;
    m_stroking = true;
    m_erase = erase;
    m_strokeBrush = brush;
    m_strokeColor = foreground;
    m_strokeBrush.size = std::clamp(brush.size, 1., 1024.);
    m_strokeBrush.spacing = std::clamp(brush.spacing, .01, 1.);
    m_strokeBrush.hardness = std::clamp(brush.hardness, 0., 1.);
    m_strokeBrush.opacity = std::clamp(brush.opacity, 0., 1.);
    m_strokeBrush.flow = std::clamp(brush.flow, 0., 1.);
    if (pencil) {
        m_strokeBrush.hardness = 1;
        m_strokeBrush.spacing = .1;
        m_strokeBrush.pressure = false;
    }
    m_lastPoint = point;
    m_lastPressure = std::clamp(pressure, 0., 1.);
    m_remaining = 0;
    m_coverage.clear();
    stamp(point, m_lastPressure);
    return true;
}
void Session::strokeTo(QPointF point, double pressure) {
    if (!m_stroking || !std::isfinite(point.x()) || !std::isfinite(point.y()) ||
        std::abs(point.x()) > 1e7 || std::abs(point.y()) > 1e7 || !std::isfinite(pressure))
        return;
    pressure = std::clamp(pressure, 0., 1.);
    const double length = QLineF(m_lastPoint, point).length(),
                 step = std::max(.5, m_strokeBrush.size * m_strokeBrush.spacing);
    if (length > 100000)
        return;
    if (length > 0)
        for (double t = step - m_remaining; t <= length; t += step)
            stamp(m_lastPoint + (point - m_lastPoint) * (t / length),
                  m_lastPressure + (pressure - m_lastPressure) * (t / length));
    m_remaining = std::fmod(m_remaining + length, step);
    m_lastPoint = point;
    m_lastPressure = pressure;
}
void Session::endStroke() {
    if (!m_stroking)
        return;
    if (m_remaining > .001)
        stamp(m_lastPoint, m_lastPressure);
    m_stroking = false;
    commit();
}
void Session::stamp(QPointF point, double pressure) {
    double radius = std::max(.25, m_strokeBrush.size *
                                      (m_strokeBrush.pressure ? std::max(.01, pressure) : 1) * .5);
    QRect dirty =
        QRectF(point - QPointF(radius + 1, radius + 1), QSizeF(2 * radius + 2, 2 * radius + 2))
            .toAlignedRect()
            .intersected(QRect(QPoint(), m_document.size));
    if (dirty.isEmpty())
        return;
    auto& layer = m_document.layers[size_t(activeLayer)];
    const auto& before = m_before->layers[size_t(activeLayer)];
    for (int ty = dirty.top() / TileSize; ty <= dirty.bottom() / TileSize; ++ty)
        for (int tx = dirty.left() / TileSize; tx <= dirty.right() / TileSize; ++tx) {
            quint64 key = tileKey(tx, ty);
            QImage coverage = m_coverage.value(key);
            if (coverage.isNull())
                coverage = emptyTile();
            QPainter p(&coverage);
            p.setRenderHint(QPainter::Antialiasing);
            p.setClipRect(
                QRect(QPoint(), m_document.size).translated(-tx * TileSize, -ty * TileSize));
            QPointF local = point - QPointF(tx * TileSize, ty * TileSize);
            QColor c = m_erase ? QColor(Qt::black) : m_strokeColor;
            c.setAlphaF(c.alphaF() * m_strokeBrush.flow);
            QRadialGradient gradient(local, radius);
            gradient.setColorAt(0, c);
            if (m_strokeBrush.hardness < 1) {
                gradient.setColorAt(m_strokeBrush.hardness, c);
                QColor edge = c;
                edge.setAlpha(0);
                gradient.setColorAt(1, edge);
            } else
                gradient.setColorAt(1, c);
            p.setPen(Qt::NoPen);
            p.setBrush(gradient);
            p.drawEllipse(local, radius, radius);
            p.end();
            m_coverage[key] = coverage;
            if (hasSelection) {
                QImage mask = emptyTile();
                QPainter mp(&mask);
                mp.setRenderHint(QPainter::Antialiasing);
                mp.translate(-tx * TileSize, -ty * TileSize);
                mp.fillPath(selection, Qt::white);
                mp.end();
                QPainter cp(&coverage);
                cp.setCompositionMode(QPainter::CompositionMode_DestinationIn);
                cp.drawImage(0, 0, mask);
            }
            QImage result = tileImage(before, tx, ty);
            if (result.isNull())
                result = emptyTile();
            QPainter out(&result);
            out.setOpacity(m_strokeBrush.opacity);
            out.setCompositionMode(m_erase ? QPainter::CompositionMode_DestinationOut
                                           : QPainter::CompositionMode_SourceOver);
            out.drawImage(0, 0, coverage);
            out.end();
            putTile(layer, tx, ty, std::move(result));
        }
    m_changed = true;
    ++m_revision;
    emit changed(dirty);
}
void Session::fill(QPoint seed, int tolerance, bool sampleVisible) {
    if (activeLayer < 0 || m_document.layers[size_t(activeLayer)].locked ||
        m_document.layers[size_t(activeLayer)].kind != LayerKind::Raster ||
        !QRect(QPoint(), m_document.size).contains(seed))
        return;
    QImage image = renderLayer(m_document, activeLayer).convertToFormat(QImage::Format_ARGB32);
    QImage sample =
        sampleVisible ? renderDocument(m_document).convertToFormat(QImage::Format_ARGB32) : image;
    QRgb target = sample.pixel(seed), color = foreground.rgba();
    if (!sampleVisible && target == color)
        return;
    const int w = image.width(), h = image.height();
    QBitArray visited(w * h);
    std::vector<QPoint> pending{seed};
    auto matches = [&](int x, int y) {
        if (x < 0 || y < 0 || x >= w || y >= h || visited.testBit(y * w + x) ||
            (hasSelection && !selection.contains(QPointF(x + .5, y + .5))))
            return false;
        QRgb v = sample.pixel(x, y);
        return std::max({std::abs(qRed(v) - qRed(target)), std::abs(qGreen(v) - qGreen(target)),
                         std::abs(qBlue(v) - qBlue(target)),
                         std::abs(qAlpha(v) - qAlpha(target))}) <= tolerance;
    };
    while (!pending.empty()) {
        if (cancelRequested())
            return;
        QPoint p = pending.back();
        pending.pop_back();
        int x = p.x(), y = p.y();
        if (!matches(x, y))
            continue;
        int left = x, right = x;
        while (matches(left - 1, y))
            --left;
        while (matches(right + 1, y))
            ++right;
        bool up = false, down = false;
        for (x = left; x <= right; ++x) {
            visited.setBit(y * w + x);
            image.setPixel(x, y, color);
            bool u = matches(x, y - 1), d = matches(x, y + 1);
            if (u && !up)
                pending.emplace_back(x, y - 1);
            if (d && !down)
                pending.emplace_back(x, y + 1);
            up = u;
            down = d;
        }
    }
    apply(tr("Fill"), [&](Document& d) { replaceRaster(d.layers[size_t(activeLayer)], image); });
}
QImage Session::copyPixels(bool merged) const {
    if (hasSelection && selection.isEmpty())
        return {};
    QImage image;
    if (!merged && object()) {
        Document isolated = m_document;
        Layer layer;
        layer.kind = LayerKind::Vector;
        layer.objects.push_back(*object());
        isolated.layers = {layer};
        image = renderDocument(isolated);
        if (!hasSelection)
            return image.copy(object()->bounds().toAlignedRect().intersected(image.rect()));
    } else
        image = merged ? renderDocument(m_document) : renderLayer(m_document, activeLayer);
    if (hasSelection) {
        QRect bounds = selection.boundingRect().toAlignedRect().intersected(image.rect());
        if (bounds.isEmpty())
            return {};
        QImage mask(image.size(), QImage::Format_ARGB32_Premultiplied);
        mask.fill(Qt::transparent);
        QPainter p(&mask);
        p.fillPath(selection, Qt::white);
        p.end();
        QPainter out(&image);
        out.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        out.drawImage(0, 0, mask);
        out.end();
        image = image.copy(bounds);
    }
    return image;
}
void Session::clearPixels() {
    if (object()) {
        deleteObject();
        return;
    }
    if (m_document.layers[size_t(activeLayer)].locked)
        return;
    if (m_document.layers[size_t(activeLayer)].kind != LayerKind::Raster)
        return;
    apply(tr("Delete pixels"), [&](Document& d) {
        auto& l = d.layers[size_t(activeLayer)];
        if (!hasSelection) {
            l.tiles.clear();
            return;
        }
        for (auto it = l.tiles.begin(); it != l.tiles.end();) {
            QPoint xy = tilePosition(it.key());
            QImage tile = it.value();
            QPainter p(&tile);
            p.translate(-xy * TileSize);
            p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
            p.fillPath(selection, Qt::white);
            p.end();
            bool clear = true;
            for (int y = 0; y < TileSize && clear; ++y) {
                auto row = reinterpret_cast<const QRgb*>(tile.constScanLine(y));
                for (int x = 0; x < TileSize; ++x)
                    if (qAlpha(row[x])) {
                        clear = false;
                        break;
                    }
            }
            if (clear)
                it = l.tiles.erase(it);
            else {
                it.value() = tile;
                ++it;
            }
        }
    });
}
void Session::paste(const QImage& image) {
    if (image.isNull() || !validSize(image.size()))
        return;
    apply(tr("Paste"), [&](Document& d) {
        Layer l;
        l.name = tr("Pasted image");
        QImage clipped(d.size, QImage::Format_ARGB32_Premultiplied);
        clipped.fill(Qt::transparent);
        QPainter p(&clipped);
        p.drawImage(0, 0, image);
        p.end();
        replaceRaster(l, clipped);
        d.layers.push_back(std::move(l));
    });
    activeLayer = int(m_document.layers.size()) - 1;
    emit changed({});
}
void Session::transformActive(const QTransform& transform, bool selectedOnly) {
    if (activeLayer < 0 || m_document.layers[size_t(activeLayer)].locked)
        return;
    if (object()) {
        editObject(tr("Transform object"),
                   [&](Object& o) { o.transform = o.transform * transform; });
        return;
    }
    if (m_document.layers[size_t(activeLayer)].kind == LayerKind::Vector) {
        apply(tr("Transform layer"), [&](Document& d) {
            for (auto& o : d.layers[size_t(activeLayer)].objects)
                o.transform = o.transform * transform;
        });
        return;
    }
    QImage source = renderLayer(m_document, activeLayer), moving = source;
    if (selectedOnly && hasSelection) {
        QImage mask(source.size(), source.format());
        mask.fill(Qt::transparent);
        QPainter p(&mask);
        p.fillPath(selection, Qt::white);
        p.end();
        QPainter m(&moving);
        m.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        m.drawImage(0, 0, mask);
        m.end();
        QPainter erase(&source);
        erase.setCompositionMode(QPainter::CompositionMode_DestinationOut);
        erase.drawImage(0, 0, mask);
    } else
        source.fill(Qt::transparent);
    QPainter p(&source);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.setTransform(transform);
    p.drawImage(0, 0, moving);
    p.end();
    apply(tr("Transform pixels"),
          [&](Document& d) { replaceRaster(d.layers[size_t(activeLayer)], source); });
    if (selectedOnly && hasSelection) {
        selection = transform.map(selection);
        emit changed({});
    }
}
void Session::resize(QSize size, bool resample, QPoint offset) {
    if (!validSize(size)) {
        emit errorOccurred(
            tr("Dimensions must be positive, at most 8192 per side and 64 million pixels."));
        return;
    }
    QSize old = m_document.size;
    apply(resample ? tr("Resize image") : tr("Resize canvas"), [&](Document& d) {
        for (size_t i = 0; i < d.layers.size(); ++i) {
            auto& l = d.layers[i];
            if (l.kind == LayerKind::Raster) {
                QImage image = renderLayer(d, int(i));
                if (resample)
                    image = image.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                else {
                    QImage canvas(size, image.format());
                    canvas.fill(Qt::transparent);
                    QPainter p(&canvas);
                    p.drawImage(offset, image);
                    p.end();
                    image = canvas;
                }
                replaceRaster(l, image);
            } else {
                QTransform t;
                if (resample)
                    t.scale(double(size.width()) / old.width(),
                            double(size.height()) / old.height());
                else
                    t.translate(offset.x(), offset.y());
                for (auto& o : l.objects)
                    o.transform = o.transform * t;
            }
        }
        d.size = size;
    });
    deselect();
}
void Session::rasterize() {
    if (activeLayer < 0 || m_document.layers[size_t(activeLayer)].locked)
        return;
    QImage image = renderLayer(m_document, activeLayer);
    apply(tr("Rasterize layer"),
          [&](Document& d) { replaceRaster(d.layers[size_t(activeLayer)], image); });
    selectedObject = {};
}
void Session::addObject(Object o) {
    if (m_document.layers[size_t(activeLayer)].locked) {
        emit errorOccurred(tr("The layer is locked."));
        return;
    }
    int target = activeLayer;
    const bool createLayer = m_document.layers[size_t(activeLayer)].kind != LayerKind::Vector;
    if (createLayer)
        ++target;
    QUuid id = o.id;
    if (apply(tr("Create object"), [&](Document& d) {
            if (createLayer) {
                Layer layer;
                layer.kind = LayerKind::Vector;
                layer.name = tr("Vector layer");
                d.layers.insert(d.layers.begin() + target, std::move(layer));
            }
            d.layers[size_t(target)].objects.push_back(std::move(o));
        })) {
        activeLayer = target;
        selectedObject = id;
    }
    emit changed({});
}
const Object* Session::object() const {
    if (activeLayer < 0 || activeLayer >= int(m_document.layers.size()))
        return nullptr;
    for (const auto& o : m_document.layers[size_t(activeLayer)].objects)
        if (o.id == selectedObject)
            return &o;
    return nullptr;
}
void Session::editObject(const QString& label, const std::function<void(Object&)>& op) {
    if (!object() || m_document.layers[size_t(activeLayer)].locked)
        return;
    apply(label, [&](Document& d) {
        for (auto& o : d.layers[size_t(activeLayer)].objects)
            if (o.id == selectedObject) {
                op(o);
                break;
            }
    });
}
void Session::deleteObject() {
    if (!object() || m_document.layers[size_t(activeLayer)].locked)
        return;
    apply(tr("Delete object"), [&](Document& d) {
        auto& objects = d.layers[size_t(activeLayer)].objects;
        std::erase_if(objects, [&](const Object& o) { return o.id == selectedObject; });
    });
    selectedObject = {};
    emit changed({});
}
int Session::objectAt(QPointF point, double tolerance) const {
    if (activeLayer < 0)
        return -1;
    const auto& objects = m_document.layers[size_t(activeLayer)].objects;
    for (int i = int(objects.size()) - 1; i >= 0; --i) {
        const auto& o = objects[size_t(i)];
        QPainterPath path = o.transform.map(o.geometry());
        QPainterPathStroker stroker;
        stroker.setWidth(std::max(tolerance * 2, o.strokeWidth));
        if (path.contains(point) || stroker.createStroke(path).contains(point))
            return i;
    }
    return -1;
}
} // namespace Zyri
