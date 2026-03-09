#include <QtTest>
#include "../../src/engine/raster/BrushStamp.h"
#include "../../src/core/RasterLayer.h"

using namespace Zyri;

class TestPaintEngine : public QObject
{
    Q_OBJECT
private slots:
    void brushStampLeavesMarkOnTile()
    {
        QImage tile(kTileSize, kTileSize, QImage::Format_ARGB32_Premultiplied);
        tile.fill(Qt::transparent);

        BrushStamp::render(tile, {128, 128}, 20.0f, 1.0f, 1.0f, Qt::red);

        // The center pixel should now be red (or close to it).
        const QColor center = tile.pixelColor(128, 128);
        QVERIFY(center.red() > 200);
        QVERIFY(center.alpha() > 200);
    }

    void eraserStampRemovesAlpha()
    {
        QImage tile(kTileSize, kTileSize, QImage::Format_ARGB32_Premultiplied);
        tile.fill(Qt::red);

        BrushStamp::render(tile, {128, 128}, 20.0f, 1.0f, 1.0f, Qt::black, /*eraser=*/true);

        const QColor center = tile.pixelColor(128, 128);
        QVERIFY(center.alpha() < 50); // center should be mostly transparent
    }

    void softBrushHasFeatheredEdge()
    {
        QImage tile(kTileSize, kTileSize, QImage::Format_ARGB32_Premultiplied);
        tile.fill(Qt::transparent);

        BrushStamp::render(tile, {128, 128}, 30.0f, 0.0f, 1.0f, Qt::blue);

        // Center should be blue.
        QVERIFY(tile.pixelColor(128, 128).blue() > 100);
        // Edge (at radius) should be nearly transparent.
        QVERIFY(tile.pixelColor(128, 128 + 30).alpha() < 30);
    }
};

QTEST_MAIN(TestPaintEngine)
#include "test_paint_engine.moc"
