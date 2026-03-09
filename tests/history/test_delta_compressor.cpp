#include <QtTest>
#include "DeltaCompressor.h"
#include "../../src/core/RasterLayer.h"
#include <QImage>

using namespace Zyri;

class TestDeltaCompressor : public QObject
{
    Q_OBJECT
private slots:
    void roundTrip()
    {
        // Create a synthetic "before" tile (solid red).
        QImage before(kTileSize, kTileSize, QImage::Format_ARGB32_Premultiplied);
        before.fill(QColor(255, 0, 0, 255));
        QByteArray beforeRaw(reinterpret_cast<const char*>(before.constBits()),
                              before.sizeInBytes());

        // Create an "after" tile (mostly same, one pixel changed).
        QImage after = before.copy();
        after.setPixel(128, 128, QColor(0, 255, 0, 255).rgba());
        QByteArray afterRaw(reinterpret_cast<const char*>(after.constBits()),
                             after.sizeInBytes());

        // Compress delta and reconstruct.
        QByteArray delta       = DeltaCompressor::compress(beforeRaw, afterRaw);
        QByteArray reconstructed = DeltaCompressor::apply(beforeRaw, delta);

        QCOMPARE(reconstructed, afterRaw);
    }

    void compressionRatio()
    {
        // Sparse edit (1 pixel changed in a 256x256 tile) should compress well.
        QImage before(kTileSize, kTileSize, QImage::Format_ARGB32_Premultiplied);
        before.fill(Qt::transparent);
        QByteArray beforeRaw(reinterpret_cast<const char*>(before.constBits()),
                              before.sizeInBytes());

        QImage after = before.copy();
        after.setPixel(10, 10, QColor(Qt::red).rgba());
        QByteArray afterRaw(reinterpret_cast<const char*>(after.constBits()),
                             after.sizeInBytes());

        QByteArray delta = DeltaCompressor::compress(beforeRaw, afterRaw);

        // 256x256x4 = 262144 bytes raw; delta should be tiny.
        const int rawSize = kTileSize * kTileSize * 4;
        qDebug() << "Raw:" << rawSize << "Delta:" << delta.size()
                 << "Ratio:" << (100.0 * delta.size() / rawSize) << "%";
        QVERIFY(delta.size() < rawSize / 10); // at least 10x compression
    }

    void identicalBuffersYieldMinimalDelta()
    {
        QByteArray buf(kTileSize * kTileSize * 4, '\xAB');
        QByteArray delta = DeltaCompressor::compress(buf, buf);
        // XOR of identical buffers = all zeros → compresses to near-nothing.
        QVERIFY(delta.size() < 400);

        QByteArray result = DeltaCompressor::apply(buf, delta);
        QCOMPARE(result, buf);
    }
};

QTEST_MAIN(TestDeltaCompressor)
#include "test_delta_compressor.moc"
