#include "editor/Application.h"
#include "editor/Canvas.h"
#include "editor/Document.h"
#include "editor/MainWindow.h"
#include "editor/ProjectIO.h"
#include "editor/Session.h"
#include <QAction>
#include <QAbstractButton>
#include <QMessageBox>
#include <QScopedValueRollback>
#include <QColorSpace>
#include <QDataStream>
#include <QFile>
#include <QImageReader>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLayout>
#include <QPainter>
#include <QRandomGenerator>
#include <QStatusBar>
#include <QTemporaryDir>
#include <QTimer>
#include <QToolBar>
#include <QtTest>
using namespace Zyri;
class TestEditor : public QObject {
    Q_OBJECT
    static Document blank(QSize size = {512, 512}) {
        Document d;
        d.size = size;
        d.layers.emplace_back();
        return d;
    }
  private slots:
    void quitWaitsForDecision_data() {
        QTest::addColumn<int>("choice");
        QTest::newRow("cancel") << int(QMessageBox::Cancel);
        QTest::newRow("discard") << int(QMessageBox::Discard);
        QTest::newRow("save") << int(QMessageBox::Save);
    }
    void quitWaitsForDecision() {
        QFETCH(int, choice);
        QTemporaryDir temp;
        MainWindow window(temp.path(), false);
        const QString path = temp.filePath("saved.zyri");
        auto session = window.addDocument(blank(), path, true)->session();
        session->setLayerProperty("name", [](Layer& l) { l.name = "edited"; });
        auto app = static_cast<Application*>(qApp);
        QScopedValueRollback<std::function<void()>> callbackGuard(
            app->requestClose, [&window] { window.close(); });
        QSignalSpy accepted(&window, &MainWindow::closeAccepted);
        window.show();
        bool sawPrompt = false;
        bool stayedOpen = false;
        bool repeatedQuitWasBlocked = false;
        QTimer::singleShot(0, &window, [&] {
            auto box = window.findChild<QMessageBox*>();
            if (!box)
                return;
            sawPrompt = true;
            stayedOpen = window.isVisible() && accepted.isEmpty();
            // A second native Quit while the modal question is open must not
            // close the window, recurse into another question, or quit the app.
            QEvent repeatedQuit(QEvent::Quit);
            QCoreApplication::sendEvent(app, &repeatedQuit);
            repeatedQuitWasBlocked = !repeatedQuit.isAccepted() && accepted.isEmpty() &&
                                     window.findChildren<QMessageBox*>().size() == 1;
            box->button(QMessageBox::StandardButton(choice))->click();
        });
        QEvent nativeQuit(QEvent::Quit);
        QCoreApplication::sendEvent(app, &nativeQuit);
        QVERIFY(!nativeQuit.isAccepted());
        QVERIFY(sawPrompt);
        QVERIFY(stayedOpen);
        QVERIFY(repeatedQuitWasBlocked);
        if (choice == QMessageBox::Cancel) {
            QVERIFY(window.isVisible());
            QVERIFY(session->dirty());
            QVERIFY(accepted.isEmpty());
        } else {
            if (choice == QMessageBox::Save) {
                // Saving is asynchronous; closing must wait for it to finish.
                QVERIFY(window.isVisible());
                QVERIFY(accepted.isEmpty());
            }
            QTRY_COMPARE_WITH_TIMEOUT(accepted.count(), 1, 10000);
            QVERIFY(!window.isVisible());
            if (choice == QMessageBox::Save) {
                auto result = loadProject(path);
                QVERIFY2(result.error.isEmpty(), qPrintable(result.error));
                QCOMPARE(result.document.layers[0].name, QString("edited"));
            }
        }
    }
    void failedSaveCancelsQuit() {
        QTemporaryDir temp;
        MainWindow window(temp.path(), false);
        // The missing parent directory makes saving fail without permissions
        // assumptions or touching any user files.
        auto session = window.addDocument(blank(), temp.filePath("missing/file.zyri"), true)
                           ->session();
        session->setLayerProperty("name", [](Layer& l) { l.name = "edited"; });
        auto app = static_cast<Application*>(qApp);
        QScopedValueRollback<std::function<void()>> callbackGuard(
            app->requestClose, [&window] { window.close(); });
        QSignalSpy accepted(&window, &MainWindow::closeAccepted);
        window.show();
        bool failureShown = false;
        bool quitBlocked = false;
        QTimer dialogs;
        dialogs.setInterval(1);
        connect(&dialogs, &QTimer::timeout, &window, [&] {
            auto box = window.findChild<QMessageBox*>();
            if (!box || !box->isVisible())
                return;
            if (box->standardButtons().testFlag(QMessageBox::Save)) {
                box->button(QMessageBox::Save)->click();
            } else {
                failureShown = true;
                QEvent quit(QEvent::Quit);
                QCoreApplication::sendEvent(app, &quit);
                quitBlocked = !quit.isAccepted() && accepted.isEmpty() &&
                              window.findChildren<QMessageBox*>().size() == 1;
                box->button(QMessageBox::Ok)->click();
            }
        });
        dialogs.start();
        QEvent quit(QEvent::Quit);
        QCoreApplication::sendEvent(app, &quit);
        QTRY_VERIFY_WITH_TIMEOUT(failureShown, 10000);
        dialogs.stop();
        QVERIFY(quitBlocked);
        QVERIFY(window.isVisible());
        QVERIFY(session->dirty());
        QVERIFY(accepted.isEmpty());
    }
    void windowCloseCanBeCancelled() {
        QTemporaryDir temp;
        MainWindow window(temp.path(), false);
        auto session = window.addDocument(blank())->session();
        session->setLayerProperty("name", [](Layer& l) { l.name = "edited"; });
        QSignalSpy accepted(&window, &MainWindow::closeAccepted);
        window.show();
        QTimer::singleShot(0, &window, [&window] {
            if (auto box = window.findChild<QMessageBox*>())
                box->button(QMessageBox::Cancel)->click();
        });
        QVERIFY(!window.close());
        QVERIFY(window.isVisible());
        QVERIFY(accepted.isEmpty());
    }
    void dimensions() {
        QVERIFY(validSize({1, 1}));
        QVERIFY(!validSize({0, 200}));
        QVERIFY(!validSize({9000, 1}));
        QVERIFY(validSize({8192, 8192}));
    }
    void blending() {
        QCOMPARE(composite(0x80800000, 0xff0000ff, Blend::Normal), QRgb(0xff80007f));
        for (auto b : {Blend::Normal, Blend::Multiply, Blend::Screen, Blend::Overlay}) {
            QCOMPARE(composite(0xffff0000, 0, b), QRgb(0xffff0000));
            QCOMPARE(composite(0, 0xff1267ab, b), QRgb(0xff1267ab));
        }
    }
    void rendererLayers() {
        auto d = blank({20, 20});
        QImage red = emptyTile();
        red.fill(Qt::red);
        d.layers[0].tiles[tileKey(0, 0)] = red;
        Layer blue;
        QImage pixel = emptyTile();
        pixel.fill(QColor(0, 0, 255, 128));
        blue.tiles[tileKey(0, 0)] = pixel;
        d.layers.push_back(blue);
        QColor result = renderDocument(d).pixelColor(4, 4);
        QVERIFY(std::abs(result.red() - 127) <= 1);
        QCOMPARE(result.blue(), 128);
        d.layers[1].visible = false;
        QCOMPARE(renderDocument(d).pixelColor(4, 4), QColor(Qt::red));
    }
    void renderBlendParity() {
        for (auto blend : {Blend::Normal, Blend::Multiply, Blend::Screen, Blend::Overlay}) {
            auto d = blank({1, 1});
            QImage a = emptyTile(), b = emptyTile();
            a.fill(QColor(40, 90, 180, 170));
            b.fill(QColor(220, 60, 80, 130));
            d.layers[0].tiles[tileKey(0, 0)] = a;
            Layer l;
            l.tiles[tileKey(0, 0)] = b;
            l.blend = blend;
            d.layers.push_back(l);
            QRgb expected = composite(b.pixel(0, 0), a.pixel(0, 0), blend),
                 actual = renderDocument(d).pixel(0, 0);
            QVERIFY(std::abs(qRed(actual) - qRed(expected)) <= 2);
            QVERIFY(std::abs(qGreen(actual) - qGreen(expected)) <= 2);
            QVERIFY(std::abs(qBlue(actual) - qBlue(expected)) <= 2);
        }
    }
    void tileSnapshot() {
        auto d = blank();
        QImage tile = emptyTile();
        tile.setPixelColor(0, 0, Qt::red);
        d.layers[0].tiles[tileKey(0, 0)] = tile;
        Document snapshot = d;
        d.layers[0].tiles[tileKey(0, 0)].setPixelColor(0, 0, Qt::blue);
        QCOMPARE(snapshot.layers[0].tiles[tileKey(0, 0)].pixelColor(0, 0), QColor(Qt::red));
    }
    void transactionCancel() {
        Session s(blank());
        auto state = s.stateId();
        QVERIFY(s.begin("rename"));
        s.mutate([](Document& d) { d.layers[0].name = "test"; });
        s.cancel();
        QCOMPARE(s.stateId(), state);
        QCOMPARE(s.document().layers[0].name, QString("Layer"));
        QVERIFY(!s.dirty());
        QVERIFY(!s.canUndo());
    }
    void dirtyAndBranches() {
        Session s(blank());
        QUuid initial = s.stateId();
        s.setLayerProperty("rename", [](Layer& l) { l.name = "A"; });
        QUuid saved = s.stateId();
        s.markSaved(saved);
        s.setLayerProperty("opacity", [](Layer& l) { l.opacity = .5; });
        QVERIFY(s.dirty());
        s.undo();
        QVERIFY(!s.dirty());
        s.undo();
        QCOMPARE(s.stateId(), initial);
        s.setLayerProperty("branch", [](Layer& l) { l.name = "B"; });
        QVERIFY(!s.canRedo());
        QVERIFY(s.dirty());
    }
    void staleSaveMarker() {
        Session s(blank());
        auto snapshot = s.stateId();
        s.setLayerProperty("rename", [](Layer& l) { l.name = "Edited"; });
        s.markSaved(snapshot);
        QVERIFY(s.dirty());
        s.undo();
        QVERIFY(!s.dirty());
    }
    void continuousStroke() {
        Session s(blank());
        s.foreground = Qt::red;
        s.brush.size = 8;
        s.brush.hardness = 1;
        s.brush.pressure = false;
        s.beginStroke({10, 250});
        s.strokeTo({490, 250});
        s.endStroke();
        QImage image = renderDocument(s.document());
        for (int x = 10; x < 490; ++x)
            QVERIFY(image.pixelColor(x, 250).alpha() > 200);
        QVERIFY(s.document().layers[0].tiles.size() >= 2);
        QVERIFY(s.dirty());
        s.undo();
        QVERIFY(s.document().layers[0].tiles.isEmpty());
        s.redo();
        QCOMPARE(renderDocument(s.document()), image);
    }
    void strokeCancel() {
        Session s(blank());
        s.beginStroke({30, 30});
        s.strokeTo({300, 300});
        s.cancel();
        QVERIFY(s.document().layers[0].tiles.isEmpty());
        QVERIFY(!s.canUndo());
    }
    void strokeOpacity() {
        Session s(blank());
        s.brush.size = 40;
        s.brush.hardness = 1;
        s.brush.opacity = .5;
        s.brush.flow = 1;
        s.brush.pressure = false;
        s.beginStroke({20, 20});
        for (int i = 0; i < 10; ++i)
            s.strokeTo({22. + i, 20});
        s.endStroke();
        QVERIFY(renderDocument(s.document()).pixelColor(25, 20).alpha() <= 128);
    }
    void boundsAndLocks() {
        Session s(blank({30, 30}));
        s.brush.size = 100;
        s.beginStroke({0, 0});
        s.strokeTo({-1000, -1000});
        s.endStroke();
        QCOMPARE(s.document().layers[0].tiles.size(), 1);
        auto tile = s.document().layers[0].tiles.first();
        QCOMPARE(tile.pixelColor(40, 40).alpha(), 0);
        s.setLayerProperty("lock", [](Layer& l) { l.locked = true; });
        QVERIFY(!s.beginStroke({10, 10}));
    }
    void eraser() {
        Session s(blank());
        s.brush.hardness = 1;
        s.beginStroke({20, 20});
        s.endStroke();
        s.beginStroke({20, 20}, 1, true);
        s.endStroke();
        QCOMPARE(renderDocument(s.document()).pixelColor(20, 20).alpha(), 0);
        s.undo();
        QVERIFY(renderDocument(s.document()).pixelColor(20, 20).alpha() > 0);
    }
    void selections() {
        Session s(blank());
        QPainterPath p;
        p.addRect(0, 0, 50, 50);
        s.select(p);
        QVERIFY(!s.dirty());
        s.brush.size = 40;
        s.brush.hardness = 1;
        s.beginStroke({49, 25});
        s.endStroke();
        auto image = renderDocument(s.document());
        QCOMPARE(image.pixelColor(60, 25).alpha(), 0);
        QVERIFY(image.pixelColor(40, 25).alpha() > 0);
        s.undo();
        s.select(QPainterPath());
        s.beginStroke({20, 20});
        s.endStroke();
        QVERIFY(s.document().layers[0].tiles.isEmpty());
        s.deselect();
    }
    void fillTolerance() {
        Session s(blank({32, 32}));
        QImage image(32, 32, QImage::Format_ARGB32_Premultiplied);
        image.fill(QColor(250, 250, 250));
        image.setPixelColor(1, 1, QColor(245, 245, 245));
        s.apply("image", [&](Document& d) { replaceRaster(d.layers[0], image); });
        s.foreground = Qt::green;
        s.fill({0, 0}, 4);
        QCOMPARE(renderDocument(s.document()).pixelColor(1, 1), QColor(245, 245, 245));
        s.undo();
        s.fill({0, 0}, 5);
        QCOMPARE(renderDocument(s.document()).pixelColor(1, 1), QColor(Qt::green));
    }
    void layerLifecycle() {
        Session s(blank());
        QUuid id = s.document().layers[0].id;
        s.beginStroke({20, 20});
        s.endStroke();
        QImage image = renderDocument(s.document());
        s.addLayer(LayerKind::Raster);
        s.activeLayer = 0;
        s.deleteLayer();
        s.undo();
        QCOMPARE(s.document().layers[0].id, id);
        s.undo();
        s.undo();
        QVERIFY(s.document().layers[0].tiles.isEmpty());
        s.redo();
        QCOMPARE(renderDocument(s.document()), image);
    }
    void historySpill() {
        Session s(blank());
        s.historyBudget = 0;
        for (int i = 0; i < 5; ++i) {
            s.beginStroke({20. + i * 30, 20});
            s.endStroke();
        }
        auto image = renderDocument(s.document());
        for (int i = 0; i < 5; ++i)
            s.undo();
        QVERIFY(s.document().layers[0].tiles.isEmpty());
        for (int i = 0; i < 5; ++i)
            s.redo();
        QCOMPARE(renderDocument(s.document()), image);
    }
    void transformSelection() {
        Session s(blank({100, 100}));
        s.brush.size = 10;
        s.brush.hardness = 1;
        s.beginStroke({20, 20});
        s.endStroke();
        QPainterPath p;
        p.addRect(0, 0, 40, 40);
        s.select(p);
        QTransform t;
        t.translate(50, 0);
        s.transformActive(t);
        QImage image = renderDocument(s.document());
        QCOMPARE(image.pixelColor(20, 20).alpha(), 0);
        QVERIFY(image.pixelColor(70, 20).alpha() > 0);
        s.undo();
        QVERIFY(renderDocument(s.document()).pixelColor(20, 20).alpha() > 0);
    }
    void canvasResize() {
        Session s(blank({100, 100}));
        s.beginStroke({50, 50});
        s.endStroke();
        s.resize({30, 30}, false, {-40, -40});
        QCOMPARE(s.document().size, QSize(30, 30));
        QVERIFY(renderDocument(s.document()).pixelColor(10, 10).alpha() > 0);
        s.undo();
        QCOMPARE(s.document().size, QSize(100, 100));
    }
    void vectorRoundTrip() {
        QTemporaryDir temp;
        Session s(blank());
        s.foreground = QColor(10, 120, 220, 140);
        s.beginStroke({250, 250});
        s.strokeTo({300, 280});
        s.endStroke();
        Object o;
        o.kind = ObjectKind::Path;
        o.path.setFillRule(Qt::WindingFill);
        o.path.moveTo(10, 10);
        o.path.cubicTo(30, 5, 50, 80, 90, 50);
        o.transform.translate(80, 50);
        o.fill = Qt::red;
        s.addObject(o);
        Object text;
        text.kind = ObjectKind::Text;
        text.text = "Editable";
        text.font.setUnderline(true);
        text.font.setStrikeOut(true);
        text.rect = {40, 100, 250, 70};
        s.addObject(text);
        auto before = renderDocument(s.document());
        QString path = temp.filePath("mixed.zyri");
        QCOMPARE(saveProject(s.document(), path), QString());
        auto result = loadProject(path);
        QVERIFY2(bool(result), qPrintable(result.error));
        QCOMPARE(result.document.id, s.document().id);
        QCOMPARE(result.document.layers[1].objects[0].path, o.path);
        QCOMPARE(result.document.layers[1].objects[1].text, text.text);
        QVERIFY(result.document.layers[1].objects[1].font.underline());
        QVERIFY(result.document.layers[1].objects[1].font.strikeOut());
        QCOMPARE(renderDocument(result.document), before);
        QCOMPARE(encodeState(result.document), encodeState(s.document()));
    }
    void invalidProject() {
        QTemporaryDir temp;
        QString path = temp.filePath("bad.zyri");
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("not a zip");
        f.close();
        QVERIFY(!loadProject(path));
        QVERIFY(!decodeState("broken"));
    }
    void saveFailurePreserves() {
        QTemporaryDir temp;
        Session s(blank());
        QString path = temp.filePath("good.zyri");
        QCOMPARE(saveProject(s.document(), path), QString());
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        auto before = file.readAll();
        file.close();
        QVERIFY(!saveProject(s.document(), temp.filePath("missing/sub/file.zyri")).isEmpty());
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), before);
    }
    void pngJpegPdf() {
        QTemporaryDir temp;
        Session s(blank({80, 60}));
        s.foreground = QColor(240, 20, 40, 120);
        s.beginStroke({20, 20});
        s.endStroke();
        auto image = renderDocument(s.document());
        QString png = temp.filePath("image.png");
        QCOMPARE(exportImage(s.document(), png), QString());
        QVERIFY(QFileInfo(png).size() < image.sizeInBytes() / 2);
        auto imported = importImage(png);
        QVERIFY(imported);
        QCOMPARE(renderDocument(imported.document), image);
        QVERIFY(QImageReader(png).read().colorSpace().isValid());
        QString jpeg = temp.filePath("image.jpg");
        QCOMPARE(exportImage(s.document(), jpeg, 1, Qt::yellow), QString());
        QImage j(jpeg);
        QVERIFY(j.pixelColor(70, 50).red() > 240);
        QString pdf = temp.filePath("image.pdf");
        QCOMPARE(exportImage(s.document(), pdf), QString());
        QVERIFY(QFileInfo(pdf).size() > 100);
#ifdef ZYRI_PDF
        auto page = importImage(pdf, 0, 96);
        QVERIFY2(bool(page), qPrintable(page.error));
        QCOMPARE(page.document.size, QSize(80, 60));
#endif
    }
    void svgRoundTrip() {
        QTemporaryDir temp;
        Session s(blank({200, 200}));
        Object object;
        object.rect = {30, 40, 100, 70};
        object.fill = Qt::red;
        s.addObject(object);
        QString path = temp.filePath("scene.svg");
        QCOMPARE(exportSvg(s.document(), path), QString());
        auto loaded = importSvg(path);
        QVERIFY2(bool(loaded), qPrintable(loaded.error));
        QVERIFY2(loaded.warning.isEmpty(), qPrintable(loaded.warning));
        QCOMPARE(renderDocument(loaded.document), renderDocument(s.document()));
    }
    void svgExternalRejected() {
        QTemporaryDir temp;
        QString path = temp.filePath("remote.svg");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("<svg xmlns='http://www.w3.org/2000/svg' width='20' height='20'><image "
                   "href='https://example.com/test.png'/></svg>");
        file.close();
        QVERIFY(!importSvg(path));
    }
    void rasterizeUndo() {
        Session s(blank({200, 200}));
        Object object;
        object.rect = {20, 20, 100, 100};
        s.addObject(object);
        auto image = renderDocument(s.document());
        s.rasterize();
        QCOMPARE(s.document().layers[1].kind, LayerKind::Raster);
        QCOMPARE(renderDocument(s.document()), image);
        s.undo();
        QCOMPARE(s.document().layers[1].kind, LayerKind::Vector);
    }
    void viewRoundTrip() {
        for (double zoom : {.05, 1., 2., 20.})
            for (double rotation : {0., 45., -90.}) {
                ViewTransform view;
                view.center = {100, 200};
                view.zoom = zoom;
                view.rotation = rotation;
                QSizeF viewport(800, 600);
                QPointF point(30, 500);
                QVERIFY(QLineF(view.toDocument(view.matrix(viewport).map(point), viewport), point)
                            .length() < 1e-8);
                QPointF anchor(234, 456), before = view.toDocument(anchor, viewport);
                view.zoomAt(1.3, anchor, viewport);
                QVERIFY(QLineF(view.toDocument(anchor, viewport), before).length() < 1e-8);
            }
    }
    void canvasMouseWorkflow() {
        Session session(blank());
        Canvas canvas(&session);
        canvas.resize(600, 600);
        canvas.view.center = {256, 256};
        canvas.view.zoom = 1;
        QTest::mousePress(&canvas, Qt::LeftButton, {}, QPoint(100, 100));
        QMouseEvent move(QEvent::MouseMove, QPointF(400, 100), QPointF(400, 100), Qt::NoButton,
                         Qt::LeftButton, {});
        QApplication::sendEvent(&canvas, &move);
        QTest::mouseRelease(&canvas, Qt::LeftButton, {}, QPoint(400, 100));
        QVERIFY(session.canUndo());
        QVERIFY(!session.document().layers[0].tiles.isEmpty());
        session.undo();
        QVERIFY(session.document().layers[0].tiles.isEmpty());
    }
    void independentTabs() {
        QTemporaryDir temp;
        MainWindow window(temp.path(), false);
        auto a = window.addDocument(blank());
        auto b = window.addDocument(blank());
        a->session()->beginStroke({20, 20});
        a->session()->endStroke();
        QVERIFY(a->session()->dirty());
        QVERIFY(b->session()->document().layers[0].tiles.isEmpty());
        QCOMPARE(window.current(), b);
    }
    void noOpHistory() {
        Session s(blank());
        s.setLayerProperty("same", [](Layer& layer) { layer.opacity = 1; });
        QVERIFY(!s.canUndo());
        s.beginStroke({10, 10}, 1, true);
        s.endStroke();
        QVERIFY(!s.canUndo());
    }
    void objectCreationIsAtomic() {
        Session s(blank());
        Object o;
        o.rect = {10, 10, 40, 40};
        s.addObject(o);
        QCOMPARE(s.document().layers.size(), size_t(2));
        s.undo();
        QCOMPARE(s.document().layers.size(), size_t(1));
        QVERIFY(!s.canUndo());
        s.redo();
        QCOMPARE(s.document().layers[1].objects[0].id, o.id);
    }
    void rejectedMetadata() {
        auto bytes = encodeState(blank());
        QDataStream input(bytes);
        input.setVersion(QDataStream::Qt_6_8);
        quint32 magic;
        QMap<QString, QByteArray> entries;
        input >> magic >> entries;
        auto json = QJsonDocument::fromJson(entries["manifest.json"]).object();
        json["version"] = 2;
        entries["manifest.json"] = QJsonDocument(json).toJson();
        QByteArray changed;
        QDataStream output(&changed, QIODevice::WriteOnly);
        output.setVersion(QDataStream::Qt_6_8);
        output << magic << entries;
        QVERIFY(!decodeState(changed));
    }
    void nonFiniteInput() {
        Session s(blank());
        QVERIFY(!s.beginStroke({10, 10}, std::numeric_limits<double>::quiet_NaN()));
        s.brush.size = std::numeric_limits<double>::infinity();
        QVERIFY(!s.beginStroke({10, 10}));
        QVERIFY(!s.editing());
    }
    void cancelledFill() {
        Session s(blank({100, 100}));
        s.cancelRequested = [] { return true; };
        s.foreground = Qt::red;
        s.fill({10, 10});
        QVERIFY(s.document().layers[0].tiles.isEmpty());
        QVERIFY(!s.canUndo());
    }
    void recoveryRotatesValidGenerations() {
        QTemporaryDir temp;
        Session s(blank({30, 30}));
        for (int i = 1; i <= 3; ++i) {
            s.setLayerProperty("name", [=](Layer& l) { l.name = QString::number(i); });
            QCOMPARE(saveRecoveryGeneration(s.document(),
                                            temp.filePath(QString("session_%1.zyri").arg(i))),
                     QString());
        }
        auto files = QDir(temp.path()).entryList({"*.zyri"}, QDir::Files, QDir::Name);
        QCOMPARE(files.size(), 2);
        QCOMPARE(loadProject(temp.filePath(files.last())).document.layers[0].name, QString("3"));
        QCOMPARE(loadProject(temp.filePath(files.first())).document.layers[0].name, QString("2"));
    }
    void asynchronousSaveTracksSnapshot() {
        QTemporaryDir temp;
        MainWindow window(temp.path(), false);
        auto canvas = window.addDocument(blank(), temp.filePath("saved.zyri"), true);
        auto s = canvas->session();
        s->setLayerProperty("name", [](Layer& l) { l.name = "saved"; });
        QAction* save = nullptr;
        for (auto a : window.findChildren<QAction*>())
            if (a->text() == "&Save")
                save = a;
        QVERIFY(save);
        save->trigger();
        s->setLayerProperty("name", [](Layer& l) { l.name = "newer"; });
        QTRY_VERIFY_WITH_TIMEOUT(window.statusBar()->currentMessage().startsWith("Saved"), 10000);
        QVERIFY(s->dirty());
        auto disk = loadProject(temp.filePath("saved.zyri"));
        QVERIFY(disk);
        QCOMPARE(disk.document.layers[0].name, QString("saved"));
        s->undo();
        QVERIFY(!s->dirty());
    }
    void recoveryWritesCommittedState() {
        QTemporaryDir temp;
        MainWindow window(temp.path(), false);
        auto canvas = window.addDocument(blank());
        canvas->session()->setLayerProperty("name", [](Layer& l) { l.name = "recover me"; });
        auto timer = window.findChild<QTimer*>("RecoveryTimer");
        QVERIFY(timer);
        QVERIFY(QMetaObject::invokeMethod(timer, "timeout", Qt::DirectConnection));
        QStringList paths;
        QTRY_VERIFY_WITH_TIMEOUT(
            ([&] {
                paths.clear();
                QDir root(temp.filePath("recovery"));
                for (auto folder : root.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                    QDir instance(root.filePath(folder));
                    for (auto file : instance.entryList({"*.zyri"}, QDir::Files))
                        paths << instance.filePath(file);
                }
                return !paths.isEmpty();
            })(),
            10000);
        auto recovered = loadProject(paths.first());
        QVERIFY(recovered);
        QCOMPARE(recovered.document.layers[0].name, QString("recover me"));
    }
    void invalidEditRollsBack() {
        Session s(blank());
        auto state = s.stateId();
        QSignalSpy error(&s, &Session::errorOccurred);
        QVERIFY(!s.apply("invalid", [](Document& d) { d.ppi = 0; }));
        QCOMPARE(s.stateId(), state);
        QCOMPARE(s.document().ppi, 96);
        QVERIFY(!error.isEmpty());
    }
    void emptySelectionCopiesNothing() {
        Session s(blank());
        s.select(QPainterPath());
        QVERIFY(s.copyPixels().isNull());
    }
    void releaseCompletesShapeWithoutMoveEvent() {
        Session s(blank());
        Canvas canvas(&s);
        canvas.resize(600, 600);
        canvas.view.zoom = 1;
        canvas.view.center = {256, 256};
        canvas.setTool(Tool::Polygon);
        QTest::mousePress(&canvas, Qt::LeftButton, {}, QPoint(100, 100));
        QTest::mouseRelease(&canvas, Qt::LeftButton, {}, QPoint(200, 200));
        QCOMPARE(s.document().layers.size(), size_t(2));
        QVERIFY(!s.document().layers[1].objects[0].path.isEmpty());
        s.undo();
        QCOMPARE(s.document().layers.size(), size_t(1));
    }
    void offscreenLayout() {
        QTemporaryDir temp;
        MainWindow window(temp.path(), false);
        auto canvas = window.addDocument(blank());
        window.resize(1440, 900);
        window.ensurePolished();
        window.layout()->activate();
        QImage image(window.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        window.render(&image);
        auto toolbar = window.findChild<QToolBar*>("Tools");
        QVERIFY(toolbar);
        QVERIFY(!toolbar->actions().first()->icon().isNull());
        QVERIFY(canvas->width() > 400);
        QVERIFY(window.minimumSizeHint().height() < 900);
    }
    void extremeObjectBoundsAreRejected() {
        Session s(blank());
        Object object;
        object.rect = {0, 0, 1000, 1000};
        object.transform.scale(1e7, 1e7);
        s.addObject(object);
        QCOMPARE(s.document().layers.size(), size_t(1));
        QVERIFY(!s.canUndo());
        QVERIFY(!s.dirty());
    }
    void randomizedHistory() {
        Session s(blank({200, 200}));
        std::vector<QByteArray> states{encodeState(s.document())};
        QRandomGenerator random(24);
        for (int i = 0; i < 25; ++i) {
            s.foreground = QColor::fromRgb(random.generate());
            s.beginStroke({double(random.bounded(200)), double(random.bounded(200))});
            s.endStroke();
            states.push_back(encodeState(s.document()));
        }
        for (int i = 24; i >= 0; --i) {
            s.undo();
            QCOMPARE(encodeState(s.document()), states[size_t(i)]);
        }
        for (int i = 1; i <= 25; ++i) {
            s.redo();
            QCOMPARE(encodeState(s.document()), states[size_t(i)]);
        }
    }
};
int main(int argc, char** argv) {
    Application app(argc, argv);
    TestEditor tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "test_editor.moc"
