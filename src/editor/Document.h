#pragma once
#include <QColor>
#include <QFont>
#include <QImage>
#include <QMap>
#include <QPainterPath>
#include <QTransform>
#include <QUuid>
#include <vector>
class QPainter;

namespace Zyri {
constexpr int TileSize = 256;
constexpr int MaxDimension = 8192;
constexpr qint64 MaxPixels = 64LL * 1024 * 1024;
constexpr qint64 MaxDocumentBytes = 512LL * 1024 * 1024;
enum class Blend { Normal, Multiply, Screen, Overlay };
enum class LayerKind { Raster, Vector };
enum class ObjectKind { Rectangle, Ellipse, RoundedRectangle, Line, Polygon, Path, Text };
struct Object {
    bool operator==(const Object&) const = default;
    QUuid id = QUuid::createUuid();
    ObjectKind kind = ObjectKind::Rectangle;
    QRectF rect;
    QPainterPath path;
    QTransform transform;
    QColor fill = QColor(70, 140, 240), stroke = Qt::black;
    double strokeWidth = 2, opacity = 1;
    QString text;
    QFont font = [] {
        QFont value;
        value.setPointSize(24);
        return value;
    }();
    int alignment = Qt::AlignLeft;
    [[nodiscard]] QPainterPath geometry() const;
    [[nodiscard]] QRectF bounds() const;
};
struct Layer {
    bool operator==(const Layer&) const = default;
    QUuid id = QUuid::createUuid();
    QString name = QStringLiteral("Layer");
    LayerKind kind = LayerKind::Raster;
    bool visible = true, locked = false;
    double opacity = 1;
    Blend blend = Blend::Normal;
    QMap<quint64, QImage> tiles;
    std::vector<Object> objects;
};
struct Document {
    bool operator==(const Document&) const = default;
    QUuid id = QUuid::createUuid();
    QSize size{1920, 1080};
    int ppi = 96;
    std::vector<Layer> layers;
    [[nodiscard]] qint64 bytes() const;
};
[[nodiscard]] bool validSize(QSize size);
[[nodiscard]] QString validateDocument(const Document& document);
[[nodiscard]] quint64 tileKey(int x, int y);
[[nodiscard]] QPoint tilePosition(quint64 key);
[[nodiscard]] QImage emptyTile();
[[nodiscard]] QImage tileImage(const Layer& layer, int x, int y);
void putTile(Layer& layer, int x, int y, QImage image);
void paintObject(QPainter& painter, const Object& object);
[[nodiscard]] QImage renderRegion(const Document& document, QRect region);
[[nodiscard]] QImage renderDocument(const Document& document, double scale = 1);
[[nodiscard]] QImage renderLayer(const Document& document, int index);
void replaceRaster(Layer& layer, const QImage& image);
[[nodiscard]] QRgb composite(QRgb source, QRgb destination, Blend blend);
struct ViewTransform {
    QPointF center;
    double zoom = 1, rotation = 0;
    [[nodiscard]] QTransform matrix(QSizeF viewport) const;
    [[nodiscard]] QPointF toDocument(QPointF point, QSizeF viewport) const;
    void zoomAt(double factor, QPointF anchor, QSizeF viewport);
};
} // namespace Zyri
