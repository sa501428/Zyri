#include <QApplication>
#include <QIcon>
#include <QMainWindow>
#include <QStatusBar>
#include <QMenuBar>
#include <QSurfaceFormat>

#include "canvas/CanvasViewport.h"
#include "core/Document.h"
#include "core/RasterLayer.h"

int main(int argc, char* argv[])
{
    // Request OpenGL 3.3 Core for the canvas viewport.
    QSurfaceFormat fmt;
    fmt.setVersion(3, 3);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setSamples(0); // no MSAA on the canvas — we handle AA in shaders
    QSurfaceFormat::setDefaultFormat(fmt);

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Zyri"));
    app.setApplicationVersion(QStringLiteral("0.1.0"));
    app.setOrganizationName(QStringLiteral("Zyri"));
    app.setOrganizationDomain(QStringLiteral("zyri.app"));
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/logo.png")));

    // ── Default document (1920x1080, one blank raster layer) ------------------
    auto* doc   = new Zyri::Document(QSize(1920, 1080), 96, Qt::transparent);
    auto  layer = std::make_unique<Zyri::RasterLayer>(QStringLiteral("Background"));
    doc->layers()->addLayer(std::move(layer));

    // ── Canvas viewport -------------------------------------------------------
    auto* viewport = new Zyri::CanvasViewport;
    viewport->setDocument(doc);

    // ── Main window -----------------------------------------------------------
    QMainWindow win;
    win.setWindowTitle(QStringLiteral("Zyri"));
    win.resize(1400, 900);
    win.setCentralWidget(viewport);

    // ── Menu bar --------------------------------------------------------------
    auto* fileMenu = win.menuBar()->addMenu(QStringLiteral("&File"));
    fileMenu->addAction(QStringLiteral("&New"),      QKeySequence::New);
    fileMenu->addAction(QStringLiteral("&Open..."),  QKeySequence::Open);
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("&Save"),     QKeySequence::Save);
    fileMenu->addAction(QStringLiteral("Save &As..."), QKeySequence::SaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("&Quit"),     QKeySequence::Quit,
                        &app, &QApplication::quit);

    auto* editMenu = win.menuBar()->addMenu(QStringLiteral("&Edit"));
    editMenu->addAction(QStringLiteral("&Undo"), QKeySequence::Undo);
    editMenu->addAction(QStringLiteral("&Redo"), QKeySequence::Redo);

    // ── Status bar: live canvas cursor coordinates ----------------------------
    QObject::connect(viewport, &Zyri::CanvasViewport::canvasPosChanged,
                     [&win](const QPointF& p) {
                         win.statusBar()->showMessage(
                             QStringLiteral("X: %1   Y: %2")
                                 .arg(qRound(p.x()))
                                 .arg(qRound(p.y())));
                     });
    win.statusBar()->showMessage(QStringLiteral("Ready"));

    win.show();
    return app.exec();
}
