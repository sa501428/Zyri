#pragma once
#include "Document.h"
#include <QString>

namespace Zyri {
struct LoadResult {
    Document document;
    QString error;
    QString warning;
    explicit operator bool() const {
        return error.isEmpty();
    }
};
[[nodiscard]] QString saveProject(const Document& document, const QString& path);
[[nodiscard]] QString saveRecoveryGeneration(const Document& document, const QString& path);
[[nodiscard]] LoadResult loadProject(const QString& path);
[[nodiscard]] LoadResult importImage(const QString& path, int page = 0, int ppi = 144);
[[nodiscard]] QString exportImage(const Document& document, const QString& path, double scale = 1,
                                  QColor matte = Qt::white, int quality = 92);
[[nodiscard]] QString exportSvg(const Document& document, const QString& path);
[[nodiscard]] LoadResult importSvg(const QString& path);
// Ephemeral history encoding, independent of the portable project container.
[[nodiscard]] QByteArray encodeState(const Document& document);
[[nodiscard]] LoadResult decodeState(const QByteArray& bytes);
} // namespace Zyri
