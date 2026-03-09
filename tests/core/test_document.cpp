#include <QtTest>
#include "Document.h"
#include "RasterLayer.h"
#include "LayerStack.h"

using namespace Zyri;

class TestDocument : public QObject
{
    Q_OBJECT
private slots:
    void defaultConstructor()
    {
        Document doc;
        QCOMPARE(doc.canvasSize(), QSize(1920, 1080));
        QCOMPARE(doc.dpi(), 96);
        QVERIFY(!doc.isDirty());
        QCOMPARE(doc.layers()->count(), 0);
    }

    void customConstructor()
    {
        Document doc(QSize(800, 600), 72, Qt::transparent);
        QCOMPARE(doc.canvasSize(), QSize(800, 600));
        QCOMPARE(doc.dpi(), 72);
    }

    void setCanvasSizeMarksDirty()
    {
        Document doc;
        doc.setCanvasSize(QSize(1024, 768));
        QVERIFY(doc.isDirty());
        QCOMPARE(doc.canvasSize(), QSize(1024, 768));
    }

    void addLayer()
    {
        Document doc;
        doc.layers()->addLayer(std::make_unique<RasterLayer>(QStringLiteral("BG")));
        QCOMPARE(doc.layers()->count(), 1);
        QCOMPARE(doc.layers()->layerAt(0)->name(), QStringLiteral("BG"));
    }
};

QTEST_MAIN(TestDocument)
#include "test_document.moc"
