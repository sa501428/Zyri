#include <QtTest>
#include "../../src/engine/raster/FillEngine.h"

using namespace Zyri;

class TestFillEngine : public QObject
{
    Q_OBJECT
private slots:
    void fillsContiguousRegion()
    {
        QImage img(64, 64, QImage::Format_ARGB32_Premultiplied);
        img.fill(Qt::white);

        // Draw a red rectangle in the center.
        for (int y = 20; y < 44; ++y)
            for (int x = 20; x < 44; ++x)
                img.setPixel(x, y, QColor(Qt::red).rgba());

        // Fill the red interior with blue, seeded at center.
        FillEngine::fill(img, {32, 32}, Qt::blue, 0);

        QCOMPARE(img.pixelColor(32, 32), QColor(Qt::blue));
        // Outside the region should remain white.
        QCOMPARE(img.pixelColor(5, 5),   QColor(Qt::white));
    }

    void respectsTolerance()
    {
        QImage img(32, 32, QImage::Format_ARGB32_Premultiplied);
        // Fill with a near-white color.
        img.fill(QColor(250, 250, 250));

        // Fill starting at (0,0) with pure white using tolerance 10.
        // Near-white pixels (diff ~5) should be included.
        FillEngine::fill(img, {0, 0}, Qt::green, 10);

        QCOMPARE(img.pixelColor(16, 16), QColor(Qt::green));
    }
};

QTEST_MAIN(TestFillEngine)
#include "test_fill_engine.moc"
