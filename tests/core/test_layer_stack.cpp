#include <QtTest>
#include "LayerStack.h"
#include "RasterLayer.h"

using namespace Zyri;

class TestLayerStack : public QObject
{
    Q_OBJECT
private slots:
    void addAndCount()
    {
        LayerStack stack;
        stack.addLayer(std::make_unique<RasterLayer>(QStringLiteral("A")));
        stack.addLayer(std::make_unique<RasterLayer>(QStringLiteral("B")));
        QCOMPARE(stack.count(), 2);
    }

    void removeLayer()
    {
        LayerStack stack;
        stack.addLayer(std::make_unique<RasterLayer>(QStringLiteral("A")));
        auto removed = stack.removeLayer(0);
        QCOMPARE(stack.count(), 0);
        QCOMPARE(removed->name(), QStringLiteral("A"));
    }

    void moveLayer()
    {
        LayerStack stack;
        stack.addLayer(std::make_unique<RasterLayer>(QStringLiteral("A")));
        stack.addLayer(std::make_unique<RasterLayer>(QStringLiteral("B")));
        stack.addLayer(std::make_unique<RasterLayer>(QStringLiteral("C")));
        stack.moveLayer(0, 2); // A moves to end
        QCOMPARE(stack.layerAt(0)->name(), QStringLiteral("B"));
        QCOMPARE(stack.layerAt(2)->name(), QStringLiteral("A"));
    }

    void layerById()
    {
        LayerStack stack;
        stack.addLayer(std::make_unique<RasterLayer>(QStringLiteral("X")));
        const QUuid id = stack.layerAt(0)->id();
        QVERIFY(stack.layerById(id) != nullptr);
        QCOMPARE(stack.layerById(id)->name(), QStringLiteral("X"));
        QCOMPARE(stack.indexOfId(id), 0);
    }

    void stackChangedSignal()
    {
        LayerStack stack;
        QSignalSpy spy(&stack, &LayerStack::stackChanged);
        stack.addLayer(std::make_unique<RasterLayer>());
        QCOMPARE(spy.count(), 1);
    }
};

QTEST_MAIN(TestLayerStack)
#include "test_layer_stack.moc"
