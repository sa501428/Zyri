#include "ProjectIO.h"
#include <QBuffer>
#include <QColorSpace>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QImageWriter>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QPdfWriter>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSvgRenderer>
#include <QTemporaryFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#ifdef ZYRI_PDF
#include <QPdfDocument>
#endif
#include <cmath>
#include <memory>
#include <stdexcept>
#include <zip.h>

namespace Zyri {
namespace {
using Blobs = QMap<QString, QByteArray>;
struct Failure {
    QString message;
};
void require(bool test, const QString& message) {
    if (!test)
        throw Failure{message};
}
QJsonArray rectangle(QRectF r) {
    return {r.x(), r.y(), r.width(), r.height()};
}
QJsonArray matrix(QTransform t) {
    return {t.m11(), t.m12(), t.m21(), t.m22(), t.dx(), t.dy()};
}
QString uuid(QUuid id) {
    return id.toString(QUuid::WithoutBraces);
}
QString tileName(const Layer& l, quint64 key) {
    QPoint p = tilePosition(key);
    return QStringLiteral("tiles/%1/%2_%3.rgba").arg(uuid(l.id)).arg(p.x()).arg(p.y());
}
QJsonObject manifest(const Document& d) {
    QJsonArray layers;
    for (const auto& l : d.layers) {
        QJsonArray tiles, objects;
        for (auto it = l.tiles.cbegin(); it != l.tiles.cend(); ++it) {
            QPoint p = tilePosition(it.key());
            tiles.append(QJsonArray{p.x(), p.y()});
        }
        for (const auto& o : l.objects) {
            QJsonArray path;
            for (int i = 0; i < o.path.elementCount(); ++i) {
                auto e = o.path.elementAt(i);
                path.append(QJsonArray{int(e.type), e.x, e.y});
            }
            objects.append(QJsonObject{{"id", uuid(o.id)},
                                       {"kind", int(o.kind)},
                                       {"rect", rectangle(o.rect)},
                                       {"path", path},
                                       {"fillRule", int(o.path.fillRule())},
                                       {"transform", matrix(o.transform)},
                                       {"fill", o.fill.name(QColor::HexArgb)},
                                       {"stroke", o.stroke.name(QColor::HexArgb)},
                                       {"strokeWidth", o.strokeWidth},
                                       {"opacity", o.opacity},
                                       {"text", o.text},
                                       {"fontFamily", o.font.family()},
                                       {"fontSize", o.font.pointSizeF() > 0
                                                        ? o.font.pointSizeF()
                                                        : double(o.font.pixelSize()) * .75},
                                       {"fontWeight", o.font.weight()},
                                       {"italic", o.font.italic()},
                                       {"underline", o.font.underline()},
                                       {"strikeOut", o.font.strikeOut()},
                                       {"fontStyle", o.font.styleName()},
                                       {"fontStretch", o.font.stretch()},
                                       {"alignment", o.alignment}});
        }
        layers.append(QJsonObject{{"id", uuid(l.id)},
                                  {"name", l.name},
                                  {"kind", int(l.kind)},
                                  {"visible", l.visible},
                                  {"locked", l.locked},
                                  {"opacity", l.opacity},
                                  {"blend", int(l.blend)},
                                  {"tiles", tiles},
                                  {"objects", objects}});
    }
    return {{"format", "Zyri"},          {"version", 1},
            {"id", uuid(d.id)},          {"width", d.size.width()},
            {"height", d.size.height()}, {"ppi", d.ppi},
            {"tileSize", TileSize},      {"pixelFormat", "RGBA8-premultiplied"},
            {"workingSpace", "sRGB"},    {"layers", layers}};
}
Blobs blobs(const Document& d) {
    Blobs entries;
    entries["manifest.json"] = QJsonDocument(manifest(d)).toJson(QJsonDocument::Compact);
    for (const auto& l : d.layers)
        for (auto it = l.tiles.cbegin(); it != l.tiles.cend(); ++it) {
            QImage rgba = it.value().convertToFormat(QImage::Format_RGBA8888_Premultiplied);
            entries[tileName(l, it.key())] =
                QByteArray(reinterpret_cast<const char*>(rgba.constBits()), rgba.sizeInBytes());
        }
    return entries;
}
double number(const QJsonValue& v, double low = -1e7, double high = 1e7) {
    require(v.isDouble() && std::isfinite(v.toDouble()) && v.toDouble() >= low &&
                v.toDouble() <= high,
            QStringLiteral("Invalid numeric project value."));
    return v.toDouble();
}
int integer(const QJsonValue& v, int low, int high) {
    double n = number(v, low, high);
    require(n == std::floor(n), QStringLiteral("Invalid integer project value."));
    return int(n);
}
QJsonArray array(const QJsonValue& v, int count) {
    require(v.isArray() && v.toArray().size() == count, QStringLiteral("Invalid project array."));
    return v.toArray();
}
Document parse(const Blobs& entries) {
    require(entries.contains("manifest.json") &&
                entries["manifest.json"].size() <= 16 * 1024 * 1024,
            QStringLiteral("Missing or oversized manifest."));
    QJsonParseError error;
    auto json = QJsonDocument::fromJson(entries["manifest.json"], &error);
    require(error.error == QJsonParseError::NoError && json.isObject(),
            QStringLiteral("Invalid project JSON."));
    auto m = json.object();
    require(m["format"] == "Zyri" && integer(m["version"], 1, 1) == 1,
            QStringLiteral("Unsupported project format/version."));
    require(m["workingSpace"] == "sRGB" && m["pixelFormat"] == "RGBA8-premultiplied" &&
                m["tileSize"] == TileSize,
            QStringLiteral("Unsupported pixel format."));
    Document d;
    d.size = {integer(m["width"], 1, MaxDimension), integer(m["height"], 1, MaxDimension)};
    require(validSize(d.size), QStringLiteral("Project dimensions exceed limits."));
    d.ppi = integer(m["ppi"], 1, 2400);
    QSet<QUuid> ids;
    auto id = [&](QJsonValue v) {
        QUuid u(v.toString());
        require(!u.isNull() && !ids.contains(u),
                QStringLiteral("Missing or duplicate project identity."));
        ids.insert(u);
        return u;
    };
    d.id = id(m["id"]);
    require(m["layers"].isArray(), QStringLiteral("Missing layers."));
    auto layers = m["layers"].toArray();
    require(!layers.isEmpty() && layers.size() <= 256, QStringLiteral("Invalid layer count."));
    int objectCount = 0;
    for (auto value : layers) {
        require(value.isObject(), QStringLiteral("Invalid layer."));
        auto j = value.toObject();
        Layer l;
        l.id = id(j["id"]);
        l.name = j["name"].toString().left(4096);
        l.kind = LayerKind(integer(j["kind"], 0, 1));
        l.visible = j["visible"].toBool(true);
        l.locked = j["locked"].toBool(false);
        l.opacity = number(j["opacity"], 0, 1);
        l.blend = Blend(integer(j["blend"], 0, 3));
        for (auto valueTile : j["tiles"].toArray()) {
            require(l.kind == LayerKind::Raster,
                    QStringLiteral("Vector layer contains raster data."));
            auto xy = array(valueTile, 2);
            int x = integer(xy[0], 0, (d.size.width() - 1) / TileSize),
                y = integer(xy[1], 0, (d.size.height() - 1) / TileSize);
            quint64 key = tileKey(x, y);
            require(!l.tiles.contains(key), QStringLiteral("Duplicate tile."));
            auto raw = entries.value(tileName(l, key));
            require(raw.size() == TileSize * TileSize * 4,
                    QStringLiteral("Missing or truncated tile."));
            const auto* bytes = reinterpret_cast<const uchar*>(raw.constData());
            for (int i = 0; i < raw.size(); i += 4)
                require(bytes[i] <= bytes[i + 3] && bytes[i + 1] <= bytes[i + 3] &&
                            bytes[i + 2] <= bytes[i + 3],
                        QStringLiteral("Invalid premultiplied pixel."));
            QImage tile(bytes, TileSize, TileSize, TileSize * 4,
                        QImage::Format_RGBA8888_Premultiplied);
            l.tiles[key] = tile.convertToFormat(QImage::Format_ARGB32_Premultiplied);
        }
        for (auto valueObject : j["objects"].toArray()) {
            require(l.kind == LayerKind::Vector && ++objectCount <= 100000,
                    QStringLiteral("Invalid vector content."));
            auto jo = valueObject.toObject();
            Object o;
            o.id = id(jo["id"]);
            o.kind = ObjectKind(integer(jo["kind"], 0, 6));
            auto r = array(jo["rect"], 4);
            o.rect = {number(r[0]), number(r[1]), number(r[2], 0), number(r[3], 0)};
            auto t = array(jo["transform"], 6);
            o.transform = QTransform(number(t[0]), number(t[1]), number(t[2]), number(t[3]),
                                     number(t[4]), number(t[5]));
            require(o.transform.isInvertible(), QStringLiteral("Singular object transform."));
            o.fill = QColor(jo["fill"].toString());
            o.stroke = QColor(jo["stroke"].toString());
            require(o.fill.isValid() && o.stroke.isValid(), QStringLiteral("Invalid color."));
            o.strokeWidth = number(jo["strokeWidth"], 0, 4096);
            o.opacity = number(jo["opacity"], 0, 1);
            o.text = jo["text"].toString();
            require(o.text.size() < 1000000, QStringLiteral("Oversized text."));
            o.font = QFont(jo["fontFamily"].toString());
            o.font.setPointSizeF(number(jo["fontSize"], 1, 512));
            o.font.setWeight(QFont::Weight(integer(jo["fontWeight"], 1, 1000)));
            o.font.setItalic(jo["italic"].toBool());
            o.font.setUnderline(jo["underline"].toBool());
            o.font.setStrikeOut(jo["strikeOut"].toBool());
            if (!jo["fontStyle"].toString().isEmpty())
                o.font.setStyleName(jo["fontStyle"].toString());
            if (jo.contains("fontStretch")) {
                int stretch = integer(jo["fontStretch"], 0, 4000);
                if (stretch > 0)
                    o.font.setStretch(stretch);
            }
            o.alignment = integer(jo["alignment"], 0, 255);
            auto path = jo["path"].toArray();
            require(path.size() <= 100000, QStringLiteral("Oversized path."));
            for (int i = 0; i < path.size(); ++i) {
                auto e = array(path[i], 3);
                int type = integer(e[0], 0, 3);
                double x = number(e[1]), y = number(e[2]);
                if (type == 0)
                    o.path.moveTo(x, y);
                else if (type == 1)
                    o.path.lineTo(x, y);
                else {
                    require(type == 2 && i + 2 < path.size(),
                            QStringLiteral("Invalid cubic path."));
                    auto b = array(path[++i], 3), c = array(path[++i], 3);
                    require(b[0] == 3 && c[0] == 3, QStringLiteral("Invalid cubic controls."));
                    o.path.cubicTo(x, y, number(b[1]), number(b[2]), number(c[1]), number(c[2]));
                }
            }
            if (jo.contains("fillRule")) {
                auto rule = Qt::FillRule(integer(jo["fillRule"], 0, 1));
                if (o.path.fillRule() != rule)
                    o.path.setFillRule(rule);
            }
            l.objects.push_back(std::move(o));
        }
        d.layers.push_back(std::move(l));
        require(d.bytes() <= MaxDocumentBytes,
                QStringLiteral("Project exceeds the 512 MiB document budget."));
    }
    QString validationError = validateDocument(d);
    require(validationError.isEmpty(), validationError);
    return d;
}
QString zipError(zip_t* z) {
    return QString::fromUtf8(zip_strerror(z));
}
} // namespace
QByteArray encodeState(const Document& d) {
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_8);
    stream << quint32(0x5a595231) << blobs(d);
    return bytes;
}
LoadResult decodeState(const QByteArray& bytes) {
    try {
        require(bytes.size() <= MaxDocumentBytes + 32 * 1024 * 1024,
                QStringLiteral("History payload exceeds limits."));
        QDataStream stream(bytes);
        stream.setVersion(QDataStream::Qt_6_8);
        quint32 magic;
        Blobs entries;
        stream >> magic >> entries;
        require(magic == 0x5a595231 && stream.status() == QDataStream::Ok && stream.atEnd(),
                QStringLiteral("Invalid history payload."));
        return {parse(entries), {}, {}};
    } catch (const Failure& e) {
        return {{}, e.message, {}};
    }
}
QString saveProject(const Document& d, const QString& path) {
    QString validationError = validateDocument(d);
    if (!validationError.isEmpty())
        return validationError;
    if (!validSize(d.size) || d.bytes() > MaxDocumentBytes)
        return QStringLiteral("Document exceeds supported limits.");
    QTemporaryFile temp(QFileInfo(path).absolutePath() + QStringLiteral("/.zyri-XXXXXX"));
    if (!temp.open())
        return temp.errorString();
    QString tempPath = temp.fileName();
    temp.close();
    int code = 0;
    zip_t* z = zip_open(QFile::encodeName(tempPath).constData(), ZIP_CREATE | ZIP_TRUNCATE, &code);
    if (!z)
        return QStringLiteral("Cannot create temporary project archive (%1).").arg(code);
    Blobs entries = blobs(d);
    for (auto it = entries.cbegin(); it != entries.cend(); ++it) {
        zip_source_t* source =
            zip_source_buffer(z, it.value().constData(), zip_uint64_t(it.value().size()), 0);
        if (!source ||
            zip_file_add(z, it.key().toUtf8().constData(), source, ZIP_FL_ENC_UTF_8) < 0) {
            if (source)
                zip_source_free(source);
            QString error = zipError(z);
            zip_discard(z);
            return error;
        }
    }
    if (zip_close(z) < 0) {
        QString error = zipError(z);
        zip_discard(z);
        return error;
    }
    QFile input(tempPath);
    if (!input.open(QIODevice::ReadOnly))
        return input.errorString();
    QSaveFile output(path);
    output.setDirectWriteFallback(false);
    if (!output.open(QIODevice::WriteOnly))
        return output.errorString();
    while (!input.atEnd()) {
        QByteArray block = input.read(1024 * 1024);
        if (block.isEmpty() && input.error() != QFileDevice::NoError)
            return input.errorString();
        if (output.write(block) != block.size())
            return output.errorString();
    }
    return output.commit() ? QString() : output.errorString();
}
LoadResult loadProject(const QString& path) {
    int code = 0;
    zip_t* raw = zip_open(QFile::encodeName(path).constData(), ZIP_RDONLY | ZIP_CHECKCONS, &code);
    if (!raw)
        return {{}, QStringLiteral("Cannot read project archive (%1).").arg(code), {}};
    std::unique_ptr<zip_t, decltype(&zip_discard)> archive(raw, zip_discard);
    try {
        Blobs entries;
        zip_int64_t count = zip_get_num_entries(raw, 0);
        require(count > 0 && count <= 100000, QStringLiteral("Invalid archive entry count."));
        quint64 expanded = 0;
        for (zip_uint64_t i = 0; i < zip_uint64_t(count); ++i) {
            zip_stat_t stat;
            require(zip_stat_index(raw, i, 0, &stat) == 0,
                    QStringLiteral("Cannot read archive directory."));
            QString name = QString::fromUtf8(stat.name);
            require(!name.startsWith('/') && !name.contains("..") && !name.contains('\\') &&
                        !entries.contains(name),
                    QStringLiteral("Unsafe or duplicate archive entry."));
            require(stat.encryption_method == ZIP_EM_NONE && stat.size <= 16 * 1024 * 1024,
                    QStringLiteral("Unsupported or oversized archive entry."));
            expanded += stat.size;
            require(expanded <= quint64(MaxDocumentBytes + 32 * 1024 * 1024),
                    QStringLiteral("Expanded project exceeds limits."));
            zip_file_t* file = zip_fopen_index(raw, i, 0);
            require(file, QStringLiteral("Cannot open archive entry."));
            QByteArray data(qsizetype(stat.size), '\0');
            zip_int64_t read = zip_fread(file, data.data(), stat.size);
            int closed = zip_fclose(file);
            require(read == zip_int64_t(stat.size) && closed == 0,
                    QStringLiteral("Corrupt or truncated project entry."));
            entries[name] = std::move(data);
        }
        return {parse(entries), {}, {}};
    } catch (const Failure& e) {
        return {{}, e.message, {}};
    }
}
QString saveRecoveryGeneration(const Document& document, const QString& path) {
    QString error = saveProject(document, path);
    if (!error.isEmpty())
        return error;
    auto verified = loadProject(path);
    if (!verified) {
        QFile::remove(path);
        return verified.error;
    }
    QDir dir = QFileInfo(path).dir();
    QString prefix = QFileInfo(path).fileName().section('_', 0, 0);
    auto files = dir.entryList({prefix + "_*.zyri"}, QDir::Files, QDir::Name | QDir::Reversed);
    for (int i = 2; i < files.size(); ++i)
        dir.remove(files[i]);
    return {};
}
LoadResult importImage(const QString& path, int page, int ppi) {
    if (QFileInfo(path).suffix().compare("zyri", Qt::CaseInsensitive) == 0)
        return loadProject(path);
    if (QFileInfo(path).suffix().compare("svg", Qt::CaseInsensitive) == 0)
        return importSvg(path);
    QImage image;
#ifdef ZYRI_PDF
    if (QFileInfo(path).suffix().compare("pdf", Qt::CaseInsensitive) == 0) {
        QPdfDocument pdf;
        if (pdf.load(path) != QPdfDocument::Error::None || page < 0 || page >= pdf.pageCount())
            return {{}, QStringLiteral("Cannot open PDF page."), {}};
        QSize size = (pdf.pagePointSize(page) * std::clamp(ppi, 36, 1200) / 72).toSize();
        if (!validSize(size))
            return {{}, QStringLiteral("PDF raster size exceeds limits."), {}};
        image = pdf.render(page, size);
    } else
#endif
    {
        QImageReader reader(path);
        reader.setAutoTransform(true);
        QSize size = reader.size();
        if (!validSize(size))
            return {{}, QStringLiteral("Image dimensions are invalid or exceed limits."), {}};
        image = reader.read();
        if (image.isNull())
            return {{}, reader.errorString(), {}};
    }
    if (!validSize(image.size()))
        return {{}, QStringLiteral("Invalid imported image."), {}};
    if (image.colorSpace().isValid())
        image.convertToColorSpace(QColorSpace::SRgb);
    else
        image.setColorSpace(QColorSpace::SRgb);
    Document d;
    d.size = image.size();
    Layer l;
    l.name = QFileInfo(path).completeBaseName();
    replaceRaster(l, image);
    d.layers.push_back(std::move(l));
    return {std::move(d), {}, {}};
}
QString exportImage(const Document& d, const QString& path, double scale, QColor matte,
                    int quality) {
    QString validationError = validateDocument(d);
    if (!validationError.isEmpty())
        return validationError;
    QString ext = QFileInfo(path).suffix().toLower();
    if (ext == "svg")
        return exportSvg(d, path);
    if (ext != "png" && ext != "jpg" && ext != "jpeg" && ext != "pdf")
        return QStringLiteral("Choose PNG, JPEG, SVG, or PDF.");
    QImage image = renderDocument(d, scale);
    if (image.isNull())
        return QStringLiteral("Export size exceeds supported limits.");
    QSaveFile file(path);
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly))
        return file.errorString();
    if (ext == "pdf") {
        {
            QPdfWriter pdf(&file);
            pdf.setResolution(d.ppi);
            pdf.setPageSize(
                QPageSize(QSizeF(d.size.width() * 25.4 / d.ppi, d.size.height() * 25.4 / d.ppi),
                          QPageSize::Millimeter));
            pdf.setPageMargins(QMarginsF(0, 0, 0, 0));
            QPainter p(&pdf);
            if (!p.isActive())
                return QStringLiteral("Cannot initialize PDF output.");
            bool flattened = false;
            for (const auto& layer : d.layers)
                if (layer.visible && layer.blend != Blend::Normal)
                    flattened = true;
            if (flattened)
                p.drawImage(QRect(0, 0, d.size.width(), d.size.height()), image);
            else {
                p.setRenderHint(QPainter::Antialiasing);
                for (size_t index = 0; index < d.layers.size(); ++index) {
                    const auto& layer = d.layers[index];
                    if (!layer.visible)
                        continue;
                    p.save();
                    p.setOpacity(layer.opacity);
                    if (layer.kind == LayerKind::Raster)
                        p.drawImage(QPoint(), renderLayer(d, int(index)));
                    else
                        for (const auto& object : layer.objects)
                            paintObject(p, object);
                    p.restore();
                }
            }
        }
        return file.commit() ? QString() : file.errorString();
    }
    if (ext != "png") {
        QImage flat(image.size(), QImage::Format_RGB32);
        flat.fill(matte);
        QPainter p(&flat);
        p.drawImage(0, 0, image);
        p.end();
        flat.setColorSpace(QColorSpace::SRgb);
        image = flat;
    }
    QImageWriter writer(&file, ext == "png" ? "png" : "jpeg");
    // Keep the PNG encoder's default compression; JPEG quality uses its own scale.
    if (ext != "png")
        writer.setQuality(std::clamp(quality, 1, 100));
    if (!writer.write(image))
        return writer.errorString();
    return file.commit() ? QString() : file.errorString();
}
namespace {
QString pathData(const QPainterPath& path) {
    QString out;
    for (int i = 0; i < path.elementCount(); ++i) {
        auto e = path.elementAt(i);
        if (e.type == QPainterPath::MoveToElement)
            out += QString("M%1 %2 ").arg(e.x, 0, 'g', 12).arg(e.y, 0, 'g', 12);
        else if (e.type == QPainterPath::LineToElement)
            out += QString("L%1 %2 ").arg(e.x, 0, 'g', 12).arg(e.y, 0, 'g', 12);
        else if (e.type == QPainterPath::CurveToElement && i + 2 < path.elementCount()) {
            auto b = path.elementAt(++i), c = path.elementAt(++i);
            out += QString("C%1 %2 %3 %4 %5 %6 ")
                       .arg(e.x, 0, 'g', 12)
                       .arg(e.y, 0, 'g', 12)
                       .arg(b.x, 0, 'g', 12)
                       .arg(b.y, 0, 'g', 12)
                       .arg(c.x, 0, 'g', 12)
                       .arg(c.y, 0, 'g', 12);
        }
    }
    return out;
}
QString transformData(QTransform t) {
    return QString("matrix(%1 %2 %3 %4 %5 %6)")
        .arg(t.m11(), 0, 'g', 12)
        .arg(t.m12(), 0, 'g', 12)
        .arg(t.m21(), 0, 'g', 12)
        .arg(t.m22(), 0, 'g', 12)
        .arg(t.dx(), 0, 'g', 12)
        .arg(t.dy(), 0, 'g', 12);
}
void svgImage(QXmlStreamWriter& w, const QImage& image) {
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    w.writeStartElement("image");
    w.writeAttribute("width", QString::number(image.width()));
    w.writeAttribute("height", QString::number(image.height()));
    w.writeAttribute("href", "data:image/png;base64," + QString::fromLatin1(png.toBase64()));
    w.writeEndElement();
}
QPainterPath readPath(const QString& text) {
    QStringList tokens;
    QRegularExpression regex(
        "[A-Za-z]|[-+]?(?:[0-9]*\\.[0-9]+|[0-9]+\\.?[0-9]*)(?:[eE][-+]?[0-9]+)?");
    auto matches = regex.globalMatch(text);
    while (matches.hasNext())
        tokens << matches.next().captured();
    require(tokens.size() <= 200000, QStringLiteral("Oversized SVG path."));
    int i = 0;
    QChar command;
    QPainterPath path;
    auto num = [&]() {
        require(i < tokens.size(), QStringLiteral("Incomplete SVG path."));
        bool ok;
        double v = tokens[i++].toDouble(&ok);
        require(ok && std::isfinite(v) && std::abs(v) < 1e7,
                QStringLiteral("Unsupported SVG path."));
        return v;
    };
    while (i < tokens.size()) {
        if (tokens[i].size() == 1 && tokens[i][0].isLetter())
            command = tokens[i++][0];
        require(!command.isNull(), QStringLiteral("Invalid SVG path."));
        bool relative = command.isLower();
        QChar c = command.toUpper();
        QPointF base = relative ? path.currentPosition() : QPointF();
        auto pos = [&]() {
            double x = num(), y = num();
            return QPointF(x, y) + base;
        };
        if (c == 'M') {
            path.moveTo(pos());
            command = relative ? 'l' : 'L';
        } else if (c == 'L')
            path.lineTo(pos());
        else if (c == 'H')
            path.lineTo(num() + (relative ? base.x() : 0), path.currentPosition().y());
        else if (c == 'V')
            path.lineTo(path.currentPosition().x(), num() + (relative ? base.y() : 0));
        else if (c == 'C') {
            QPointF a = pos(), b = pos(), end = pos();
            path.cubicTo(a, b, end);
        } else if (c == 'Q') {
            QPointF a = pos(), end = pos();
            path.quadTo(a, end);
        } else if (c == 'Z') {
            path.closeSubpath();
            command = {};
        } else
            throw Failure{QStringLiteral("Unsupported SVG path command.")};
    }
    return path;
}
QTransform readTransform(QString text) {
    QTransform result;
    QRegularExpression re("(matrix|translate|scale|rotate)\\s*\\(([^)]*)\\)");
    auto matches = re.globalMatch(text);
    QString consumed;
    while (matches.hasNext()) {
        auto match = matches.next();
        consumed += match.captured();
        QStringList values =
            match.captured(2).split(QRegularExpression("[ ,\\s]+"), Qt::SkipEmptyParts);
        QVector<double> v;
        for (auto s : values) {
            bool ok;
            double n = s.toDouble(&ok);
            require(ok && std::isfinite(n) && std::abs(n) < 1e7,
                    QStringLiteral("Invalid SVG transform."));
            v << n;
        }
        QTransform t;
        QString name = match.captured(1);
        if (name == "matrix" && v.size() == 6)
            t = QTransform(v[0], v[1], v[2], v[3], v[4], v[5]);
        else if (name == "translate" && (v.size() == 1 || v.size() == 2))
            t.translate(v[0], v.size() == 2 ? v[1] : 0);
        else if (name == "scale" && (v.size() == 1 || v.size() == 2))
            t.scale(v[0], v.size() == 2 ? v[1] : v[0]);
        else if (name == "rotate" && (v.size() == 1 || v.size() == 3)) {
            if (v.size() == 3)
                t.translate(v[1], v[2]);
            t.rotate(v[0]);
            if (v.size() == 3)
                t.translate(-v[1], -v[2]);
        } else
            throw Failure{QStringLiteral("Unsupported SVG transform.")};
        result = t * result;
    }
    text.remove(QRegularExpression("(matrix|translate|scale|rotate)\\s*\\([^)]*\\)"));
    require(text.trimmed().isEmpty(), QStringLiteral("Unsupported SVG transform."));
    return result;
}
} // namespace
QString exportSvg(const Document& d, const QString& path) {
    QString validationError = validateDocument(d);
    if (!validationError.isEmpty())
        return validationError;
    QSaveFile file(path);
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly))
        return file.errorString();
    QXmlStreamWriter w(&file);
    w.setAutoFormatting(true);
    w.writeStartDocument();
    w.writeStartElement("svg");
    w.writeDefaultNamespace("http://www.w3.org/2000/svg");
    w.writeAttribute("width", QString::number(d.size.width()));
    w.writeAttribute("height", QString::number(d.size.height()));
    w.writeAttribute("viewBox", QString("0 0 %1 %2").arg(d.size.width()).arg(d.size.height()));
    bool flatten = false;
    for (const auto& l : d.layers)
        if (l.visible && l.blend != Blend::Normal)
            flatten = true;
    if (flatten) {
        w.writeComment("Blend-mode composition flattened to preserve appearance");
        svgImage(w, renderDocument(d));
    } else
        for (size_t i = 0; i < d.layers.size(); ++i) {
            const auto& l = d.layers[i];
            if (!l.visible)
                continue;
            w.writeStartElement("g");
            w.writeAttribute("opacity", QString::number(l.opacity));
            if (l.kind == LayerKind::Raster)
                svgImage(w, renderLayer(d, int(i)));
            else
                for (const auto& o : l.objects) {
                    w.writeStartElement(o.kind == ObjectKind::Text ? "text" : "path");
                    w.writeAttribute("transform", transformData(o.transform));
                    w.writeAttribute("fill", o.fill.alpha() ? o.fill.name() : "none");
                    w.writeAttribute("fill-opacity", QString::number(o.fill.alphaF()));
                    w.writeAttribute("stroke", o.stroke.alpha() ? o.stroke.name() : "none");
                    w.writeAttribute("stroke-opacity", QString::number(o.stroke.alphaF()));
                    w.writeAttribute("stroke-width", QString::number(o.strokeWidth));
                    w.writeAttribute("opacity", QString::number(o.opacity));
                    if (o.kind == ObjectKind::Text) {
                        w.writeAttribute("x", QString::number(o.rect.x()));
                        w.writeAttribute("y",
                                         QString::number(o.rect.y() + o.font.pointSizeF() * 4 / 3));
                        w.writeAttribute("font-family", o.font.family());
                        w.writeAttribute("font-size", QString::number(o.font.pointSizeF() * 4 / 3));
                        w.writeCharacters(o.text);
                    } else
                        w.writeAttribute("d", pathData(o.geometry()));
                    w.writeEndElement();
                }
            w.writeEndElement();
        }
    w.writeEndElement();
    w.writeEndDocument();
    if (w.hasError())
        return QStringLiteral("SVG serialization failed.");
    return file.commit() ? QString() : file.errorString();
}
LoadResult importSvg(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() > 16 * 1024 * 1024)
        return {{}, QStringLiteral("SVG cannot be read or exceeds 16 MiB."), {}};
    QByteArray data = file.readAll();
    // Reject active/external references before either editable or raster import.
    QXmlStreamReader check(data);
    while (!check.atEnd()) {
        check.readNext();
        if (check.tokenType() == QXmlStreamReader::DTD ||
            check.tokenType() == QXmlStreamReader::EntityReference)
            return {{}, QStringLiteral("SVG entities are unsupported."), {}};
        if (check.isStartElement()) {
            if (check.name() == "script" || check.name() == "foreignObject")
                return {{}, QStringLiteral("Active SVG content is unsupported."), {}};
            for (auto a : check.attributes())
                if ((a.name() == "href" && !a.value().startsWith(QLatin1String("data:image/")) &&
                     !a.value().startsWith('#')) ||
                    a.value().contains(QLatin1String("url(")) ||
                    a.name().startsWith(QLatin1String("on")))
                    return {{},
                            QStringLiteral("External or active SVG references are unsupported."),
                            {}};
        }
    }
    if (check.hasError())
        return {{}, check.errorString(), {}};
    try {
        Document d;
        QXmlStreamReader xml(data);
        QVector<QTransform> transforms{QTransform()};
        QVector<double> opacities{1};
        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.isEndElement() && xml.name() == "g") {
                require(transforms.size() > 1, QStringLiteral("Invalid SVG group."));
                transforms.pop_back();
                opacities.pop_back();
            }
            if (!xml.isStartElement())
                continue;
            QString tag = xml.name().toString();
            auto a = xml.attributes();
            auto value = [&](const char* name, double fallback = 0) {
                if (!a.hasAttribute(QLatin1String(name)))
                    return fallback;
                QString s = a.value(QLatin1String(name)).toString();
                s.remove(QRegularExpression("px$"));
                bool ok;
                double n = s.toDouble(&ok);
                require(ok && std::isfinite(n) && std::abs(n) < 1e7,
                        QStringLiteral("Unsupported SVG dimensions."));
                return n;
            };
            if (tag == "svg") {
                d.size = QSize(qRound(value("width", 512)), qRound(value("height", 512)));
                require(validSize(d.size), QStringLiteral("SVG dimensions exceed limits."));
                if (a.hasAttribute("viewBox")) {
                    QStringList box = a.value("viewBox").toString().split(
                        QRegularExpression("[ ,\\s]+"), Qt::SkipEmptyParts);
                    require(box.size() == 4, QStringLiteral("Invalid SVG viewBox."));
                    double w = box[2].toDouble(), h = box[3].toDouble();
                    require(w > 0 && h > 0, QStringLiteral("Invalid SVG viewBox."));
                    transforms[0].scale(d.size.width() / w, d.size.height() / h);
                    transforms[0].translate(-box[0].toDouble(), -box[1].toDouble());
                }
                continue;
            }
            if (tag == "title" || tag == "desc") {
                xml.skipCurrentElement();
                continue;
            }
            if (tag == "g") {
                require(transforms.size() < 64, QStringLiteral("SVG nesting exceeds limits."));
                require(!a.hasAttribute("fill") && !a.hasAttribute("stroke") &&
                            !a.hasAttribute("style"),
                        QStringLiteral("Inherited SVG styles require raster import."));
                transforms << readTransform(a.value("transform").toString()) * transforms.back();
                opacities << std::clamp(value("opacity", 1), 0., 1.) * opacities.back();
                continue;
            }
            require(!a.hasAttribute("style") && !a.hasAttribute("class"),
                    QStringLiteral("CSS SVG styles require raster import."));
            if (tag == "image") {
                QString href = a.value("href").toString();
                require(href.startsWith("data:image/png;base64,"),
                        QStringLiteral("Unsupported SVG image."));
                QImage image =
                    QImage::fromData(QByteArray::fromBase64(href.mid(22).toLatin1()), "PNG");
                require(validSize(image.size()), QStringLiteral("Invalid SVG image."));
                QImage canvas(d.size, QImage::Format_ARGB32_Premultiplied);
                canvas.fill(Qt::transparent);
                QPainter p(&canvas);
                p.setTransform(transforms.back());
                p.drawImage(QRectF(value("x"), value("y"), value("width", image.width()),
                                   value("height", image.height())),
                            image);
                p.end();
                Layer l;
                l.opacity = opacities.back();
                l.name = "SVG image";
                replaceRaster(l, canvas);
                d.layers.push_back(std::move(l));
                continue;
            }
            Object o;
            o.fill = QColor(a.value("fill").isEmpty() ? QStringLiteral("black")
                                                      : a.value("fill").toString());
            if (a.value("fill") == "none")
                o.fill = Qt::transparent;
            o.stroke = QColor(a.value("stroke").toString());
            if (!o.stroke.isValid())
                o.stroke = Qt::transparent;
            o.fill.setAlphaF(o.fill.alphaF() * std::clamp(value("fill-opacity", 1), 0., 1.));
            o.stroke.setAlphaF(o.stroke.alphaF() * std::clamp(value("stroke-opacity", 1), 0., 1.));
            o.opacity = std::clamp(value("opacity", 1), 0., 1.) * opacities.back();
            o.strokeWidth = std::clamp(value("stroke-width", 1), 0., 4096.);
            o.transform = readTransform(a.value("transform").toString()) * transforms.back();
            if (tag == "rect") {
                o.kind =
                    a.hasAttribute("rx") ? ObjectKind::RoundedRectangle : ObjectKind::Rectangle;
                o.rect = {value("x"), value("y"), value("width"), value("height")};
            } else if (tag == "ellipse" || tag == "circle") {
                o.kind = ObjectKind::Ellipse;
                double rx = value(tag == "circle" ? "r" : "rx"),
                       ry = value(tag == "circle" ? "r" : "ry");
                o.rect = {value("cx") - rx, value("cy") - ry, rx * 2, ry * 2};
            } else if (tag == "line") {
                o.kind = ObjectKind::Path;
                o.path.moveTo(value("x1"), value("y1"));
                o.path.lineTo(value("x2"), value("y2"));
            } else if (tag == "path") {
                o.kind = ObjectKind::Path;
                o.path = readPath(a.value("d").toString());
            } else if (tag == "polygon" || tag == "polyline") {
                o.kind = ObjectKind::Path;
                QStringList points = a.value("points").toString().split(
                    QRegularExpression("[ ,\\s]+"), Qt::SkipEmptyParts);
                require(points.size() % 2 == 0, QStringLiteral("Invalid polygon."));
                for (int i = 0; i < points.size(); i += 2) {
                    QPointF pt(points[i].toDouble(), points[i + 1].toDouble());
                    if (i == 0)
                        o.path.moveTo(pt);
                    else
                        o.path.lineTo(pt);
                }
                if (tag == "polygon")
                    o.path.closeSubpath();
            } else if (tag == "text") {
                o.kind = ObjectKind::Text;
                o.font.setFamily(a.value("font-family").isEmpty()
                                     ? QStringLiteral("Sans Serif")
                                     : a.value("font-family").toString());
                o.font.setPointSizeF(std::clamp(value("font-size", 24) * .75, 1., 512.));
                o.rect = {value("x"), value("y") - o.font.pointSizeF() * 4 / 3,
                          double(d.size.width()), double(d.size.height())};
                o.text = xml.readElementText(QXmlStreamReader::ErrorOnUnexpectedElement);
                require(!xml.hasError(),
                        QStringLiteral("Complex SVG text requires raster import."));
            } else
                throw Failure{QStringLiteral("Unsupported SVG element: ") + tag};
            if (d.layers.empty() || d.layers.back().kind != LayerKind::Vector) {
                Layer l;
                l.kind = LayerKind::Vector;
                l.name = "SVG objects";
                d.layers.push_back(std::move(l));
            }
            d.layers.back().objects.push_back(std::move(o));
            require(d.layers.size() <= 256 && d.layers.back().objects.size() <= 100000,
                    QStringLiteral("SVG scene exceeds limits."));
        }
        require(!xml.hasError(), xml.errorString());
        if (d.layers.empty())
            d.layers.emplace_back();
        QString validationError = validateDocument(d);
        require(validationError.isEmpty(), validationError);
        return {std::move(d), {}, {}};
    } catch (const Failure& e) {
        QSvgRenderer renderer(data);
        if (!renderer.isValid() || !validSize(renderer.defaultSize()))
            return {{}, e.message, {}};
        QImage image(renderer.defaultSize(), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter p(&image);
        renderer.render(&p);
        p.end();
        Document d;
        d.size = image.size();
        Layer l;
        l.name = "Rasterized SVG";
        replaceRaster(l, image);
        d.layers.push_back(std::move(l));
        return {std::move(d),
                {},
                e.message +
                    QStringLiteral(
                        " The SVG was rendered as a raster image; objects are not editable.")};
    }
}
} // namespace Zyri
