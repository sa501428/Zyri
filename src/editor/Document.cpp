#include "Document.h"
#include <QColorSpace>
#include <QPainter>
#include <QPainterPathStroker>
#include <QSet>
#include <algorithm>
#include <cmath>

namespace Zyri {
bool validSize(QSize s) {
    return s.width() > 0 && s.height() > 0 && s.width() <= MaxDimension &&
           s.height() <= MaxDimension && qint64(s.width()) * s.height() <= MaxPixels;
}
QString validateDocument(const Document& d) {
    if (!validSize(d.size) || d.ppi < 1 || d.ppi > 2400 || d.layers.empty() ||
        d.layers.size() > 256 || d.bytes() > MaxDocumentBytes)
        return QStringLiteral(
            "Document exceeds its dimensions, resolution, layer, or 512 MiB data limit.");
    QSet<QUuid> ids;
    auto identity = [&](QUuid id) {
        if (id.isNull() || ids.contains(id))
            return false;
        ids.insert(id);
        return true;
    };
    auto bounded = [](double n, double low, double high) {
        return std::isfinite(n) && n >= low && n <= high;
    };
    if (!identity(d.id))
        return QStringLiteral("Invalid document identity.");
    size_t objects = 0;
    for (const auto& l : d.layers) {
        if (!identity(l.id) || l.name.size() > 4096 || !bounded(l.opacity, 0, 1) ||
            int(l.blend) < 0 || int(l.blend) > 3 || int(l.kind) < 0 || int(l.kind) > 1)
            return QStringLiteral("Invalid layer properties or identity.");
        if ((l.kind == LayerKind::Raster && !l.objects.empty()) ||
            (l.kind == LayerKind::Vector && !l.tiles.isEmpty()))
            return QStringLiteral("Layer content does not match its type.");
        for (auto it = l.tiles.cbegin(); it != l.tiles.cend(); ++it) {
            QPoint p = tilePosition(it.key());
            if (p.x() < 0 || p.y() < 0 || qint64(p.x()) * TileSize >= d.size.width() ||
                qint64(p.y()) * TileSize >= d.size.height() ||
                it.value().size() != QSize(TileSize, TileSize) ||
                it.value().format() != QImage::Format_ARGB32_Premultiplied)
                return QStringLiteral("Invalid tile coordinates or format.");
        }
        objects += l.objects.size();
        if (objects > 100000)
            return QStringLiteral("Scene exceeds 100000 objects.");
        for (const auto& o : l.objects) {
            auto t = o.transform;
            double font = o.font.pointSizeF() > 0 ? o.font.pointSizeF() : o.font.pixelSize() * .75;
            if (!identity(o.id) || int(o.kind) < 0 || int(o.kind) > 6 || !o.fill.isValid() ||
                !o.stroke.isValid() || !bounded(o.opacity, 0, 1) ||
                !bounded(o.strokeWidth, 0, 4096) || !bounded(font, 1, 512) ||
                o.path.elementCount() > 100000 || o.text.size() >= 1000000 ||
                !bounded(o.rect.x(), -1e7, 1e7) || !bounded(o.rect.y(), -1e7, 1e7) ||
                !bounded(o.rect.width(), 0, 1e7) || !bounded(o.rect.height(), 0, 1e7) ||
                !t.isAffine() || !t.isInvertible())
                return QStringLiteral("Invalid object geometry, style, or identity.");
            for (double n : {t.m11(), t.m12(), t.m21(), t.m22(), t.dx(), t.dy()})
                if (!bounded(n, -1e7, 1e7))
                    return QStringLiteral("Transform exceeds coordinate limits.");
            const auto bounds = o.bounds();
            for (double n : {bounds.left(), bounds.top(), bounds.right(), bounds.bottom()})
                if (!bounded(n, -1e7, 1e7))
                    return QStringLiteral("Transformed bounds exceed coordinate limits.");
            for (int i = 0; i < o.path.elementCount(); ++i) {
                auto e = o.path.elementAt(i);
                if (!bounded(e.x, -1e7, 1e7) || !bounded(e.y, -1e7, 1e7))
                    return QStringLiteral("Path exceeds coordinate limits.");
            }
        }
    }
    return {};
}
quint64 tileKey(int x, int y) {
    return (quint64(quint32(x)) << 32) | quint32(y);
}
QPoint tilePosition(quint64 k) {
    return {int(qint32(k >> 32)), int(qint32(k & 0xffffffff))};
}
QImage emptyTile() {
    QImage i(TileSize, TileSize, QImage::Format_ARGB32_Premultiplied);
    i.fill(Qt::transparent);
    return i;
}
QImage tileImage(const Layer& l, int x, int y) {
    return l.tiles.value(tileKey(x, y));
}
void putTile(Layer& l, int x, int y, QImage i) {
    bool empty = true;
    for (int row = 0; row < i.height() && empty; ++row) {
        const auto* p = reinterpret_cast<const QRgb*>(i.constScanLine(row));
        for (int col = 0; col < i.width(); ++col)
            if (qAlpha(p[col])) {
                empty = false;
                break;
            }
    }
    if (empty)
        l.tiles.remove(tileKey(x, y));
    else
        l.tiles.insert(tileKey(x, y), std::move(i));
}
qint64 Document::bytes() const {
    qint64 n = 0;
    for (const auto& l : layers) {
        n += 256 + l.name.size() * qint64(sizeof(QChar));
        for (const auto& object : l.objects)
            n += 512 + object.path.elementCount() * qint64(sizeof(QPainterPath::Element)) +
                 object.text.size() * qint64(sizeof(QChar));
        for (const auto& i : l.tiles)
            n += i.sizeInBytes();
    }
    return n;
}
QPainterPath Object::geometry() const {
    QPainterPath p;
    switch (kind) {
    case ObjectKind::Rectangle:
        p.addRect(rect);
        break;
    case ObjectKind::Ellipse:
        p.addEllipse(rect);
        break;
    case ObjectKind::RoundedRectangle:
        p.addRoundedRect(rect, 12, 12);
        break;
    case ObjectKind::Line:
        p.moveTo(rect.topLeft());
        p.lineTo(rect.bottomRight());
        break;
    case ObjectKind::Text:
        p.addRect(rect);
        break;
    case ObjectKind::Polygon:
    case ObjectKind::Path:
        p = path;
        break;
    }
    return p;
}
QRectF Object::bounds() const {
    return transform.mapRect(
        geometry().boundingRect().adjusted(-strokeWidth, -strokeWidth, strokeWidth, strokeWidth));
}
void paintObject(QPainter& p, const Object& o) {
    p.save();
    p.setTransform(o.transform, true);
    p.setOpacity(p.opacity() * o.opacity);
    p.setPen(QPen(o.stroke, o.strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    if (o.stroke.alpha() == 0 || o.strokeWidth <= 0)
        p.setPen(Qt::NoPen);
    p.setBrush(o.fill);
    if (o.kind == ObjectKind::Text) {
        QFont font = o.font;
        if (font.pointSizeF() > 0)
            font.setPixelSize(qRound(font.pointSizeF() * 4 / 3));
        p.setFont(font);
        p.setPen(o.fill);
        p.drawText(o.rect, o.alignment | Qt::TextWordWrap, o.text);
    } else
        p.drawPath(o.geometry());
    p.restore();
}
static QPainter::CompositionMode mode(Blend b) {
    switch (b) {
    case Blend::Multiply:
        return QPainter::CompositionMode_Multiply;
    case Blend::Screen:
        return QPainter::CompositionMode_Screen;
    case Blend::Overlay:
        return QPainter::CompositionMode_Overlay;
    default:
        return QPainter::CompositionMode_SourceOver;
    }
}
static QImage layerRegion(const Layer& l, QRect region) {
    QImage image(region.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter p(&image);
    p.setRenderHint(QPainter::Antialiasing);
    p.translate(-region.topLeft());
    if (l.kind == LayerKind::Raster) {
        const int x0 = std::max(0, region.left() / TileSize),
                  y0 = std::max(0, region.top() / TileSize);
        for (int y = y0; y <= region.bottom() / TileSize; ++y)
            for (int x = x0; x <= region.right() / TileSize; ++x) {
                auto it = l.tiles.constFind(tileKey(x, y));
                if (it != l.tiles.cend())
                    p.drawImage(QPoint(x * TileSize, y * TileSize), it.value());
            }
    } else
        for (const auto& o : l.objects)
            if (o.bounds().intersects(region))
                paintObject(p, o);
    return image;
}
QImage renderRegion(const Document& d, QRect region) {
    region = region.intersected(QRect(QPoint(), d.size));
    if (region.isEmpty())
        return {};
    QImage image(region.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter p(&image);
    for (const auto& l : d.layers)
        if (l.visible && l.opacity > 0) {
            p.setOpacity(l.opacity);
            p.setCompositionMode(mode(l.blend));
            p.drawImage(0, 0, layerRegion(l, region));
        }
    p.end();
    image.setColorSpace(QColorSpace::SRgb);
    return image;
}
QImage renderDocument(const Document& d, double scale) {
    if (!std::isfinite(scale) || scale <= 0 || scale > 8 || !validSize(d.size))
        return {};
    QSize size(qRound(d.size.width() * scale), qRound(d.size.height() * scale));
    if (!validSize(size))
        return {};
    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter p(&image);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.scale(scale, scale);
    // Bounded tiles keep compositor scratch independent of full document size.
    for (int y = 0; y < d.size.height(); y += TileSize)
        for (int x = 0; x < d.size.width(); x += TileSize)
            p.drawImage(QPoint(x, y), renderRegion(d, QRect(x, y, TileSize, TileSize)));
    p.end();
    image.setColorSpace(QColorSpace::SRgb);
    image.setDotsPerMeterX(qRound(d.ppi / 0.0254));
    image.setDotsPerMeterY(qRound(d.ppi / 0.0254));
    return image;
}
QImage renderLayer(const Document& d, int index) {
    if (index < 0 || index >= int(d.layers.size()))
        return {};
    return layerRegion(d.layers[size_t(index)], QRect(QPoint(), d.size));
}
void replaceRaster(Layer& l, const QImage& source) {
    l.kind = LayerKind::Raster;
    l.tiles.clear();
    l.objects.clear();
    QImage image = source.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    for (int y = 0; y < image.height(); y += TileSize)
        for (int x = 0; x < image.width(); x += TileSize) {
            QImage tile = emptyTile();
            QPainter p(&tile);
            p.drawImage(QPoint(), image, QRect(x, y, TileSize, TileSize));
            p.end();
            putTile(l, x / TileSize, y / TileSize, std::move(tile));
        }
}
QRgb composite(QRgb s, QRgb d, Blend b) {
    const double sa = qAlpha(s) / 255., da = qAlpha(d) / 255.;
    const double alpha = sa + da * (1 - sa);
    auto channel = [&](int sc, int dc) {
        double sp = sc / 255., dp = dc / 255.;
        if (b == Blend::Normal)
            return qRound(255 * (sp + dp * (1 - sa)));
        double cs = sa ? sp / sa : 0, cd = da ? dp / da : 0, mix = 0;
        if (b == Blend::Multiply)
            mix = cs * cd;
        else if (b == Blend::Screen)
            mix = cs + cd - cs * cd;
        else
            mix = cd <= .5 ? 2 * cs * cd : 1 - 2 * (1 - cs) * (1 - cd);
        return qRound(255 * ((1 - da) * sp + (1 - sa) * dp + sa * da * mix));
    };
    return qRgba(std::clamp(channel(qRed(s), qRed(d)), 0, 255),
                 std::clamp(channel(qGreen(s), qGreen(d)), 0, 255),
                 std::clamp(channel(qBlue(s), qBlue(d)), 0, 255), qRound(alpha * 255));
}
QTransform ViewTransform::matrix(QSizeF vp) const {
    QTransform t;
    t.translate(vp.width() / 2, vp.height() / 2);
    t.scale(zoom, zoom);
    t.rotate(rotation);
    t.translate(-center.x(), -center.y());
    return t;
}
QPointF ViewTransform::toDocument(QPointF p, QSizeF vp) const {
    return matrix(vp).inverted().map(p);
}
void ViewTransform::zoomAt(double factor, QPointF anchor, QSizeF vp) {
    if (!std::isfinite(factor) || factor <= 0)
        return;
    QPointF before = toDocument(anchor, vp);
    zoom = std::clamp(zoom * factor, .02, 64.);
    center += before - toDocument(anchor, vp);
}
} // namespace Zyri
